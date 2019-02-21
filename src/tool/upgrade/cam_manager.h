
#ifndef _CAM_MANAGER_H_
#define _CAM_MANAGER_H_
#include <vector>
#include <thread>
#include <map>
#include <string>
#include "common.h"

class usb_camera;

class cam_manager
{
public:
	static int32_t power_on_all();
	static int32_t power_off_all();

	cam_manager();
	~cam_manager();
	int32_t open_all_cam();
	void close_all_cam();
	int32_t open_cam(std::vector<uint32_t>& index);
	int32_t upgrade(std::string file_name, std::string version);
	//int32_t get_one_version(int32_t index, std::vector<std::string>& version);
	int32_t get_all_version(std::vector<std::string>& version);
	int32_t send_awb_data(uint32_t index, std::string file_name);
	int32_t send_lp_data(uint32_t index, std::string file_name);
	int32_t send_vig_maxvalue(uint32_t index, uint8_t* data, uint32_t size);
	int32_t get_uuid_sensorid(std::vector<std::string>& v_uuid, std::vector<std::string>& v_sensorid);
	int32_t test_spi(int32_t index);
	int32_t calibration_bpc_all();
	int32_t calibration_blc_all(bool reset);
	std::map<int32_t, int32_t> test_connection();
	int32_t get_all_options(std::string property, std::vector<std::string>& v_value);
	int32_t master_index() { return master_index_; };
	
private:
	bool is_all_open();
	bool is_all_close();
	int32_t do_open_cam(std::vector<uint32_t>& index);
	int32_t reboot_all_camera();
	int32_t check_fw_version(std::string version);
	int32_t md5_file(std::string file_name, std::string& md5_value);
	int32_t send_data(uint32_t index, std::string file_name, int32_t type);
	int32_t send_buff(uint32_t index, uint8_t* data, uint32_t size, int32_t type);
	int32_t wait_reboot();
	const uint32_t cam_num_ = INS_CAM_NUM;
	uint32_t vid_ = 0x4255;
	std::vector<uint32_t> pid_;
	std::map<uint32_t, std::shared_ptr<usb_camera>, std::greater<uint32_t>> map_cam_;
	int32_t master_index_ = 0;
};

#endif
