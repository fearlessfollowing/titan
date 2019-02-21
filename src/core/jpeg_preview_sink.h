#ifndef __JPEG_PREVIEW_SINK_H__
#define __JPEG_PREVIEW_SINK_H__

#include <thread>
#include <mutex>
#include <queue>
#include <memory>
#include <string>
#include "ins_frame.h"

class jpeg_preview_sink
{
public:
    ~jpeg_preview_sink();
    int32_t open(double framerate);
    void queue_image(const ins_jpg_frame& frame);
private:
    bool dequeue_unsave_image(ins_jpg_frame& frame);
    void queue_save_url(std::string url);
    void task();
    int32_t save_to_file(std::string url, uint8_t* data, uint32_t size);
    std::queue<ins_jpg_frame> unsave_queue_;
    std::queue<std::string> save_queue_;
    std::mutex mtx_;
    std::thread th_;
    uint32_t buff_cnt_ = 100;
    bool quit_ = false;
};

#endif