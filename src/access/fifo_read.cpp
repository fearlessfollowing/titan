
#include "fifo_read.h"
#include "inslog.h"
#include "common.h"
#include "access_msg_buff.h"
#include <unistd.h>
#include <linux/limits.h>
#include "ins_util.h"

void fifo_read::stop()
{
	if (quit_) return;
	quit_ = true;
	ev_async_.send();
	INS_THREAD_JOIN(th_);
}

int fifo_read::start(std::string name, std::function<void(const char*, unsigned int)>handle_read_data)
{
    name_ = name;
    handle_read_data_ = handle_read_data;

	th_ = std::thread(&fifo_read::task, this);

	return INS_OK;
}

void fifo_read::ev_async_cb(ev::async &w, int e)  
{
	ev_loop_.break_loop(ev::ALL);
}

void fifo_read::ev_io_cb(ev::io &w, int e)  
{
	while (!quit_)
	{
		char buff[PIPE_BUF] = {0};
		int ret = read(fd_,buff,sizeof(buff));
		if (ret <= 0)
		{
			if (errno == EAGAIN && ret != 0)
			{
				//LOGINFO("fifo read eagain");
				usleep(30*1000);
				continue;
			}
			else
			{
				LOGINFO("fifo read ret:%d errno:%d %s, restart read-fifo", ret, errno, strerror(errno));
				nofity_fifo_close();
				ev_loop_.break_loop(ev::ALL);
				break;
			}
		}
		else
		{
			//LOGINFO("fifo read size:%d", ret);
			handle_read_data_(buff, ret);
		}
	}
}

void fifo_read::task()
{
	while (!quit_)
	{
		ins_util::check_create_dir(name_);
		if (access(name_.c_str(), F_OK) == -1)
		{
			if (mkfifo(name_.c_str(), 0666))
			{
				LOGINFO("make fifo:%s fail", name_.c_str());
				return;
			}
		}

        fd_ = open(name_.c_str(), O_RDONLY|O_NONBLOCK);
		if (fd_ == -1)
		{
			LOGERR("read fifo:%s open fail", name_.c_str());
			return;
		}

		LOGINFO("read fifo:%s open success fd:%d", name_.c_str(), fd_);

		ev_io_.set<fifo_read,&fifo_read::ev_io_cb>(this);
		ev_io_.set(ev_loop_);
		ev_io_.set(fd_, ev::READ);
		ev_io_.start();
		ev_async_.set<fifo_read, &fifo_read::ev_async_cb>(this);
		ev_async_.set(ev_loop_);
		ev_async_.start();
        ev_loop_.run();  
        ev_io_.stop();
        ev_async_.stop();
        close(fd_);
		fd_ = -1;
	}

	LOGINFO("read fifo:%s task finish", name_.c_str());
}
 
void fifo_read::nofity_fifo_close()
{
	std::string content = std::string("{\"name\":\"") + INTERNAL_CMD_FIFO_CLOSE + "\"}";
	auto msg = std::make_shared<access_msg_buff>(0, content.c_str(), content.length());
	handle_read_data_(msg->buff, msg->size);
}

