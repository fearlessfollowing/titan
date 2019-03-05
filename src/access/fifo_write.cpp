
#include "fifo_write.h"
#include "inslog.h"
#include "common.h"
#include "json_obj.h"
#include "access_msg.h"
#include <unistd.h>
#include <linux/limits.h>
#include "ins_util.h"

int fifo_write::start(std::string name)
{
    name_ = name;
	th_ = std::thread(&fifo_write::task, this);

    return INS_OK;
}

void fifo_write::stop()
{
    if (quit_) {
		return;
	}

	quit_ = true;

	INS_THREAD_JOIN(th_);
}

void fifo_write::ev_io_w_cb(ev::io &w, int e)  
{
	while (1) {
		if (quit_) {
			ev_loop_.break_loop(ev::ALL);
			break;
		}

		auto msg = deque_msg();
		if (msg == nullptr) {
			usleep(20*1000);
			break;
		}

		unsigned offset = 0;
		bool b_success = true;
		int i = 0;
		
		while (offset < msg->size) {
			int size = std::min(msg->size - offset, (unsigned)PIPE_BUF);
			int ret = write(fd_, msg->buff + offset, size);
			if (ret <= 0) {
				if (errno == EAGAIN) {
					usleep(50*1000);
					continue;
				} else {
					LOGERR("write msg error size:%d ret:%d %d %s", size, ret, errno, strerror(errno));
					ev_loop_.break_loop(ev::ALL);
					b_success = false;
					break;
				}
			} else if (ret != size) {
				LOGERR("write msg ret:%d != size:%d", ret, size);
				queue_msg_to_front(msg);
				b_success = false;
				break;
			} else {
				//LOGINFO("write size:%d success", size);
				offset += size;
			}
		}

		if (!b_success) break;

		json_obj root(msg->content);
		std::string name;
		root.get_string(ACCESS_MSG_NAME, name);
		if (name == ACCESS_CMD_S_TASK_STATS_ && stats_msg_cnt_++%100) {	// s定时消息每隔100次写一次日志
			//LOGINFO("---- not print msg:%s", msg->content);
		} else if (name == ACCESS_CMD_S_QUERY_TASK_LIST && query_msg_cnt_++%100) {

		} else {
			LOGINFO("send msg seq:%u content len:%d content:%s", msg->sequence, msg->content_len, msg->content);
		}
	}
}

void fifo_write::task()
{
	while (!quit_) {
		ins_util::check_create_dir(name_);
		if (access(name_.c_str(), F_OK) == -1) {
			if (mkfifo(name_.c_str(), 0666)) {
				LOGINFO("make fifo:%s fail", name_.c_str());
				return;
			}
		}

        fd_ = open(name_.c_str(), O_WRONLY|O_NONBLOCK);
		if (fd_ == -1) {
			//LOGERR("to client fifo open fail");
			usleep(30*1000);
			continue;
		}

		LOGINFO("write fifo:%s open success fd:%d", name_.c_str(), fd_);

		ev_io_w_.set<fifo_write, &fifo_write::ev_io_w_cb>(this);
		ev_io_w_.set(ev_loop_);
		ev_io_w_.set(fd_, ev::WRITE);
		ev_io_w_.start();
    	ev_loop_.run();  
    	ev_io_w_.stop();
		close(fd_);
		fd_ = -1;
	}

	LOGINFO("write fifo:%s finish", name_.c_str());
}

void fifo_write::send_msg_sync(std::shared_ptr<access_msg_buff>& msg)
{
	std::lock_guard<std::mutex> lock(mtx_);

	int loopcnt = 0;
	while (++loopcnt < 300) {
		if (fd_ != -1) break;
		usleep(20*1000);
	}

	if (fd_ == -1) {
		LOGERR("send sync msg fail, fifo not open");
		return;
	}
	
	int ret = write(fd_, msg->buff, msg->size);
	if (ret < 0) {
		LOGERR("fifo sync send msg fail");
	} else {
		LOGINFO("sync send msg seq:%u content len:%d content:%s", msg->sequence, msg->content_len, msg->content);
	}
}

void fifo_write::queue_msg(std::shared_ptr<access_msg_buff>& msg)
{
	std::lock_guard<std::mutex> lock(mtx_);
	queue_msg_.push_back(msg);
}

void fifo_write::queue_msg_to_front(std::shared_ptr<access_msg_buff>& msg)
{
	std::lock_guard<std::mutex> lock(mtx_);
	queue_msg_.push_front(msg);
}

std::shared_ptr<access_msg_buff> fifo_write::deque_msg()
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (!queue_msg_.size()) {
		return nullptr;
	}

	auto msg = queue_msg_.front();
	queue_msg_.pop_front();
	return msg;
}