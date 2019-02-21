#ifndef _INS_TIMER_H_
#define _INS_TIMER_H_

#include "ev++.h"
#include <thread>

class ins_timer
{
public:
    ins_timer(std::function<void()>& cb, int duration = -1);
    ~ins_timer();
    void timer_cb(ev::timer &w, int e);
    void start(int duration) { duration_ = duration; };

private:
    void task();
    std::thread th_;
	ev::timer watcher_;
	ev::dynamic_loop loop_;
    int duration_ = -1;
    bool quit_ = false;
    std::function<void()> cb_;
};

#endif