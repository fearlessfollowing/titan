#ifndef _GPS_FILE_H_
#define _GPS_FILE_H_

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <deque>
#include "gps_sink.h"
#include "insbuff.h"

class gps_file : public gps_sink
{
public:
    virtual ~gps_file();
    int32_t open(std::string filename);
    virtual void queue_gps(std::shared_ptr<ins_gps_data>& gps);

private:
    void task();
    std::shared_ptr<page_buffer> dequeue_buff();
    std::thread th_;
    std::mutex mtx_;
    FILE* fp_ = nullptr;
    std::string filename_;
    bool quit_ = false;
    std::deque<std::shared_ptr<page_buffer>> queue_;
    std::shared_ptr<page_buffer> buff_;
};

#endif