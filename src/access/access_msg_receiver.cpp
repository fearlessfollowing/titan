
#include "access_msg_receiver.h"
#include "inslog.h"
#include "common.h"
#include <unistd.h>

void access_msg_receiver::stop()
{
	quit_ = true;
	INS_THREAD_JOIN(th_);

	reader_ = nullptr;

	INS_DELETE_ARRAY(msg_buff_);
	msg_buff_offset_ = 0;
	msg_buff_size_ = 0;
}

int access_msg_receiver::start(std::string name, std::function<void(const std::shared_ptr<access_msg_buff>)>& handler)
{
	msg_handler_ = handler;

	auto func = [this](const char* buff, unsigned int size)
	{
		parse_msg(buff, size);
	};

	reader_ = std::make_shared<fifo_read>();
	if (reader_->start(name, func)) return INS_ERR;

	th_ = std::thread(&access_msg_receiver::task, this);

	return INS_OK;
}

void access_msg_receiver::parse_msg(const char* buff, unsigned int size)
{
	copy_msg_buff(buff, size);

	while (msg_buff_offset_ >= sizeof(access_msg_head))
	{
		access_msg_head* head = (access_msg_head*)msg_buff_;
		unsigned sequece = ntohl(head->sequece);
		unsigned content_len = ntohl(head->content_len);
		unsigned int msg_len = content_len + sizeof(access_msg_head);
		//LOGINFO("recv msg, sequece:%u content len:%u", head->sequece, head->content_len);

		if (msg_buff_offset_ < msg_len)
		{
			LOGINFO("msg buff size:%d < msg len:%d", msg_buff_offset_, msg_len);
			break;
		}

		head->sequece = sequece;
		head->content_len = content_len;
		auto msg = std::make_shared<access_msg_buff>(head->sequece, msg_buff_ + sizeof(access_msg_head), head->content_len);
		queue_msg(msg);

		msg_buff_offset_ -= msg_len;
		if (msg_buff_offset_)
		{
			memmove(msg_buff_, msg_buff_+msg_len, msg_buff_offset_);
		}
	}
}

void access_msg_receiver::copy_msg_buff(const char* buff, unsigned int size)
{
	if (size + msg_buff_offset_ > msg_buff_size_)
	{
		msg_buff_size_ = ((size + msg_buff_offset_)/512 + 1)*512;
		char* new_buff = new char[msg_buff_size_]();
		if (msg_buff_)
		{
			memcpy(new_buff, msg_buff_, msg_buff_offset_);
			delete[] msg_buff_;
		}
		msg_buff_ = new_buff;
		//LOGINFO("new msg buff:%u", msg_buff_size_);
	}

	memcpy(msg_buff_+msg_buff_offset_, buff, size);
	msg_buff_offset_ += size;
}

void access_msg_receiver::task()
{
	while (!quit_)
	{
		auto msg = deque_msg();
		if (msg == nullptr)
		{
			usleep(30000);
			continue;
		}
		else
		{
			msg_handler_(msg);
		}
	}
}

void access_msg_receiver::queue_msg(std::shared_ptr<access_msg_buff>& msg)
{
	std::lock_guard<std::mutex> lock(mtx_);
	queue_msg_.push_back(msg);
}

std::shared_ptr<access_msg_buff> access_msg_receiver::deque_msg()
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (!queue_msg_.size())
	{
		return nullptr;
	}

	auto msg = queue_msg_.front();
	queue_msg_.pop_front();

	return msg;
}
