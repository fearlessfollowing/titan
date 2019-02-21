#include "img_seq_composer.h"
#include "compose_option.h"
#include "common.h"
#include "inslog.h"
#include "nv_video_enc.h"
#include "nv_jpeg_dec.h"
#include "render.h"
#include "cam_img_repo.h"
#include "nvbuf_utils.h"
#include "xml_config.h"
#include "csp_transform.h"
#include "stream_sink.h"
#include <sstream>

img_seq_composer::~img_seq_composer()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
    LOGINFO("image seq composer close");
}

int32_t img_seq_composer::open(const compose_option& option)
{
    mode_ = option.mode;
    enc_ = std::make_shared<nv_video_enc>();
    enc_->set_resolution(option.width, option.height);
    enc_->set_bitrate(option.bitrate*1000);
    enc_->set_framerate(option.framerate);
    for (auto it = option.m_sink.begin(); it != option.m_sink.end(); it++)
    {
        enc_->add_output(it->first, it->second);
    }

    auto ret = enc_->open("enc", option.mime);
    RETURN_IF_NOT_OK(ret);
 
    for (int32_t i = 0; i < INS_CAM_NUM; i++)
    {
        auto dec = std::make_shared<nv_jpeg_dec>();
        std::stringstream ss;
        ss << "dec-" << i;
        ret = dec->open(ss.str());
        RETURN_IF_NOT_OK(ret);
        dec_.push_back(dec);
    }

    std::string offset = xml_config::get_offset(INS_CROP_FLAG_PIC);
    LOGINFO("PIC 4:3 offset:%s", offset.c_str());

    render_ = std::make_shared<render>();
	render_->set_mode(option.mode);
	render_->set_map_type(option.map);
	render_->set_input_resolution(option.in_w, option.in_h);
    render_->set_output_resolution(option.width, option.height);

    th_ = std::thread(&img_seq_composer::enc_task, this);

    return INS_OK;
}

void img_seq_composer::enc_task()
{
    int32_t ret = 0;
    int64_t pts = 0;

	render_->setup();

    std::queue<EGLImageKHR> img_q; 
    render_->create_out_image(1, img_q);
    rend_img_ = img_q.front();

    while (!quit_)
    {
        std::map<uint32_t, std::shared_ptr<ins_frame>> frames;
        while (!quit_)
        {
            frames = repo_->dequeue_frame();
            if (frames.empty() || frames.begin()->second->metadata.raw)
            {
                usleep(100*1000);
            }
            else
            {
                break;
            }
        }
         
        if (frames.empty()) break;

        std::vector<int32_t> v_fd;
        for (int32_t i = 0; i < INS_CAM_NUM; i++)
        {
            int32_t fd;
            auto it = frames.find(i);
            assert(it != frames.end());
            ret = dec_[i]->decode(it->second->page_buf, fd);
            BREAK_IF_NOT_OK(ret);
            v_fd.push_back(fd);
        }
        
        if (v_fd.size() < INS_CAM_NUM) break;

        NvBuffer* buff = nullptr;
        ret = enc_->dequeue_input_buff(buff, 20);
        BREAK_IF_NOT_OK(ret);

        for (int32_t i = 0; i < buff->n_planes; i++)
        {
            auto &plane = buff->planes[i];
            plane.bytesused = plane.fmt.stride * plane.fmt.height;
            //LOGINFO("plane:%d stride:%d height:%d", i, plane.fmt.stride, plane.fmt.height);
        }

        ret = compose(v_fd, buff->planes[0].fd);
        BREAK_IF_NOT_OK(ret);

        enc_->queue_intput_buff(buff, frames[0]->pts);
    }

    enc_->send_eos();

    render_ = nullptr;

    LOGINFO("compose task exit");
}

int32_t img_seq_composer::compose(std::vector<int32_t>& v_in_fd, int32_t out_fd)
{
    auto egl_display_ = render_->get_display();

    std::vector<EGLImageKHR> v_in_img; 
    for (uint32_t i = 0; i < v_in_fd.size(); i++)
    {
        auto img = NvEGLImageFromFd(egl_display_, v_in_fd[i]); 
        RETURN_IF_TRUE(img == nullptr, "NvEGLImageFromFd fail", INS_ERR);
        v_in_img.push_back(img);   
    }

    std::vector<bool> v_half;
    std::vector<EGLImageKHR> v_out_img; 
    auto out_img = NvEGLImageFromFd(egl_display_, out_fd); 
    RETURN_IF_TRUE(out_img == nullptr, "NvEGLImageFromFd fail", INS_ERR);
    v_out_img.push_back(out_img);
    v_half.push_back(false);

    render_->draw(v_in_img, rend_img_);
    
    csp_transform::transform_scaling(rend_img_, v_out_img, v_half);
    
    for (uint32_t i = 0; i < v_in_img.size(); i++)
    {
        NvDestroyEGLImage(egl_display_, v_in_img[i]);
    }

    for (uint32_t i = 0; i < v_out_img.size(); i++)
    {
        NvDestroyEGLImage(egl_display_, v_out_img[i]);
    }

    return INS_OK;
}