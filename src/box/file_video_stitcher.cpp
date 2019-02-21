#include "ins_blender.h"
#include "file_video_stitcher.h"
#include <sstream>
#include "common.h"
#include "ins_clock.h"
#include "inslog.h"
#include "file_sink.h"
#include "insdemux.h"
#include "nv_video_dec.h"
#include "nv_video_enc.h"
#include "offset_wrap.h"
#include "ffh264dec.h"
#include "cudaEGL.h"
#include "nvbuf_utils.h"

file_video_stitcher::~file_video_stitcher()
{
    quit_ = true;

    for (int i = 0; i < INS_CAM_NUM; i++)
    {
        INS_THREAD_JOIN(dec_th_[i]);
    }

    INS_THREAD_JOIN(enc_th_);

    if (egl_display_) 
    {
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
    }

    LOGINFO("file video stitcher close");
}

int32_t file_video_stitcher::open(const task_option& option)
{
    if (option.output.width > 3840 || option.output.height > 3840)
    {
        LOGERR("not support now"); return INS_ERR;
    }

    int32_t ret;
    framerate_ = option.input.framerate;
    total_frames_ = option.input.total_frames;

    //offset
    std::string offset;
    if (option.blend.offset != "")
    {
        offset = option.blend.offset;
    }
    else
    {
        LOGINFO("video calc offset, index:%d timestamp:%lf", option.blend.cap_index, option.blend.cap_time);
        ret = calc_offset(option.input.file[option.blend.cap_index], option.blend.len_ver, option.blend.cap_time, offset);
        RETURN_IF_NOT_OK(ret);
    }

    //sink
    // std::function<void(int)> func = [this](int error)
    // {
    //     result_ = error; quit_ = true;
    // };
    // sink_ = std::make_shared<file_sink>(option.output.file, false, true, false);
    // sink_->set_event_cb(func);

    egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    RETURN_IF_TRUE(egl_display_ == EGL_NO_DISPLAY, "eglGetDisplay error", INS_ERR);
    ret = eglInitialize(egl_display_, nullptr, nullptr);
    RETURN_IF_TRUE(ret == 0, "eglInitialize error", INS_ERR);

    //demux
    for (uint32_t i = 0; i < option.input.file[0].size(); i++)
    {
        auto demux = std::make_shared<ins_demux>();
        ret = demux->open(option.input.file[0][i].c_str());
        RETURN_IF_NOT_OK(ret);
        ins_demux_param demux_param;
        demux->get_param(demux_param);
        demux_.push_back(demux);

        std::stringstream ss;
        ss << "dec" << i; 
        auto dec = std::make_shared<nv_video_dec>();
        ret = dec->open(ss.str(), "h264");
        RETURN_IF_TRUE(ret != INS_OK, "decode open fail", ret);
        dec_.push_back(dec);
    }

    auto sink = std::make_shared<file_sink>(option.output.file, false, true, false);

    //encode
    enc_ = std::make_shared<nv_video_enc>();
    enc_->set_resolution(option.output.width, option.output.height);
    enc_->set_bitrate(option.output.bitrate*1000);
    enc_->add_output(0, sink);
    ret = enc_->open("enc", "h264");
    RETURN_IF_NOT_OK(ret);

    blender_ = std::make_shared<ins_blender>(INS_BLEND_TYPE_OPTFLOW, option.blend.mode, ins::BLENDERTYPE::YUV420P_CUDA);
    blender_->set_map_type(INS_MAP_FLAT);
    blender_->set_speed(ins::SPEED::FAST);
    blender_->set_optflow_type(ins::OPTFLOWTYPE::OPTFLOW2);
    blender_->setup(offset);

    for (int32_t i = 0; i < INS_CAM_NUM; i++)
    {
        dec_th_[i] = std::thread(&file_video_stitcher::dec_task, this, i);
    }

    enc_th_ = std::thread(&file_video_stitcher::enc_task, this);

    result_ = INS_OK;
    running_th_cnt_ = INS_CAM_NUM + 1;

    LOGINFO("file_video_stitcher open success");

    return INS_OK;
}

int32_t file_video_stitcher::read_frame(NvBuffer* buff, int32_t index, int64_t& pts)
{
    std::shared_ptr<ins_demux_frame> frame;
    while (1)
    {
        if (demux_[index]->get_next_frame(frame))
        {
            LOGINFO("demux:%d eos", index);
            return INS_ERR;
        }

        if (frame->media_type == INS_MEDIA_VIDEO)
        {
            memcpy(buff->planes[0].data, frame->data, frame->len);
            buff->planes[0].bytesused = frame->len;
            pts = frame->pts;
            return INS_OK;
        }
    }
}

void file_video_stitcher::dec_task(int32_t index)
{
    int32_t ret = 0;
    int64_t pts = 0;
    NvBuffer* buff = nullptr;

    while (!quit_)
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
            ret = read_frame(buff, index, pts);
            if (ret != INS_OK) buff->planes[0].bytesused = 0;
            dec_[index]->queue_input_buff(buff, pts);
            if (ret != INS_OK) break;
        }
    }

    running_th_cnt_--;

    LOGINFO("dec:%d task exit", index);
}

void file_video_stitcher::enc_task()
{
    int32_t ret = 0;
    int64_t pts = 0;

    while (!quit_)
    {
        std::vector<NvBuffer*> v_dec_buff;
        for (auto& dec : dec_)
        {
            NvBuffer* buff;
            ret = dec->dequeue_output_buff(buff, pts, 0);
            if (ret != INS_OK) break;
            v_dec_buff.push_back(buff);
        }

        if (v_dec_buff.size() != dec_.size()) break;

        NvBuffer* enc_buff = nullptr;
        ret = enc_->dequeue_input_buff(enc_buff, 0);
        if (ret != INS_OK) break;

        for (int32_t i = 0; i < enc_buff->n_planes; i++)
        {
            auto &plane = enc_buff->planes[i];
            plane.bytesused = plane.fmt.stride * plane.fmt.height;
        }

        ret = frame_stitching(v_dec_buff, enc_buff);
        if (ret != INS_OK) break;

        progress_ = ++cur_frames_*100.0/total_frames_;
        print_fps_info();

        for (uint32_t i = 0; i < dec_.size(); i++)
        {
            dec_[i]->queue_output_buff(v_dec_buff[i]);
        }

        enc_->queue_intput_buff(enc_buff, pts);
    }

    enc_->send_eos();

    running_th_cnt_--;

    LOGINFO("enc task exit");
}

int32_t file_video_stitcher::frame_stitching(std::vector<NvBuffer*>& v_in_buff, NvBuffer* out_buff)
{
    std::vector<EGLImageKHR> v_in_img;
    for (uint32_t i = 0; i < v_in_buff.size(); i++)
    {
        auto img = NvEGLImageFromFd(egl_display_, v_in_buff[i]->planes[0].fd); 
        RETURN_IF_TRUE(img == nullptr, "NvEGLImageFromFd fail", INS_ERR);
        v_in_img.push_back(img);
    }

    auto out_img = NvEGLImageFromFd(egl_display_, out_buff->planes[0].fd); 
    RETURN_IF_TRUE(out_img == nullptr, "NvEGLImageFromFd fail", INS_ERR);

    auto ret = blender_->blend(v_in_img, out_img, true);
    
    for (uint32_t i = 0; i < v_in_img.size(); i++)
    {
        NvDestroyEGLImage(egl_display_, v_in_img[i]);
    }

    NvDestroyEGLImage(egl_display_, out_img);

    return ret;
}

int32_t file_video_stitcher::calc_offset(const std::vector<std::string>& file, int32_t len_ver, double timestamp, std::string& offset)
{
    std::vector<ins_img_frame> v_img;
    for (uint32_t i = 0; i < INS_CAM_NUM; i++)
    {
        ins_demux demux;
        int32_t ret = demux.open(file[i].c_str());
        if (ret != INS_OK) return INS_ERR_ORIGIN_VIDEO_DAMAGE;

        ins_demux_param param;
        demux.get_param(param);

        ret = demux.seek_frame(timestamp, INS_MEDIA_VIDEO);
        RETURN_IF_NOT_OK(ret);

        std::shared_ptr<ins_demux_frame> frame;
        while (1)
        {
            if (demux.get_next_frame(frame))
            {
                LOGINFO("i:%d demux over", i);
                break;
            }
    
            if (frame->media_type == INS_DEMUX_VIDEO && frame->is_key) break;
        }

        if (frame == nullptr)
        {
            LOGERR("i:%d cannt get frame", i);
            return INS_ERR;
        }

        ffh264dec_param dec_param;
        dec_param.width = param.width;
        dec_param.height = param.height;
        dec_param.sps = param.sps;
        dec_param.sps_len = param.sps_len;
        dec_param.pps = param.pps;
        dec_param.pps_len = param.pps_len;
        dec_param.o_pix_fmt = AV_PIX_FMT_BGR24;
    
        ffh264dec dec;
        ret = dec.open(dec_param);
        RETURN_IF_NOT_OK(ret);

        auto BGR_frame = dec.decode(frame->data, frame->len, 0, 0);
        if (BGR_frame == nullptr)
        {
            LOGERR("i:%d frame decode fail", i);
            return INS_ERR;
        }

        v_img.push_back(*(BGR_frame.get()));
    }

    // offset_wrap ow;
    // int32_t ret = ow.calc(v_img, len_ver);
    // RETURN_IF_NOT_OK(ret);

    // if (v_img[0].w*3 == v_img[0].h*4)
    // {
    //     offset = ow.get_4_3_offset();
    // }
    // else
    // {
    //     offset = ow.get_16_9_offset();
    // }

    LOGINFO("calc offset:%s", offset.c_str());

    return INS_OK;
}

void file_video_stitcher::print_fps_info()
{
	if (cnt_ == -1)
	{
        cnt_ = 0;
		clock_ = std::make_shared<ins_clock>();
	}

	if (cnt_++ > 20)
	{
        double fps = cnt_/clock_->elapse_and_reset();
		cnt_ = 0;
		LOGINFO("fps: %lf", fps);
	}
}
