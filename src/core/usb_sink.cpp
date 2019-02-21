#include "usb_sink.h"
#include "inslog.h"
#include "common.h"
#include <assert.h>
#include <limits.h>
#include "cam_manager.h"
#include "usb_msg.h"
#include "json_obj.h"
#include "ins_base64.h"
#include <fstream>

usb_sink::~usb_sink()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int32_t usb_sink::open(cam_manager* camera)
{
    camera_ = camera;
    buff_ = std::make_shared<insbuff>(2048);
    memset(buff_->data(), 0, buff_->size());
    th_ = std::thread(&usb_sink::task, this);
    return INS_OK;
}

void usb_sink::task()
{
    auto index = camera_->master_index();

    LOGINFO("---------usb sink task start");

    while (!quit_)
    {
        auto buff = deque_frame();
        if (buff == nullptr)
        {
            usleep(10*1000);
        }
        else
        {
            amba_frame_info* head = (amba_frame_info*)buff->data();
            //LOGINFO("------send type:%d size:%d", head->type, buff->offset());
            camera_->send_data(index, buff->data(), buff->offset());
        }
    }

    LOGINFO("-------usb sink task exit");
}

void usb_sink::queue_frame(const std::shared_ptr<ins_frame>& frame)
{
    assert(frame->media_type == INS_MEDIA_AUDIO);
    if (!b_audio_) return;
    if (!audio_param_) return;

    // static bool flag = false;

    // if (frame->media_type == INS_MEDIA_AUDIO && !flag)
    // {
    //     FILE* fp = fopen("/home/nvidia/1.aac", "wb");
    //     fwrite(frame->buf->data(), 1, frame->buf->size(), fp);
    //     fclose(fp);
    //     flag = true;
    // }

    std::lock_guard<std::mutex> lock(mtx_);
    if (audio_queue_.size() > 500) 
    {
        LOGERR("usb sink audio queue size:%lu discard", audio_queue_.size());
    }
    else
    {
        audio_queue_.push_back(frame);
    }
}

void usb_sink::queue_gps(std::shared_ptr<ins_gps_data>& gps)
{
    if (!b_gps_) return;

    std::lock_guard<std::mutex> lock(mtx_);
    if (gps_queue_.size() > 100)
    {
        LOGERR("usb sink gps queue size:%lu discard", gps_queue_.size());
    }
    else
    {
        gps_queue_.push_back(gps);
    }
}

std::shared_ptr<insbuff> usb_sink::deque_frame()
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (!prj_file_send_ && prj_file_name_ != "")
    {
        prj_file_send_ = true;
        return gen_prj_file_frame();
    }

    int64_t gps_min_pts = LONG_LONG_MAX;
    int64_t audio_min_pts = LONG_LONG_MAX;
    if (!audio_queue_.empty()) audio_min_pts = audio_queue_.front()->pts;
    if (!gps_queue_.empty()) gps_min_pts = gps_queue_.front()->pts;

    if (audio_min_pts <= gps_min_pts)
    {
        return deque_audio_frame();
    }
    else
    {
        return deque_gps_frame();
    }
}

std::shared_ptr<insbuff> usb_sink::deque_audio_frame()
{
    if (audio_queue_.empty()) return nullptr;

    if (!audio_param_send_)
    {
        if (!audio_param_) return nullptr;
        audio_param_send_ = true;
        json_obj obj;
        obj.set_string("mime", audio_param_->mime);
        obj.set_int("samplerate", audio_param_->samplerate);
        obj.set_int("channel", audio_param_->channels);
        obj.set_int("bitrate", audio_param_->bitrate);
        obj.set_bool("spatial", audio_param_->b_spatial);
        obj.set_int("frame_size", 1024);
        auto config = ins_base64::encode(audio_param_->config);
        obj.set_string("config", config);
        
        amba_frame_info* head = (amba_frame_info*)buff_->data();
        head->syncword = 0xffffffff;
        head->type = AMBA_FRAME_AUDIO_OPT;
        head->sequence = 0;
        head->timestamp = 0;
        head->size = obj.to_string().length();
        buff_->set_offset(sizeof(amba_frame_info) + head->size);
        memcpy(buff_->data()+sizeof(amba_frame_info), obj.to_string().c_str(), head->size);

        return buff_;
    }
    else
    {
        auto frame = audio_queue_.front();
        audio_queue_.pop_front();

        if (frame->buf->offset() + sizeof(amba_frame_info) > buff_->size())
        {
            LOGINFO("!!!!!!! audio size:%d > buff size:%d realloc", frame->buf->size(), buff_->size());
            buff_ = std::make_shared<insbuff>(frame->buf->size()+sizeof(amba_frame_info));
            memset(buff_->data(), 0, buff_->size());
        }

        amba_frame_info* head = (amba_frame_info*)buff_->data();
        head->syncword = 0xffffffff;
        head->type = AMBA_FRAME_AUDIO_DATA;
        head->sequence = audio_seq_++;
        head->timestamp = frame->pts;
        head->size = frame->buf->offset();
        buff_->set_offset(sizeof(amba_frame_info) + head->size);
        memcpy(buff_->data()+sizeof(amba_frame_info), frame->buf->data(), head->size);

        return buff_;
    }
}


std::shared_ptr<insbuff> usb_sink::deque_gps_frame()
{
    if (gps_queue_.empty()) return nullptr;

    auto frame = gps_queue_.front();
    gps_queue_.pop_front();

    amba_frame_info* head = (amba_frame_info*)buff_->data();
    head->syncword = 0xffffffff;
    head->type = AMBA_FRAME_GPS_DATA;
    head->sequence = gps_seq_++;
    head->timestamp = frame->pts; 
    head->size = sizeof(int32_t) + 9*sizeof(double);
    buff_->set_offset(sizeof(amba_frame_info) + head->size);

    *((int32_t*)(buff_->data()+sizeof(amba_frame_info))) = frame->fix_type;
    double* p = (double*)(buff_->data()+sizeof(amba_frame_info)+sizeof(int32_t));
    p[0] = frame->latitude;
    p[1] = frame->longitude;
    p[2] = frame->altitude;
    p[3] = frame->h_accuracy;
    p[4] = frame->v_accuracy;
    p[5] = frame->velocity_east;
    p[6] = frame->velocity_north;
    p[7] = frame->velocity_up;
    p[8] = frame->speed_accuracy;

    return buff_;
}

std::shared_ptr<insbuff> usb_sink::gen_prj_file_frame()
{
    std::fstream s(prj_file_name_, s.binary | s.in);
    if (!s.is_open())
    {
        LOGERR("file:%s open fail", prj_file_name_.c_str());
        return nullptr;
    }

    s.seekg(0, std::ios::end);
    int size = (int)s.tellg();
    s.seekg(0, std::ios::beg);

    auto buff = std::make_shared<insbuff>(size+sizeof(amba_frame_info));
    if (!s.read((char*)buff->data()+sizeof(amba_frame_info), size))
    {
        LOGERR("file:%s read fail, req size:%d read size:%d", prj_file_name_.c_str(), size, s.gcount());
        return nullptr;
    }

    amba_frame_info* head = (amba_frame_info*)buff->data();
    head->syncword = 0xffffffff;
    head->type = AMBA_FRAME_PRJ_INFO;
    head->sequence = 0;
    head->timestamp = 0;
    head->size = size;
    buff->set_offset(sizeof(amba_frame_info) + head->size);

    return buff;
}
