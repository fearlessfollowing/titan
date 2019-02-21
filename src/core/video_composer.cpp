//#include "ins_blender.h"
#include "video_composer.h"
#include "inslog.h"
#include "common.h"
#include "ffutil.h"
#include <sstream>
#include <cuda.h>
#include <cuda_runtime.h>
#include "cudaEGL.h"
#include "nvbuf_utils.h"
#include "nv_video_enc.h"
#include "nv_video_dec.h"
#include "all_cam_video_queue.h"
#include "xml_config.h" 
#include "stream_sink.h"
#include "render.h"
#include "csp_transform.h"
#include "access_msg_center.h"
#include "stabilization.h"
#include "sps_parser.h"
#include "audio_play.h"
#include "nv_jpeg_enc.h"
#include "jpeg_preview_sink.h"
#include "render_screen.h"
#include "insx11.h"

bool video_composer::hdmi_change_ = false;
bool video_composer::in_hdmi_ = false;

video_composer::~video_composer()
{
    LOGINFO("video_composer 1");
    quit_  = true;
    for (int32_t i = 0; i < INS_CAM_NUM; i++)
    {
        INS_THREAD_JOIN(th_dec_[i]);
    } 

    INS_THREAD_JOIN(th_enc_);

    LOGINFO("video_composer 2");

    dec_.clear();

    enc_.clear();

    INS_THREAD_JOIN(th_screen_render_);

    LOGINFO("video composer destroy");
}

int32_t video_composer::open(const compose_option& option)
{
    int32_t ret = 0;
    
    if (!option.m_sink.empty())
    {
        option_.insert(std::make_pair(option.index, option));
    }
 
    render_ = std::make_shared<render>();
    dec_fps_ = option.ori_framerate; //解码的帧率，解码后流进行编码前可以通过丢帧变化帧率
    rend_mode_ = option.mode;    //渲染出来帧的模式
	render_->set_mode(option.mode);
	render_->set_map_type(option.map);
    render_->set_offset_type(option.crop_flag, option.offset_type);
	render_->set_input_resolution(option.in_w, option.in_h);
    render_->set_output_resolution(option.width, option.height);
    if (option.logo_file != "") render_->set_logo(option.logo_file);
    
    th_enc_ = std::thread(&video_composer::enc_task, this);

    in_hdmi_ = option.hdmi_display;
    if (option.hdmi_display)
    {
        th_screen_render_ = std::thread(&video_composer::screen_render_task, this);
    }

    LOGINFO("compose open in w:%u h:%u mode:%d map:%d hdmi:%d out w:%d h:%d crop:%d type:%d jpeg:%d fps:%lf %lf logo:%s", 
        option.in_w, option.in_h, 
        option.mode, option.map, 
        option.hdmi_display, 
        option.width, option.height, option.crop_flag,option.offset_type,
        option.jpeg,
        option.ori_framerate.to_double(),
        option.framerate.to_double(),
        option.logo_file.c_str());

    return INS_OK;
}

void video_composer::screen_render_task()
{
    auto s_render = std::make_shared<render_sceen>();
    auto ret = s_render->setup();
    if (ret != INS_OK) return;
    hdmi_change_ = false;

    LOGINFO("HDMI render task start");

    while (!quit_)
    {
        if (hdmi_change_)
        {
            LOGINFO("-----hdmi dev change restart render");
            s_render = nullptr;
            insx11::release_x();
            insx11::setup_x();
            s_render = std::make_shared<render_sceen>();
            ret = s_render->setup();
            if (ret != INS_OK) break;
            hdmi_change_ = false;
            std::lock_guard<std::mutex> lock(mtx_rend_img_);
            while (!full_rend_img_queue_.empty())
            {
                auto img = full_rend_img_queue_.front();
                free_rend_img_queue_.push(img);
                full_rend_img_queue_.pop();
            }
            LOGINFO("-----render restart over");
        }

        auto img = deque_full_rend_img();
        if (!img) 
        {
            usleep(20*1000);
            continue;
        }
        s_render->draw(img);
        queue_free_rend_img(img);
    }

    LOGINFO("-----HDMI render task exit");

    s_render = nullptr;
    in_hdmi_ = false; 
}

void video_composer::dec_task(int32_t index)
{
    int32_t ret = 0;
    NvBuffer* buff = nullptr;
    int64_t pts = 0;

    while (1)
    {
        ret = dec_[index]->dequeue_input_buff(buff, 20);
        if (ret == INS_ERR_TIME_OUT)
        {
            continue;
        }
        else if (ret != INS_OK)
        {
            break;
        }
        else
        {
            ret = dequeue_frame(buff, index, pts);
            if (ret != INS_OK) buff->planes[0].bytesused = 0;
            dec_[index]->queue_input_buff(buff, pts);
            if (ret != INS_OK) break;
        }
    }

    dec_exit_num_++;

    LOGINFO("dec:%d task exit", index);
}

void video_composer::enc_task()
{
    int32_t ret = 0;
    int64_t pts = 0;
    int64_t cnt = 0;
    ins_mat_4f mat_4f;

	render_->setup();
    render_->create_out_image(5, free_rend_img_queue_);
    egl_display_ = render_->get_display();

    mtx_.lock();
    render_setup_ = true;
    for (auto& opt : option_)
    {
        auto enc = std::make_shared<nv_video_enc>();
        enc->set_resolution(opt.second.width, opt.second.height);
        enc->set_bitrate(opt.second.bitrate*1000);
        enc->set_framerate(opt.second.framerate);
        for (auto it = opt.second.m_sink.begin(); it != opt.second.m_sink.end(); it++)
        {
            enc->add_output(it->first, it->second);
        }
        std::stringstream ss;
        ss << "enc-" << opt.second.index;
        ret = enc->open(ss.str(), opt.second.mime);
        if (ret != INS_OK) continue;
        enc_.insert(std::make_pair(opt.second.index, enc));
        modes_.insert(std::make_pair(opt.second.index, opt.second.mode));
        //编码帧率>解码帧率的时候,不插帧,将编码帧率=解码帧率
        uint32_t interval = std::max(dec_fps_.num*opt.second.framerate.den/dec_fps_.den/opt.second.framerate.num, 1);
        enc_intervals_.insert(std::make_pair(opt.second.index, interval));

        if (opt.second.jpeg && !jpeg_enc_) //只有一路编码jpeg
        {
            jpeg_index_ = opt.first;
            jpeg_enc_ = std::make_shared<nv_jpeg_enc>();
            auto ret_enc = jpeg_enc_->open("jpgenc", opt.second.width, opt.second.height);
            jpeg_sink_ = std::make_shared<jpeg_preview_sink>();
            auto ret_sink = jpeg_sink_->open(opt.second.framerate.to_double());
            if (ret_enc != INS_OK || ret_sink != INS_OK) 
            {
                jpeg_enc_ = nullptr;
                jpeg_sink_ = nullptr;
            }
        }

        // LOGINFO("index:%d w:%d h:%d bitrate:%d framerate:%d/%d mode:%d interval:%d", 
        //     opt.second.index, 
        //     opt.second.width,
        //     opt.second.height,
        //     opt.second.bitrate,
        //     opt.second.framerate.num,
        //     opt.second.framerate.den,
        //     opt.second.mode,
        //     enc_intervals_[opt.first]);
    }
    mtx_.unlock();

    for (uint32_t i = 0; i < INS_CAM_NUM; i++)
    {
        std::stringstream ss;
        ss << "dec" << i; 
        auto dec = std::make_shared<nv_video_dec>();
        if (i == 0) 
        {
            dec->set_close_wait_time_ms(3000);
        }
        else
        {
            dec->set_close_wait_time_ms(500);
        }
        ret = dec->open(ss.str(), "h264");
        if (ret != INS_OK) return;
        dec_.push_back(dec);
    }

    for (uint32_t i = 0; i < dec_.size(); i++)
    {
        th_dec_[i] = std::thread(&video_composer::dec_task, this, i);
    }

    while (!quit_ || dec_exit_num_ < INS_CAM_NUM)
    {
        std::vector<NvBuffer*> v_dec_buff;
        for (auto& dec : dec_)
        {
            NvBuffer* buff;
            ret = dec->dequeue_output_buff(buff, pts, 0);
            BREAK_IF_NOT_OK(ret);
            v_dec_buff.push_back(buff);
        }

        if (v_dec_buff.size() != dec_.size()) break;

        std::lock_guard<std::mutex> lock(mtx_);
        std::map<uint32_t, NvBuffer*> m_enc_buff;
        for (auto it = enc_.begin(); it != enc_.end(); it++)
        {
            if (cnt%enc_intervals_[it->first]) continue;
            NvBuffer* buff = nullptr;
            ret = it->second->dequeue_input_buff(buff, 20);
            BREAK_IF_NOT_OK(ret);

            for (int32_t i = 0; i < buff->n_planes; i++)
            {
                auto &plane = buff->planes[i];
                plane.bytesused = plane.fmt.stride * plane.fmt.height;
                //LOGINFO("plane:%d stride:%d height:%d", i, plane.fmt.stride, plane.fmt.height);
            }
            m_enc_buff.insert(std::make_pair(it->first, buff));
        }

        cnt++;
        BREAK_IF_NOT_OK(ret);

        mtx_stablz_.lock();
        float* mat = nullptr;
        if (stablz_ && stablz_->get_rotation_mat(pts, mat_4f) == INS_OK) 
            mat = mat_4f.m;
        mtx_stablz_.unlock();

        audio_play::video_timestamp_ = pts;
        ret = compose(v_dec_buff, m_enc_buff, mat);
        BREAK_IF_NOT_OK(ret);
        
        print_fps_info();

        for (auto it = m_enc_buff.begin(); it != m_enc_buff.end(); it++)
        {
            if (jpeg_index_ == it->first) 
            {
                ins_jpg_frame frame;
                frame.buff = jpeg_enc_->encode(it->second);
                frame.pts = pts;
                if (frame.buff) jpeg_sink_->queue_image(frame);
            }
            auto item = enc_.find(it->first);
            if (item == enc_.end()) continue; 
            ret = item->second->queue_intput_buff(it->second, pts); 
            if (ret != INS_OK) 
            {
                re_start_encoder(it->first);
            }
        }
    }

    mtx_.lock();
    for (auto it = enc_.begin(); it != enc_.end(); it++) it->second->send_eos();
    mtx_.unlock();

    render_ = nullptr;

    LOGINFO("enc task exit");
}

void video_composer::re_start_encoder(uint32_t index)
{
    auto it = enc_.find(index);
    if (it == enc_.end()) return;
    enc_.erase(it);

    auto option = option_.find(index);
    if (option == option_.end()) return;

    usleep(20*1000);

    auto enc = std::make_shared<nv_video_enc>();
    enc->set_resolution(option->second.width, option->second.height);
    enc->set_bitrate(option->second.bitrate*1000);
    enc->set_framerate(option->second.framerate);
    for (auto it = option->second.m_sink.begin(); it != option->second.m_sink.end(); it++)
    {
        enc->add_output(it->first, it->second);
    }
    std::stringstream ss;
    ss << "enc-" << option->second.index;
    auto ret = enc->open(ss.str(), option->second.mime);
    if (ret != INS_OK) return;

    enc_.insert(std::make_pair(index, enc));
}

int32_t video_composer::dequeue_frame(NvBuffer* buff, int32_t index, int64_t& pts)
{
    while (!quit_)
    {
        auto frame = repo_->deque_frame(index);
        if (frame == nullptr)
        {
            usleep(5*1000);
            continue;
        }
        else
        {
            memcpy(buff->planes[0].data, frame->page_buf->data(), frame->page_buf->offset());
            buff->planes[0].bytesused = frame->page_buf->offset();
            pts = frame->pts;
            return INS_OK;
        }
    }
    return INS_ERR;
}

int32_t video_composer::compose(std::vector<NvBuffer*>& v_in_buff, std::map<uint32_t, NvBuffer*>& m_out_buff, const float* mat)
{
    if (b_gpu_err_) return INS_OK;
    
    std::vector<EGLImageKHR> v_in_img; 
    for (uint32_t i = 0; i < v_in_buff.size(); i++)
    {
        auto img = NvEGLImageFromFd(egl_display_, v_in_buff[i]->planes[0].fd); 
        RETURN_IF_TRUE(img == nullptr, "NvEGLImageFromFd fail", INS_ERR);
        v_in_img.push_back(img);   
    }

    std::vector<bool> v_half;
    std::vector<EGLImageKHR> v_out_img; 
    for (auto it = m_out_buff.begin(); it != m_out_buff.end(); it++)
    {
        auto img = NvEGLImageFromFd(egl_display_, it->second->planes[0].fd); 
        RETURN_IF_TRUE(img == nullptr, "NvEGLImageFromFd fail", INS_ERR);
        v_out_img.push_back(img);
        bool b_half = false;
        if (rend_mode_ != INS_MODE_PANORAMA && modes_[it->first] == INS_MODE_PANORAMA) b_half = true;
        v_half.push_back(b_half);
    }

    auto rend_img = deque_free_rend_img();
    render_->draw(v_in_img, rend_img, mat);

    for (uint32_t i = 0; i < v_in_img.size(); i++)
    {
        NvDestroyEGLImage(egl_display_, v_in_img[i]);
    }

    for (uint32_t i = 0; i < dec_.size(); i++)
    {
        dec_[i]->queue_output_buff(v_in_buff[i]);
    }
    
    if (!v_out_img.empty())
    {
        auto ret = csp_transform::transform_scaling(rend_img, v_out_img, v_half);
        if (ret != INS_OK)
        {
            json_obj obj;
            obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
            obj.set_int("code", INS_ERR_GPU);
            access_msg_center::queue_msg(0, obj.to_string());
            b_gpu_err_ = true;
        }
    }

    queue_full_rend_img(rend_img);

    for (uint32_t i = 0; i < v_out_img.size(); i++)
    {
        NvDestroyEGLImage(egl_display_, v_out_img[i]);
    }

    return INS_OK;
}

int32_t video_composer::add_encoder(const compose_option& option)
{
    if (enc_.find(option.index) != enc_.end())
    {
        LOGERR("add enc index:%u exist", option.index);
        return INS_ERR;
    }

    assert(enc_.size() < 2);

    std::lock_guard<std::mutex> lock(mtx_);
    
    if (!render_setup_) 
    {
        option_.insert(std::make_pair(option.index, option));
        return INS_OK;
    }

    auto enc = std::make_shared<nv_video_enc>();
    enc->set_resolution(option.width, option.height);
    enc->set_bitrate(option.bitrate*1000);
    enc->set_framerate(option.framerate);
    for (auto it = option.m_sink.begin(); it != option.m_sink.end(); it++)
    {
        enc->add_output(it->first, it->second);
    }
    std::stringstream ss;
    ss << "enc-" << option.index;
    auto ret = enc->open(ss.str(), option.mime);
    RETURN_IF_TRUE(ret != INS_OK, "enc open fail", ret);

    enc_.insert(std::make_pair(option.index, enc));
    option_.insert(std::make_pair(option.index, option));
    modes_.insert(std::make_pair(option.index, option.mode));

    //编码帧率>解码帧率的时候,不插帧,将编码帧率=解码帧率
    uint32_t interval = std::max(dec_fps_.num*option.framerate.den/dec_fps_.den/option.framerate.num, 1);
    enc_intervals_.insert(std::make_pair(option.index, interval));

    return INS_OK;
}

void video_composer::delete_encoder(uint32_t index)
{
    mtx_.lock();
    auto it = enc_.find(index);
    it->second->send_eos();
    if (it != enc_.end()) enc_.erase(it);
    mtx_.unlock();
}

void video_composer::encoder_del_output(uint32_t enc_index, uint32_t sink_index)
{
    auto it = enc_.find(enc_index);
    if (it == enc_.end()) return;
    it->second->delete_output(sink_index);
}

void video_composer::encoder_add_output(uint32_t enc_index, uint32_t sink_index, std::shared_ptr<sink_interface> sink)
{
    auto it = enc_.find(enc_index);
    if (it == enc_.end()) return;
    it->second->add_output(sink_index, sink);
}

void video_composer::print_fps_info() 
{
	if (cnt_ == -1)
	{
        cnt_ = 0;
		clock_ = std::make_shared<ins_clock>();
	}

	if (cnt_++ > 90)
	{
        double fps = cnt_/clock_->elapse_and_reset();
		cnt_ = 0;
		LOGINFO("fps: %lf", fps); 
	}
}

int32_t video_composer::init_cuda()
{
    static bool init = false;
    if (init) return INS_OK;
    init = true;

    int32_t width = 3840;
    int32_t height = 1920;

    ins_clock t;
    insegl egl;
    int32_t ret;
    ret = egl.setup_pbuff(0, width, height);
    RETURN_IF_NOT_OK(ret);

    EGLImageKHR out_egl_img = nullptr;
    GLuint texture = 0;
    int32_t fd = -1;
 
    do {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        auto in_egl_img = egl.create_egl_img(texture);
        BREAK_IF_TRUE(in_egl_img == nullptr, "create egl img fail");

        NvBufferCreate(&fd, width, height, NvBufferLayout_Pitch, NvBufferColorFormat_YUV420);
        BREAK_IF_TRUE(fd < 0, "NvBufferCreate");

        std::vector<bool> v_half;
        std::vector<EGLImageKHR> v_out_img; 
        out_egl_img = NvEGLImageFromFd(egl.get_display(), fd);
        BREAK_IF_TRUE(out_egl_img == nullptr, "NvEGLImageFromFd");
        v_out_img.push_back(out_egl_img);
        v_half.push_back(false);

        csp_transform::transform_scaling(in_egl_img, v_out_img, v_half);
    } while (0);

    if (out_egl_img) NvDestroyEGLImage(egl.get_display(), out_egl_img);
    if (fd >= 0) NvBufferDestroy(fd);
    if (texture) glDeleteTextures(1, &texture);

    LOGINFO("cuda init time:%lf", t.elapse());

    return INS_OK;
}

