#ifndef _GPS_DEV_H_
#define _GPS_DEV_H_

extern "C"
{
#include "ublox_gps.h"
}
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class gps_dev
{
public:
    ~gps_dev();
    void open();
    void set_data_cb(std::function<void(GoogleGpsData&)> cb)
    {
        data_cb_ = cb;
    }
    std::function<void(GoogleGpsData&)> data_cb_;
    std::atomic_bool dev_error_;
    std::atomic_int no_cb_cnt_;
    int64_t cnt_ = 0;

private:
    void task();
    std::thread th_;
    std::condition_variable cv_;
    std::mutex mtx_;
    bool quit_ = false;
};

#endif