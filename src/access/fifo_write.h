#ifndef _FIFO_WIRTE_H_
#define _FIFO_WIRTE_H_

#include <memory>
#include <deque>
#include <thread>
#include <mutex>
#include <string>
#include <fcntl.h>
#include "ev++.h"
#include "access_msg_buff.h"

class fifo_write
{
public:
    ~fifo_write() {stop();};
    int start(std::string name);
    void queue_msg(std::shared_ptr<access_msg_buff>& msg);
    void send_msg_sync(std::shared_ptr<access_msg_buff>& msg);

    void ev_io_w_cb(ev::io &w, int e);

private:
    void stop();
	void queue_msg_to_front(std::shared_ptr<access_msg_buff>& msg);
	std::shared_ptr<access_msg_buff> deque_msg();
    void task();

    std::string name_;
    bool quit_ = false;
    ev::io ev_io_w_;
	ev::dynamic_loop ev_loop_;
    int fd_ = -1;
    std::thread th_;
    std::mutex mtx_;

    std::deque<std::shared_ptr<access_msg_buff>> queue_msg_;

    long long stats_msg_cnt_ = 0;
    long long query_msg_cnt_ = 0;
};

#endif