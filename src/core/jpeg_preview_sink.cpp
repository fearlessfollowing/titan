#include "jpeg_preview_sink.h"
#include "common.h"
#include "inslog.h"
#include <unistd.h>

#define PREVIEW_JPEG_PATH "/tmp/preview_jpeg"

jpeg_preview_sink::~jpeg_preview_sink()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);

    std::string cmd = std::string("rm -r ") + PREVIEW_JPEG_PATH;
    system(cmd.c_str()); //删除目录 
}

int32_t jpeg_preview_sink::open(double framerate)
{   
    std::string cmd = std::string("rm -r ") + PREVIEW_JPEG_PATH;
    system(cmd.c_str()); //删除目录
    cmd = std::string("mkdir ") + PREVIEW_JPEG_PATH;
    system(cmd.c_str()); //重新创建目录

    buff_cnt_ = (uint32_t)framerate*5; //缓存5s
    
    th_  = std::thread(&jpeg_preview_sink::task, this);
    return INS_OK;
}

void jpeg_preview_sink::queue_image(const ins_jpg_frame& frame)
{
    std::lock_guard<std::mutex> lock(mtx_);
    unsave_queue_.push(frame);
}

bool jpeg_preview_sink::dequeue_unsave_image(ins_jpg_frame& frame)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (unsave_queue_.empty()) return false;
    frame = unsave_queue_.front();
    unsave_queue_.pop();
    return true;
}

void jpeg_preview_sink::queue_save_url(std::string url)
{
    save_queue_.push(url);

    if (save_queue_.size() > buff_cnt_)
    {
        auto front_url = save_queue_.front();
        save_queue_.pop();
        unlink(front_url.c_str());
        //LOGINFO("delete image:%s", front_url.c_str());
    }
}

void jpeg_preview_sink::task()
{
    ins_jpg_frame frame;
    while (!quit_)
    {
        if (!dequeue_unsave_image(frame))
        {
            usleep(20*1000);
            continue;
        }
        std::string url = std::string(PREVIEW_JPEG_PATH) + "/" + std::to_string(frame.pts) + ".jpg";
        save_to_file(url, frame.buff->data(), frame.buff->offset());
        queue_save_url(url);
    }
}

int32_t jpeg_preview_sink::save_to_file(std::string url, uint8_t* data, uint32_t size)
{
    FILE* fp = fopen(url.c_str(), "wb");
    if (fp == nullptr)
    {
        LOGERR("file:%s open fail", url.c_str());
        return INS_ERR;
    }

    fwrite(data, 1, size, fp);
    fclose(fp);

    //LOGINFO("save file:%s", url.c_str());

    return INS_OK;
}