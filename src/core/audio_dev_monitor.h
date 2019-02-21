#ifndef _SND_MONITOR_H_
#define _SND_MONITOR_H_

#include <thread>
#include <mutex>
#include <condition_variable>

class audio_dev_monitor
{
public:
    ~audio_dev_monitor();
    int32_t start();
private:
    void task();
    void send_snd_dev_change_msg(int32_t dev);
    int32_t check_snd_dev();
    //int32_t check_usb_dev_ok();
    std::thread th_;
    bool quit_ = false; 
    std::condition_variable cv_;
    std::mutex mtx_;
    int32_t dev_last_;
};

#endif