
#include "cam_manager.h"
#include "usb_camera.h"
#include "inslog.h"
#include "usb_device.h"
#include "json_obj.h"
#include <fstream>
#include "usb_msg.h"
#include <unistd.h>
#include <future>

int cam_manager::power_on_all()
{
	LOGINFO("power on");
	system("power_manager power_on");
	usb_device::create();

	return INS_OK;
}

int cam_manager::power_off_all()
{
	LOGINFO("power off");
	system("power_manager power_off");

	while (usb_device::get() && !usb_device::get()->is_all_close())
	{
		usleep(50*1000);
		system("power_manager power_off");//调用一次power_off,usb可能掉不了电,会卡死在这,故每次循环都调用一次
	}
	
	usb_device::destroy();

	return INS_OK;
}

cam_manager::~cam_manager() 
{
	close_all_cam(); 
};

cam_manager::cam_manager()
{
	for (unsigned int i = 0; i < cam_num_; i++)
	{
		pid_.push_back(cam_num_-i);
	}

	master_index_ = 0;
}

bool cam_manager::is_all_open()
{
	return usb_device::get()->is_all_open();
}

bool cam_manager::is_all_close()
{
	return usb_device::get()->is_all_close();
}

int cam_manager::open_all_cam()
{
	std::vector<unsigned int> index;
	for (unsigned int i = 0; i < cam_num_; i++)
	{
		index.push_back(i);
	}

	return open_cam(index);
}

int cam_manager::open_cam(std::vector<unsigned int>& index)
{
	int ret = do_open_cam(index);
	if (ret != INS_OK)
	{
		close_all_cam();
		return INS_ERR_CAMERA_OPEN;
	}
	else
	{
		return INS_OK;
	}
}

int cam_manager::do_open_cam(std::vector<unsigned int>& index)
{
	if (!map_cam_.empty())
	{
		LOGERR("camera have been open");
		return INS_ERR;
	}

	for (unsigned int i = 0; i < index.size(); i++)
	{
		if (index[i] >= cam_num_) return INS_ERR;
	}

	//wait A12 power on
	bool b_all_open = true;
	int loop_cnt = 400;
	while (--loop_cnt > 0)
	{
		b_all_open = true;
		for (unsigned int i = 0; i < index.size(); i++)
		{
			if (!usb_device::get()->is_open(pid_[index[i]]))
			{
				b_all_open = false; 
				break;
			}
		}

		if (b_all_open) break;
		else usleep(50*1000);
	}

	if (!b_all_open)
	{
		LOGERR("not all camera open");
		return INS_ERR;
	}
	else
	{
		LOGINFO("all camera open");
	}

	usleep(1000*1000); //wait A12 init ...

	for (unsigned int i = 0; i < index.size(); i++)
	{
		auto cam = std::make_shared<usb_camera>(pid_[index[i]], index[i]);
		map_cam_.insert(std::make_pair(index[i], cam));
	}

	return INS_OK;
}

void cam_manager::close_all_cam()
{
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
	{
		it->second->close();
	}

	map_cam_.clear();
}

std::map<int32_t, int32_t> cam_manager::test_connection()
{
	std::map<int32_t, int32_t> map_ret;
	if (open_all_cam() != INS_OK)
	{
		for (int32_t i = 0; i < cam_num_; i++)
		{
			int32_t r = INS_ERR_CAMERA_OPEN;
			if (usb_device::get()->is_open(pid_[i])) r = INS_OK;
			map_ret.insert(std::make_pair(pid_[i], r)); 
		}
	}
	else
	{
		for (auto& it : map_cam_)
		{
			auto r = it.second->test_connection();
			map_ret.insert(std::make_pair(pid_[it.first], r));
		}
	}


	close_all_cam();

	return map_ret;
}

// int cam_manager::get_one_firmware_version(int index, std::vector<std::string>& version)
// {
// 	auto it = map_cam_.find(index);
// 	if (it == map_cam_.end()) return INS_ERR;

// 	int ret = it->second->get_fw_version();
// 	RETURN_IF_NOT_OK(ret);

// 	auto f = it->second->wait_cmd_over();
// 	ret = f.get();
// 	RETURN_IF_NOT_OK(ret);

// 	auto result = it->second->get_result();
// 	json_obj root(result.c_str());
// 	std::string tmp;
// 	root.get_string("version", tmp);
// 	version.push_back(tmp);
// 	LOGINFO("index:%d firmware version:%s", it->first, tmp.c_str());

// 	return INS_OK;
// }

// int cam_manager::get_one_version(int index, std::vector<std::string>& version)
// {
// 	auto it = map_cam_.find(index);
// 	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

// 	std::string ver;
// 	int ret = it->second->get_version(ver);
// 	RETURN_IF_NOT_OK(ret);
// 	version.push_back(ver);

// 	return INS_OK;
// }

int cam_manager::get_all_version(std::vector<std::string>& version)
{
	if (map_cam_.empty()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
	{
		std::string ver;
		auto r = it->second->get_version(ver);
		if (r != INS_OK) 
		{
			ret = r;
		}
		else
		{
			version.push_back(ver);
		}
	}
	
	return ret;
}

// int cam_manager::get_all_firmware_version(std::vector<std::string>& version)
// {
// 	if (map_cam_.empty()) return INS_ERR_CAMERA_NOT_OPEN;

// 	int result = INS_OK;
// 	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
// 	{
// 		int ret = it->second->get_fw_version();
// 	}

// 	std::vector<std::future<int32_t>> v_f;
// 	for (auto& it : map_cam_)
// 	{
// 		auto f = it.second->wait_cmd_over();
// 		v_f.push_back(std::move(f));
// 	}

// 	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
// 	{
// 		auto f = it->second->wait_cmd_over();
// 		auto ret = f.get();
// 		if (ret == INS_OK)
// 		{
// 			auto res = it->second->get_result();
// 			json_obj root(res.c_str());
// 			std::string tmp;
// 			root.get_string("version", tmp);
// 			version.push_back(tmp);
// 			LOGINFO("index:%d firmware version:%s", it->first, tmp.c_str());
// 		} 
// 		else
// 		{
// 			result = ret;
// 		}
// 	}

// 	return result;
// }

int cam_manager::test_spi(int index)
{
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = it->second->test_spi();
	RETURN_IF_NOT_OK(ret);

	auto f = it->second->wait_cmd_over();
	ret = f.get();
	RETURN_IF_NOT_OK(ret);

	auto result = it->second->get_result();
	json_obj root_obj(result.c_str());
	int value = 0;
	root_obj.get_int("value", value);

	if (value)
	{
		return INS_ERR;
	}
	else
	{
		return INS_OK;
	}
}

int32_t cam_manager::calibration_bpc_all()
{
	int32_t ret = INS_OK;
	std::future<int32_t> futures[INS_CAM_NUM];

	for (int32_t i = 0; i < INS_CAM_NUM; i++)
	{
		futures[i] = std::async(std::launch::async, [this, i]
		{
			auto it = map_cam_.find(i);
			if (it == map_cam_.end()) 
			{
				return INS_ERR;
			}
			else
			{
				return it->second->calibration_bpc_req();
			}
		});
	}

	for (int32_t i = 0; i < INS_CAM_NUM; i++)
	{
		auto r = futures[i].get();
		if (r != INS_OK) ret = r;
	}

	LOGINFO("bpc calibration result:%d", ret);

	return ret;
}

int cam_manager::calibration_blc_all(bool reset)
{
	int32_t ret = INS_OK;
	std::future<int32_t> futures[INS_CAM_NUM];

	for (int i = 0; i < INS_CAM_NUM; i++)
	{
		futures[i] = std::async(std::launch::async, [this, i, reset]
		{
			auto it = map_cam_.find(i);
			if (it == map_cam_.end())  return INS_ERR;
			if (reset)
			{
				return it->second->calibration_blc_reset();
			}
			else
			{
				return it->second->calibration_blc_req();
			}
		});	
	}

	for (int32_t i = 0; i < INS_CAM_NUM; i++)
	{
		auto r = futures[i].get();
		if (r != INS_OK) ret = r;
	}

	LOGINFO("blc calibration result:%d", ret);

	return ret;
}

int cam_manager::upgrade(std::string file_name, std::string version)
{
	int ret = INS_OK;
	std::string md5_value;
	ret = md5_file(file_name, md5_value);
	RETURN_IF_NOT_OK(ret);

	std::thread th[cam_num_];
	int result[cam_num_];
	for (unsigned int i = 0; i < cam_num_; i++)
	{
		th[i] = std::thread([this, i, &file_name, &result, &md5_value]()
		{	
			result[i] = INS_ERR;
			int loop = 3;//如果升级失败尝试3次
			while (loop-- > 0) 
			{
				if (INS_OK == map_cam_[i]->upgrade(file_name, md5_value)) 
				{
					result[i] = INS_OK;
					break;
				}
			}
		});
	}

	for (unsigned int i = 0; i < cam_num_; i++)
	{
		INS_THREAD_JOIN(th[i]);
		if (result[i] != INS_OK) ret = INS_ERR;
	}

	RETURN_IF_NOT_OK(ret);

	ret = reboot_all_camera();
	RETURN_IF_NOT_OK(ret);

	for (int i = 0; i < 3; i++)
	{
		ret = wait_reboot();
		if (ret != INS_OK)
		{
			power_off_all();
			usleep(50*1000);
			power_on_all();
		}
		else
		{
			break;
		}
	}

	RETURN_IF_NOT_OK(ret);

	usleep(100*1000);

	ret = check_fw_version(version);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t cam_manager::wait_reboot()
{
	int32_t loop_cnt = 500;
	bool exception = true;
	while (--loop_cnt > 0)
	{
		if (!is_all_open())
		{
			LOGINFO("module start reboot");
			exception = false;
			break;
		}

		usleep(50*1000);
	}

	if (exception) 
	{
		LOGERR("not all module left after reboot");
		return INS_ERR;
	}

	exception = true;
	loop_cnt = 800;
	while (--loop_cnt > 0)
	{
		if (is_all_open())
		{
			LOGINFO("all module reboot success");
			exception = false;
			break;
		}

		usleep(50*1000);
	}

	if (exception) 
	{
		LOGERR("not all module reboot success");
		return INS_ERR;
	}
	else
	{
		return INS_OK;
	}
}

int cam_manager::check_fw_version(std::string version)
{
	std::vector<std::string> v_version;
	get_all_version(v_version);
	for (unsigned int i = 0; i < v_version.size(); i++)
	{
		auto pos = v_version[i].find(".", 0);
		if (pos == std::string::npos) return INS_ERR;
		std::string tmp = v_version[i].substr(pos+1);
		if (version != tmp)
		{
			LOGERR("version check fail, module version:%d %s after upgrade != req version:%d %s", 
				tmp.length(), tmp.c_str(), version.length(), version.c_str());
			return INS_ERR;
		}
	}

	return INS_OK;
}

int cam_manager::reboot_all_camera()
{
	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
	{
		auto r = it->second->reboot();
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
	{
		auto f = it->second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f)
	{
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	return ret;
}

int cam_manager::md5_file(std::string file_name, std::string& md5_value)
{
    std::string tmp_file = "/tmp/.md5_result";
    std::string cmd = "md5sum -b " + file_name + " > " + tmp_file;
    LOGINFO("cmd:%s", cmd.c_str());
    system(cmd.c_str());

    std::ifstream file(tmp_file);
    if (!file.is_open())
    {
        LOGERR("file:%s open fail", tmp_file.c_str());
        return INS_ERR;
    }
    std::string line;
    getline(file, line);
    file.close();
    remove(tmp_file.c_str());

	auto pos = line.find(" ");
	md5_value = line.substr(0, pos);
	LOGINFO("file:%s out:%s md5:%s", file_name.c_str(), line.c_str(), md5_value.c_str());

    return INS_OK;
}

int cam_manager::get_uuid_sensorid(std::vector<std::string>& v_uuid, std::vector<std::string>& v_sensorid)
{
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
	{
		int ret = it->second->get_uuid_sensorid();
		RETURN_IF_NOT_OK(ret);
	}

	std::map<uint32_t, std::future<int32_t>> map_f;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
	{
		auto f = it->second->wait_cmd_over();
		map_f.insert(std::make_pair(it->first, std::move(f)));
	}

	int32_t ret = INS_OK;
	for (auto& it : map_f)
	{
		auto r = it.second.get();
		if (r != INS_OK) 
		{
			ret = r; continue;
		}
		std::string result = map_cam_[it.first]->get_result();
		json_obj root_obj(result.c_str());
		std::string uuid;
		std::string sensorid;
		root_obj.get_string("uuid", uuid);
		root_obj.get_string("sensorId", sensorid);
		if (uuid == "")
		{
			LOGERR("index:%d uuid is null", it.first);
			ret = INS_ERR; continue;
		}
		if (sensorid == "")
		{
			LOGERR("index:%d sensorid is null", it.first);
			ret = INS_ERR; continue;
		}

		v_uuid[it.first] = uuid;
		v_sensorid[it.first] = sensorid;
		LOGINFO("index:%d uuid:%s sensorid:%s", it.first, uuid.c_str(), sensorid.c_str());
	}

	return ret;
}

int cam_manager::send_awb_data(unsigned int index, std::string file_name)
{
	int ret = send_data(index, file_name, AMBA_FRAME_CB_AWB);
	if (ret == INS_OK)
	{
		LOGINFO("index:%d send awb data success", index);
	}
	else
	{
		LOGERR("index:%d send awb data fail", index);
	}
	return ret;
}

int cam_manager::send_lp_data(unsigned int index, std::string file_name)
{
	int ret = send_data(index, file_name, AMBA_FRAME_CB_BRIGHT);
	if (ret == INS_OK)
	{
		LOGINFO("index:%d send lp data success", index);
	}
	else
	{
		LOGERR("index:%d send lp data fail", index);
	}
	return ret;
}

int cam_manager::send_vig_maxvalue(unsigned int index, unsigned char* data, unsigned int size)
{
	int ret = send_buff(index, data, size, AMBA_FRAME_CB_VIG_MAXVALUE);
	if (ret == INS_OK)
	{
		LOGINFO("index:%d send vig maxvalue success", index);
	}
	else
	{
		LOGERR("index:%d send vig maxvalue fail", index);
	}
	return ret;
}

int cam_manager::send_data(unsigned int index, std::string file_name, int type)
{
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR;

	return it->second->send_calibration_data(file_name, type);
}

int cam_manager::send_buff(unsigned int index, unsigned char* data, unsigned int size, int type)
{
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR;

	return it->second->send_calibration_data(data, size, type);
}

int cam_manager::get_all_options(std::string property, std::vector<std::string>& v_value)
{
	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++)
	{
		std::string value;
		ret = it->second->get_options(property, value);
		BREAK_IF_NOT_OK(ret);
		v_value.push_back(value);
	}

	return ret;
}

