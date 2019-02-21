
#include "ins_timer.h"
#include "inslog.h"
#include "common.h"

ins_timer::~ins_timer()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
    LOGINFO("timer destroy");
}

ins_timer::ins_timer(std::function<void()>& cb, int duration) 
    : duration_(duration)
    , cb_(cb)
{
    th_ = std::thread(&ins_timer::task, this);
}

void ins_timer::timer_cb(ev::timer &w, int e)
{
    if (quit_) loop_.break_loop(ev::ALL);

    if (duration_ < 0) return; //表示定时器没有启动

    if (--duration_ == 0)
    {
        cb_();
        //loop_.break_loop(ev::ALL);
    }
}

void ins_timer::task()
{
    watcher_.set<ins_timer,&ins_timer::timer_cb>(this);
    watcher_.set(loop_);
    watcher_.start(1, 1); //1s
    loop_.run(); 
    watcher_.stop();
}