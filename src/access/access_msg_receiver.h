
#ifndef _ACCESS_CENTER_H_
#define _ACCESS_CENTER_H_

#include <thread>
#include <memory>
#include <deque>
#include <mutex>
#include "fifo_read.h"
#include "access_msg_buff.h"

class access_msg_receiver
{
public:
	~access_msg_receiver() { stop(); };
	int start(std::string name, std::function<void(const std::shared_ptr<access_msg_buff>)>& handler);
	void queue_msg(std::shared_ptr<access_msg_buff>& msg);

private:
	void stop();
	void copy_msg_buff(const char* buff, unsigned int size);
	void parse_msg(const char* buff, unsigned int size);
	std::shared_ptr<access_msg_buff> deque_msg();
	void task();

	std::shared_ptr<fifo_read> reader_;

	char* msg_buff_ = nullptr;
	unsigned int msg_buff_offset_ = 0;
	unsigned int msg_buff_size_ = 0;

	bool quit_ = false;
	std::thread th_;
	std::mutex mtx_;
	std::deque<std::shared_ptr<access_msg_buff>> queue_msg_;

	std::function<void(const std::shared_ptr<access_msg_buff>)> msg_handler_;
};


#endif