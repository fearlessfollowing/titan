#ifndef _FIFO_READ_H_
#define _FIFO_READ_H_

#include <string>
#include <thread>
#include <fcntl.h>
#include "ev++.h"

class fifo_read
{
public:
    ~fifo_read() {stop();};
    int start(std::string name, std::function<void(const char*, unsigned int)>handle_read_data);

    void ev_io_cb(ev::io &w, int e);
	void ev_async_cb(ev::async &w, int e);

private:
    void stop();
    void task();
    void nofity_fifo_close();

    std::string name_;
    int fd_ = -1;
	bool quit_ = false;
	std::thread th_;
	ev::async ev_async_;
	ev::io ev_io_;
	ev::dynamic_loop ev_loop_;
    std::function<void(const char*, unsigned int)> handle_read_data_;
};

#endif