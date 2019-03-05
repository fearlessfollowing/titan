#ifndef _ACCESS_MSG_BUFF_H_
#define _ACCESS_MSG_BUFF_H_

#include "access_msg.h"
#include <arpa/inet.h>

/*
 * access_msg_buff - 消息对象
 */
struct access_msg_buff {

	char* buff = nullptr;
	unsigned int size = 0;
	unsigned int sequence = 0;
	unsigned int content_len = 0;
	char* content = nullptr;

	access_msg_buff(unsigned int seq, const char* msg, unsigned int len) {
		size = sizeof(access_msg_head) + len;
		buff = new char[size+1](); //多一个字节来表示字符串结尾，便于最后字符串打印
		sequence = seq;
		content_len = len;

		auto msg_head = (access_msg_head*)buff;
		msg_head->sequece = htonl(sequence);
		msg_head->content_len = htonl(len);

		content = buff + sizeof(access_msg_head);
		memcpy(content, msg, len);
	};

	~access_msg_buff() {
		if (buff) {
			delete[] buff;
			buff = nullptr;
			size = 0;
			sequence = 0;
			content_len = 0;
			content = nullptr;
		}
	};
};

#endif