#include "ff_enc_wrap.h"
#include "common.h"
#include "inslog.h"

ff_enc_wrap::~ff_enc_wrap()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int ff_enc_wrap::open(const ins_enc_option& option)
{
    auto func  = [this](const unsigned char* data, unsigned size, long long pts, int flag)
    {
        write_frame(data, size, pts, flag);
    };

    option_ = option;
    int frame_size = option.width*option.height;
    if (frame_size >= 7680*3840)
    {
        max_queue_size_ = 1;
    }
    else if (frame_size >= 6400*3200)
    {
        max_queue_size_ = 2;
    }
    else
    {
        max_queue_size_ = 5;
    }

    enc_ = std::make_shared<ffh264enc>();
    enc_->set_frame_cb(func);
    auto ret = enc_->open(option_);
    RETURN_IF_NOT_OK(ret);
    th_ = std::thread(&ff_enc_wrap::task, this);
    return INS_OK;
}

void ff_enc_wrap::eos_and_flush() 
{ 
    b_eos_ = true; 
    INS_THREAD_JOIN(th_);
}

int ff_enc_wrap::encode(const std::shared_ptr<ins_frame>& frame) 
{  
    queue_.push(frame); 
    
    int retry = 0;
    while (!quit_)
    {
        if (queue_.size() < max_queue_size_)
        {
            break;
        }
        else
        {
            //printf("queue size:%d >= max:%d\n", queue_.size(), max_queue_size_);
            if (!(++retry%50)) LOGINFO("queue size:%d >= max:%d retry:%d", queue_.size(), max_queue_size_, retry); 
            usleep(30*1000);
        }
    }

    return INS_OK;
}

void ff_enc_wrap::task()
{
    long long cnt = 0;
	while (!quit_)
	{
        std::shared_ptr<ins_frame> frame;
        if (!queue_.pop(frame) || frame == nullptr)
		{
            if (b_eos_)
            {
                break;
            }
            else
            {
                usleep(20*1000);
            }
			continue;
        }

		//ins_clock clock;
		enc_->encode(frame->page_buf->data(), frame->pts);
		//printf("encode time:%lf\n", clock.elapse());
    }

    while (!quit_)
    {
        if (INS_OK != enc_->encode(nullptr, 0)) break;
    }

    LOGINFO("ff enc task finish");
}

void ff_enc_wrap::add_sink(unsigned index, const std::shared_ptr<sink_interface>& sink)
{
    std::lock_guard<std::mutex> lock(mtx_);

    set_sink_video_param(sink);
    sink_.insert(std::make_pair(index, sink));
}

void ff_enc_wrap::del_sink(unsigned index)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = sink_.find(index);
    if (it != sink_.end()) sink_.erase(it);
}

void ff_enc_wrap::write_frame(const unsigned char* data, unsigned size, long long pts, int flag)
{
    // sps pps
    if (flag & ffh264enc::FRAME_FLAG_CONFIG) 
    {
        if (config_ != nullptr) return;
        config_ = std::make_shared<insbuff>(size);
        memcpy(config_->data(), data, size);
        
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto it = sink_.begin(); it != sink_.end(); it++)
        {
            set_sink_video_param(it->second);
        }
    }
    else
    {
        auto frame = std::make_shared<ins_frame>();
        frame->page_buf = std::make_shared<page_buffer>(size);
        frame->pts = pts;
        frame->dts = frame->pts;
        frame->duration = 1000000.0/option_.framerate;
        frame->is_key_frame = flag & ffh264enc::FRAME_FLAG_KEY;
        frame->media_type = INS_MEDIA_VIDEO;
        memcpy(frame->page_buf->data(), data, size);

        std::lock_guard<std::mutex> lock(mtx_);
        for (auto it = sink_.begin(); it != sink_.end(); it++)
        {
            it->second->queue_frame(frame);
        }
    }
}

void ff_enc_wrap::set_sink_video_param(const std::shared_ptr<sink_interface>& sink)
{
    if (config_ == nullptr) return;

    ins_video_param param;
    param.fps = option_.framerate; 
    param.bitrate = option_.bitrate;
    param.width = option_.width;
    param.height = option_.height;
    param.config = config_;
    param.spherical_mode = option_.spherical_mode;
    sink->set_video_param(param);
}

