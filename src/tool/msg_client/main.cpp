
#include <iostream>
#include "inslog.h"
#include "fifo_read.h"
#include "fifo_write.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include <signal.h>

// bool _quit = false;

// void sig_handle(int sig)
// {
//     _quit = true;
// }

int main(int argc, char const *argv[])
{
	std::function<void(const char*, unsigned int)> func = [](const char* buff, unsigned int size)
	{
		access_msg_head* head = (access_msg_head*)buff;
		head->sequece = ntohl(head->sequece);
		head->content_len = ntohl(head->content_len);
		LOGINFO("sequece:%u content:%s", head->sequece, buff + sizeof(access_msg_head));
	};

	ins_log::init(INS_LOG_PATH, "client_log");

	auto read_rsp = std::make_shared<fifo_read>();
	if (read_rsp->start(INS_FIFO_TO_CLIENT, func)) return -1;
	
	auto read_ind = std::make_shared<fifo_read>();
	if (read_ind->start(INS_FIFO_TO_CLIENT_A, func)) return -1;

	auto read_father = std::make_shared<fifo_read>();
	if (read_father->start(INS_FIFO_TO_CLIENT_FATHER, func)) return -1;

	auto write_req = std::make_shared<fifo_write>();
	if (write_req->start(INS_FIFO_TO_SERVER)) return -1;

	auto write_father = std::make_shared<fifo_write>();
	if (write_father->start(INS_FIFO_TO_SERVER_FATHER)) return -1;

	// struct sigaction sig_action;
    // sig_action.sa_handler = sig_handle;
	// sigaction(SIGINT, &sig_action, nullptr);
    // sigaction(SIGQUIT, &sig_action, nullptr);
    // sigaction(SIGTERM, &sig_action, nullptr);

	//带参数
	if (argc > 1)
	{
		std::string content = argv[1];
		LOGINFO("input msg:%s", content.c_str());
		auto msg = std::make_shared<access_msg_buff>(0, content.c_str(), content.length());
		write_req->queue_msg(msg);
		return 0;
	}

	unsigned int sequence = 0;
	std::string content;
	while (1)
	{
		getline(std::cin, content);
		if (content == "q")
		{
			break;
		}

		if (content.length() <= 5)
		{
			continue;
		}

		auto msg = std::make_shared<access_msg_buff>(sequence++, content.c_str(), content.length());

		if (content.find("camera._reset", 0) == std::string::npos)
		{
			write_req->queue_msg(msg);
		}
		else
		{
			write_father->queue_msg(msg);
		}
	}

	return 0;
}