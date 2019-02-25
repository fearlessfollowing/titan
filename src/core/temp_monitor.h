#ifndef _TEMPERATURE_MONITOR_H_
#define _TEMPERATURE_MONITOR_H_

#include <thread>
#include <condition_variable>
#include <mutex>

class ins_battery;

class temp_monitor {
public:
        temp_monitor(std::function<bool()>& cb);
        ~temp_monitor();

private:
    void task();
    //std::shared_ptr<ins_battery> battery_;
    bool                    quit_ = false;
    std::thread             th_;
    std::mutex              mtx_;
    std::condition_variable cv_;
    std::function<bool()>   need_report_temp_cb_;
};

#endif