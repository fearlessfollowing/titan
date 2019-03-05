
#include "access_msg_sender.h"
#include "common.h"
#include "inslog.h"
#include <sstream>
#include <string>
#include "json_obj.h"




/***********************************************************************************************
** 函数名称: setup
** 函数功能: 创建消息发送器(同步和异步)
** 入口参数:
** 返 回 值: 成功返回INS_OK
*************************************************************************************************/
int access_msg_sender::setup()
{
	msg_sender_ = std::make_shared<fifo_write>();
	if (msg_sender_->start(INS_FIFO_TO_CLIENT)) 
		return INS_ERR;

	msg_sender_a_ = std::make_shared<fifo_write>();
	if (msg_sender_a_->start(INS_FIFO_TO_CLIENT_A)) 
		return INS_ERR;

	return INS_OK;
}



/***********************************************************************************************
** 函数名称: re_setup
** 函数功能: 重新初始化消息发送器(同步和异步)
** 入口参数:
** 返 回 值: 无
*************************************************************************************************/
void access_msg_sender::re_setup()
{
	msg_sender_ = nullptr;
	msg_sender_a_ = nullptr;

	msg_sender_ = std::make_shared<fifo_write>();
	msg_sender_->start(INS_FIFO_TO_CLIENT);

	msg_sender_a_ = std::make_shared<fifo_write>();
	msg_sender_a_->start(INS_FIFO_TO_CLIENT_A);
}



/***********************************************************************************************
** 函数名称: send_rsp_msg
** 函数功能: 发送响应消息
** 入口参数:
**		sequence - 序列值
**		rsp_code - 响应码(用来区分响应的状态)
**		cmd		 - 响应的命令(对应请求)
**		res_obj	 - results
**		b_sync	 - 同步还是异步发送
** 返 回 值: 无
*************************************************************************************************/
void access_msg_sender::send_rsp_msg(unsigned int sequence, int rsp_code, const std::string& cmd, const json_obj* res_obj, bool b_sync) const
{
	json_obj root_obj;
	root_obj.set_string("name", cmd);
	root_obj.set_int("sequence", sequence);

	if (rsp_code == INS_OK) {
		root_obj.set_string("state", "done");
	} else {
		json_obj obj;
		obj.set_int("code", rsp_code);
		auto err_str = inserr_to_str(rsp_code);
		obj.set_string("description", err_str);
		root_obj.set_string("state", "error");
		root_obj.set_obj("error", &obj);
	}

	if (res_obj) 
		root_obj.set_obj("results", res_obj);

	auto content = root_obj.to_string();
	
	auto msg = std::make_shared<access_msg_buff>(sequence, content.c_str(), content.length());
	if (b_sync) {
		msg_sender_->send_msg_sync(msg);
	} else {
		msg_sender_->queue_msg(msg);
	}
}



/***********************************************************************************************
** 函数名称: send_ind_msg
** 函数功能: 发送指示消息
** 入口参数:
**		cmd		 - 响应的命令(对应请求)
**		rsp_code - 响应码(用来区分响应的状态)
**		res_obj	 - results
** 返 回 值: 无
*************************************************************************************************/
void access_msg_sender::send_ind_msg(std::string cmd, int rsp_code, const json_obj* res_obj)
{	
	auto sequence = get_ind_msg_sequece(cmd);
	json_obj root_obj;
	root_obj.set_string("name", cmd);
	root_obj.set_int("sequence", sequence);
	json_obj param_obj;

	if (rsp_code == INS_OK) {
		param_obj.set_string("state", "done");
	} else {
		json_obj obj;
		obj.set_int("code", rsp_code);
		auto err_str = inserr_to_str(rsp_code);
		obj.set_string("description", err_str);
		param_obj.set_string("state", "error");
		param_obj.set_obj("error", &obj);
	}

	if (res_obj) 
		param_obj.set_obj("results", res_obj);

	root_obj.set_obj(ACCESS_MSG_PARAM, &param_obj);

	auto content = root_obj.to_string();

	auto msg = std::make_shared<access_msg_buff>(sequence_++, content.c_str(), content.length());
	msg_sender_a_->queue_msg(msg);
}



void access_msg_sender::send_ind_msg_(std::string cmd, const json_obj* param)
{	
	auto sequence = get_ind_msg_sequece(cmd);

	json_obj root_obj;
	root_obj.set_string("name", cmd);
	root_obj.set_int("sequence", sequence);
	root_obj.set_obj(ACCESS_MSG_PARAM, param);
	auto content = root_obj.to_string();

	auto msg = std::make_shared<access_msg_buff>(sequence_++, content.c_str(), content.length());
	msg_sender_a_->queue_msg(msg);
}



/***********************************************************************************************
** 函数名称: set_ind_msg_sequece
** 函数功能: 设置指示消息的sequence
** 入口参数:
**		cmd		 - 响应的命令(对应请求)
**		sequence - 序列值
** 返 回 值: 无
*************************************************************************************************/
void access_msg_sender::set_ind_msg_sequece(std::string cmd, unsigned int sequence)
{
	if (map_msg_seq_.count(cmd) > 0) {
		map_msg_seq_[cmd] = sequence;
	} else {
		map_msg_seq_.insert(std::make_pair(cmd, sequence));
	}
}


/***********************************************************************************************
** 函数名称: get_ind_msg_sequece
** 函数功能: 获取指定命令的sequence
** 入口参数:
**		cmd		 - 响应的命令(对应请求)
** 返 回 值: 存在返回对应的sequence; 否则返回0
*************************************************************************************************/
unsigned int access_msg_sender::get_ind_msg_sequece(std::string cmd)
{
	if (map_msg_seq_.count(cmd) > 0) {
		return map_msg_seq_[cmd];
	} else {
		return 0;
	}
}

