#ifndef _HDMI_MONITOR_H_
#define _HDMI_MONITOR_H_

#include <thread>
#include <atomic>

class hdmi_monitor {
public:
            ~hdmi_monitor();
    int32_t start();
    
    
    static std::atomic_bool audio_init_;

private:
    void    task();
    // int32_t epfd_ = -1;
    // int32_t hdmifd_ = -1;
    bool        quit_ = false;
    std::thread th_;
    std::string state_;
};

#endif