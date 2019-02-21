
#include "cam_manager.h"
#include "usb_device.h"
#include "inslog.h"
#include "common.h"
#include <fstream>
#include <sstream>
#include "json_obj.h"
#include <unistd.h>

void rsp_msg(int rsp_code, std::string result)
{
	if (result == "")
	{
		printf("{\"respone_code\":%d,\"description\":\"%s\"}\n", rsp_code, inserr_to_str(rsp_code).c_str());
		LOGINFO("{\"respone_code\":%d,\"description\":\"%s\"}", rsp_code, inserr_to_str(rsp_code).c_str());
	}
	else
	{
		printf("{\"respone_code\":%d,\"description\":\"%s\", \"results\":%s}\n", rsp_code, inserr_to_str(rsp_code).c_str(), result.c_str());
		LOGINFO("{\"respone_code\":%d,\"description\":\"%s\", \"results\":%s}", rsp_code, inserr_to_str(rsp_code).c_str(), result.c_str());
	}
}

void rsp_msg(int rsp_code)
{
	rsp_msg(rsp_code, "");
}

int get_uuid_sensorid(std::string& result)
{
	if (usb_device::create() == nullptr) return INS_ERR;

	auto camera = std::make_shared<cam_manager>();
	int ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	std::vector<std::string> v_uuid(INS_CAM_NUM);
	std::vector<std::string> v_sensorid(INS_CAM_NUM);
	ret = camera->get_uuid_sensorid(v_uuid, v_sensorid);
	RETURN_IF_NOT_OK(ret);

	std::stringstream ss;
	ss << "[";
	for (int i = 0; i < INS_CAM_NUM; i++)
	{
		ss << "{\"index\":" << i << ",\"uuid\":\"" << v_uuid[i] << "\",\"sensorId\":\"" <<  v_sensorid[i] << "\"}";
		if (i < INS_CAM_NUM-1) ss << ",";
	}
	ss << "]";

	result = ss.str();

	return INS_OK;
}

int set_calibration_data(const std::shared_ptr<json_obj>& param_obj)
{
	if (param_obj == nullptr) 
	{
		LOGERR("no parameters key");
		return INS_ERR_INVALID_MSG_FMT;
	}

	int index;
	param_obj->get_int("index", index);
	if (index < 0 || index > INS_CAM_NUM-1)
	{
		LOGERR("index:%d invalid", index);
		return INS_ERR_INVALID_MSG_PARAM;
	}

	std::string vig_path;
	std::string vig_max_value_array;
	param_obj->get_string("vig", vig_path);
	if (vig_path == "")
	{
		LOGERR("no vig path");
		return INS_ERR_INVALID_MSG_PARAM;
	}

	auto v_vig_max_value = param_obj->get_int_array("vig_max_value");
	if (v_vig_max_value.empty())
	{
		LOGERR("no vig max value");
		return INS_ERR_INVALID_MSG_PARAM;
	}

	LOGINFO("index:%d vig path:%s", index, vig_path.c_str());

	if (usb_device::create() == nullptr) return INS_ERR;

	auto camera = std::make_shared<cam_manager>();
	int ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	ret = camera->send_awb_data(index, vig_path.c_str());
	RETURN_IF_NOT_OK(ret);

	unsigned short* maxvalue = new unsigned short[v_vig_max_value.size()];
	for (unsigned int i = 0; i < v_vig_max_value.size(); i++)
	{
		maxvalue[i] = v_vig_max_value[i];
		LOGINFO("vig max value:%d", maxvalue[i]);
	}
	ret = camera->send_vig_maxvalue(index, (unsigned char*)maxvalue, sizeof(maxvalue));
	RETURN_IF_NOT_OK(ret);
	
	return INS_OK;
}

int test_module_communication()
{
	if (usb_device::create() == nullptr) return INS_ERR;
	auto camera = std::make_shared<cam_manager>();
	int ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	std::vector<std::string> v_version;
	ret = camera->get_all_version(v_version);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int test_module_spi()
{
	if (usb_device::create() == nullptr) return INS_ERR;
	auto camera = std::make_shared<cam_manager>();
	int ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	ret = camera->test_spi(camera->master_index());
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t test_connection(std::string& result)
{
	std::map<int32_t, int32_t> m_ret;
	if (usb_device::create() == nullptr)
	{
		for (int32_t i = 1; i < INS_CAM_NUM; i++)
		{
			m_ret.insert(std::make_pair(i, INS_ERR));
		}
	}
	else
	{
		auto camera = std::make_shared<cam_manager>();
		m_ret = camera->test_connection();
	}

	int32_t ret  = INS_OK;
	auto res_obj = json_obj::new_array();
	for (int32_t i = 1; i <= INS_CAM_NUM; i++)
	{
		auto it = m_ret.find(i);
		if (it != m_ret.end())
		{
			res_obj->array_add(it->second);
		}
		else
		{
			res_obj->array_add(INS_ERR);
		}
		if (it->second != INS_OK) ret = it->second;
	}

	result = res_obj->to_string();

	return ret;
}

int blc_calibration(bool reset)
{
	if (usb_device::create() == nullptr) return INS_ERR;
	auto camera = std::make_shared<cam_manager>();
	int ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	ret = camera->calibration_blc_all(reset);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int bpc_calibration()
{
	if (usb_device::create() == nullptr) return INS_ERR;
	auto camera = std::make_shared<cam_manager>();
	int ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	ret = camera->calibration_bpc_all();
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int query_storage(std::string& result)
{
	if (usb_device::create() == nullptr) return INS_ERR;
	auto camera = std::make_shared<cam_manager>();
	int ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	std::vector<std::string> v_value;
	ret = camera->get_all_options("storage_capacity", v_value); //获取模组存储容量
	RETURN_IF_NOT_OK(ret);

	json_obj obj;
	auto array = json_obj::new_array();
	for (auto it = v_value.begin(); it != v_value.end(); it++)
	{
		json_obj obj((*it).c_str());
		array->array_add(&obj);	
	}
	obj.set_obj("module", array.get());

	result = obj.to_string();

	return INS_OK;
}

int main(int argc, char* argv[])
{
	ins_log::init(INS_LOG_PATH, "log", false);

	if (argc < 2)
	{
		rsp_msg(INS_ERR_INVALID_MSG_FMT);
		LOGERR("please input json cmd");
		return INS_ERR;
	}

	LOGINFO("recv json cmd:%s", argv[1]);

	int ret;
	std::string cmd;
	json_obj root_obj(argv[1]);
	root_obj.get_string("name", cmd);
	
	if (cmd == "power_on")
	{
		cam_manager::power_off_all();
		usleep(5*1000);
		cam_manager::power_on_all();
		rsp_msg(INS_OK);
	}
	else if (cmd == "power_off")
	{
		cam_manager::power_off_all();
		rsp_msg(INS_OK);
	}
	else if (cmd == "get_uuid_sensorid")
	{
		std::string result;
		ret = get_uuid_sensorid(result);
		rsp_msg(ret, result);
	}
	else if (cmd == "set_calibration_data")
	{
		auto param_obj = root_obj.get_obj("parameters");
		ret = set_calibration_data(param_obj);
		rsp_msg(ret);
	}
	else if (cmd == "test_module_communication")
	{
		ret = test_module_communication();
		rsp_msg(ret);
	}
	else if (cmd == "test_module_spi")
	{
		ret = test_module_spi();
		rsp_msg(ret);
	}
	else if (cmd == "test_module_connection")
	{
		std::string result;
		ret = test_connection(result);
		rsp_msg(ret, result);
	}
	else if (cmd == "blc_calibration")
	{
		ret = blc_calibration(false);
		rsp_msg(ret);
	}
	else if (cmd == "bpc_calibration")
	{
		ret = bpc_calibration();
		rsp_msg(ret);
	}
	else if (cmd == "query_storage")
	{
		std::string result;
		ret = query_storage(result);
		rsp_msg(ret, result);
	}
	else
	{
		rsp_msg(INS_ERR_INVALID_MSG_FMT);
		LOGERR("cmd:%s unsupport", cmd.c_str());
	}

	return INS_OK;
}






