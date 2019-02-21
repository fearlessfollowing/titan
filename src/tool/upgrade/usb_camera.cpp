#include "stdio.h"
#include <sstream>
#include <future>
#include "usb_camera.h"
#include <fcntl.h>
#include "inslog.h"
#include "common.h"
#include "usb_msg.h"
#include "json_obj.h"
#include "usb_device.h"
#include <string.h>
#include <unistd.h>
#include "cam_manager.h"

#define SEND_CMD_TIMEOUT 3000
#define RECV_CMD_TIMEOUT 5000
#define RECV_VID_TIMEOUT 3000
#define RECV_PIC_TIMEOUT 70000
#define RECV_TIMELAPSE_TIMEOUT 50

void usb_camera::close()
{
	quit_ = true;
	usb_device::get()->cancle_transfer(pid_);
	INS_THREAD_JOIN(th_cmd_read_)
}

int32_t usb_camera::get_version(std::string& version)
{
	auto ret = send_cmd(USB_CMD_GET_SYSTEM_VERSION, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	json_obj obj(cmd_result_.c_str());
	obj.get_string("version", version);

	return INS_OK;
}

int usb_camera::reboot()
{
	return send_cmd(USB_CMD_REBOOT);
}

int usb_camera::get_uuid_sensorid()
{
	return send_cmd(USB_CMD_GET_UUID);
}

int usb_camera::test_spi()
{
	return send_cmd(USB_CMD_TEST_SPI);
}

int32_t usb_camera::test_connection()
{
	auto ret = send_cmd(USB_CMD_TEST_CONNECTION, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t usb_camera::calibration_bpc_req()
{
	int32_t ret = send_cmd(USB_CMD_CALIBRATION_BPC_REQ);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(2*60*1000, USB_CMD_CALIBRATION_BPC_IND); //2min
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t usb_camera::calibration_blc_req()
{
	int32_t ret = send_cmd(USB_CMD_CALIBRATION_BLC_REQ);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(4*60*1000, USB_CMD_CALIBRATION_BLC_IND);//4min
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t usb_camera::calibration_blc_reset()
{
	int32_t ret = send_cmd(USB_CMD_CALIBRATION_BLC_RESET, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t usb_camera::get_options(std::string property, std::string& value)
{
	int32_t ret;

	std::stringstream ss;
	ss << "{\"property\":\"" << property << "\"}";

	if (property == "storage_capacity")
	{
		ret = send_cmd(USB_CMD_GET_CAMERA_PARAM, ss.str());
		RETURN_IF_NOT_OK(ret);
	}
	else 
	{
		LOGERR("get invalid option:%s", property.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	if (property == "storage_capacity")
	{
		json_obj obj(cmd_result_.c_str());
		obj.set_int("index", pid_);
		value = obj.to_string();
		//LOGINFO("pid:%d %s:%s", pid_, property.c_str(), value.c_str());
	}
	else
	{	
		value = cmd_result_;
		//LOGINFO("pid:%d %s:%s", pid_, property.c_str(), value.c_str());
	}

	return INS_OK;
}

int usb_camera::send_calibration_data(const std::string& file_name, int type)
{
	int ret = send_file(file_name, type);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(10*1000, USB_CMD_SEND_DATA_RESULT_IND);//4min
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int usb_camera::send_calibration_data(unsigned char* data, unsigned int size, int type)
{
	int ret = send_buff(data, size, type);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(10*1000, USB_CMD_SEND_DATA_RESULT_IND);//4min
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

std::string usb_camera::get_result()
{
	return cmd_result_;
}

std::future<int32_t> usb_camera::wait_cmd_over()
{
	return std::async(std::launch::async, &usb_camera::do_wait_cmd_over, this);
}

int32_t usb_camera::do_wait_cmd_over()
{
	//表示发送命令失败或者没有发送命令，不用去等待响应
	if (send_cmd_ == -1) return INS_OK;

	return read_cmd_rsp(RECV_CMD_TIMEOUT);
}

void usb_camera::read_cmd_task()
{
	while (!quit_)
	{
		read_cmd(0); //无限长
	}

	LOGINFO("pid:%d cmd read task exit", pid_);
}

int32_t usb_camera::read_cmd_rsp(int32_t timeout, int32_t cmd)
{
	uint32_t rsp_cmd = (cmd != -1)?cmd:send_cmd_;
	int32_t loop_cnt = timeout/30;

	while (!quit_ && loop_cnt-- > 0)
	{
		mtx_cmd_rsp_.lock();
		auto it = cmd_rsp_map_.find(rsp_cmd);
		if (it == cmd_rsp_map_.end())
		{
			mtx_cmd_rsp_.unlock();
			usleep(30*1000);
			continue;
		}
		else
		{
			auto ret = it->second;
			cmd_rsp_map_.erase(it);
			mtx_cmd_rsp_.unlock();
			return ret;
		}
	}

	LOGERR("pid:%d read cmd:%x rsp timeout", pid_, rsp_cmd);

	return INS_ERR_CAMERA_READ_CMD;
}

int32_t usb_camera::read_cmd(int32_t timeout)
{
	if (rebooting_) 
	{
		usleep(20*1000);
		return INS_OK;
	}

	char buff[USB_MAX_CMD_SIZE];
	int32_t ret = usb_device::get()->read_cmd(pid_, buff, USB_MAX_CMD_SIZE, timeout);
	if (ret != LIBUSB_SUCCESS)
	{
		if (ret != LIBUSB_ERROR_CANCLE) 
		{
			LOGERR("pid:%d usb read cmd fail", pid_);
		}
		return INS_ERR_CAMERA_READ_CMD;
	}

	int32_t rsp_cmd = 0;
	int32_t rsp_result = 0;
	json_obj root_obj(buff);
	root_obj.get_int("type", rsp_cmd);
	root_obj.get_int("code", rsp_result);
	if (rsp_result >= INS_OK) rsp_result = INS_OK;

	LOGINFO("pid:%d recv cmd:%x respone:%s code:%d", pid_, rsp_cmd, buff, rsp_result); 

	cmd_result_ = "";
	root_obj.get_string("data", cmd_result_);
	if (rsp_result == INS_OK) //success
	{
		root_obj.get_string("data", cmd_result_);
		if (rsp_cmd == USB_CMD_REBOOT) rebooting_ = true;
	}

	mtx_cmd_rsp_.lock();
	cmd_rsp_map_.insert(std::make_pair(rsp_cmd, rsp_result));
	mtx_cmd_rsp_.unlock();

	return INS_OK;
}

int32_t usb_camera::send_cmd(uint32_t cmd, std::string content)
{	
	char buff[USB_MAX_CMD_SIZE] = {0};

	if (content.length() > USB_MAX_CMD_SIZE - 100)
	{
		LOGERR("content:%s too long", content.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	rebooting_ = false;

	if (content.length())
	{
		snprintf(buff, USB_MAX_CMD_SIZE, "{%s:%u,%s:%s}", USB_MSG_KEY_CMD, cmd, USB_MSG_KEY_DATA, content.c_str());
	}
	else
	{
		snprintf(buff, USB_MAX_CMD_SIZE, "{%s:%u}", USB_MSG_KEY_CMD, cmd);
	}

	LOGINFO("pid:%d send cmd:%x %s", pid_, cmd, buff);

	send_cmd_ = cmd;
	cmd_result_ = "";
	mtx_cmd_rsp_.lock();
	cmd_rsp_map_.erase(send_cmd_);
	mtx_cmd_rsp_.unlock();

	int32_t ret = usb_device::get()->write_cmd(pid_, buff, strlen(buff)+1, SEND_CMD_TIMEOUT);
	if (ret != LIBUSB_SUCCESS)
	{
		return INS_ERR;
	}
	else
	{
		return INS_OK;
	}
}

int usb_camera::send_data(unsigned char* data, unsigned int size)
{
	return usb_device::get()->write_data(pid_, data, size, SEND_CMD_TIMEOUT);
}

int usb_camera::send_buff(unsigned char* data, unsigned int size, int type)
{
	unsigned char* buff = new unsigned char[size+sizeof(amba_frame_info)]();
	amba_frame_info* frame = (amba_frame_info*)(buff);
	frame->syncword = -1;
	frame->type = type;
	frame->sequence = 1;
	frame->timestamp = 2;
	frame->size = size;
	memcpy(buff+sizeof(amba_frame_info), data, size);

	int ret = send_data(buff, size+sizeof(amba_frame_info));
	if (ret != INS_OK)
	{
		LOGERR("pid:%d send buff fail", pid_);
	}
	else
	{
		LOGINFO("pid:%d send buff success", pid_);
	}

	INS_DELETE_ARRAY(buff);
	return ret;
}

//六个摄像头必须同时升级
int usb_camera::upgrade(std::string file_name, const std::string& md5)
{
	int ret = send_cmd(USB_CMD_UPDATE_SYSTEM_START);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = send_file(file_name, AMBA_FRAME_UP_DATA, md5.c_str());

	std::string result;
	if (ret == INS_OK)
	{
		result = "{\"state\":\"done\"}";
	}
	else
	{
		result = "{\"state\":\"error\"}";
	}

	ret = send_cmd(USB_CMD_UPDATE_SYSTEM_COMPLETE, result);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int usb_camera::send_file(const std::string& file_name, int type, const std::string md5)
{
	LOGINFO("pid:%d send data begin", pid_);

	FILE* fp = fopen(file_name.c_str(), "rb");
	if (fp == nullptr)
	{
		LOGERR("file:%s open fail", file_name.c_str());
		return INS_ERR_FILE_OPEN;
	}

	int ret = INS_OK;
	unsigned int size = 51200;
	unsigned char* buff = new unsigned char[size+sizeof(amba_frame_info)]();
	unsigned char* data = buff+sizeof(amba_frame_info);
	amba_frame_info* frame = (amba_frame_info*)(buff);
	frame->syncword = -1;
	frame->type = type;
	frame->sequence = 1;
	while (!feof(fp))
	{	
		int len = fread(data, 1, size, fp);
		if (len < 0) 
		{
			LOGERR("pid:%d file read errror", pid_);
			ret = INS_ERR_FILE_IO;
			break;
		}
		if (len == 0) break;

		if (feof(fp)) frame->timestamp = 2;
		frame->size = len;
		ret = send_data(buff, len+sizeof(amba_frame_info));
		if (ret != INS_OK) break;
		frame->sequence++;
	}

	if (ret == INS_OK)
	{
		LOGINFO("pid:%d send data over", pid_);

		if (md5 != "")
		{
			LOGINFO("pid:%d send md5 begin lenght:%lu", pid_, md5.length());
			frame->size = md5.length();
			frame->type = AMBA_FRAME_UP_MD5;
			memcpy(data, md5.c_str(),md5.length());
			ret = send_data(buff, md5.length()+sizeof(amba_frame_info));
			if (ret == INS_OK)
			{
				LOGINFO("pid:%d send md5 over", pid_);
			}
			else
			{
				LOGINFO("pid:%d send md5 error", pid_);
			}
		}
	}
	else
	{
		LOGINFO("pid:%d send data error", pid_);
	}

	fclose(fp);
	INS_DELETE_ARRAY(buff);

	return ret;
}



