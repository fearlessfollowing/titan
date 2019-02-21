
#ifndef _CAM_MANAGER_H_
#define _CAM_MANAGER_H_
#include <vector>
#include <thread>
#include <map>
#include <future>
#include <condition_variable>
#include "all_cam_queue_i.h"
#include "cam_data.h"
#include "all_cam_video_buff.h"
#include "cam_img_repo.h"
#include "common.h"
#include "insbuff.h"

class usb_camera;

class cam_manager
{
public:
	static int32_t power_on_all();
	static int32_t power_off_all();
	static int32_t nv_amba_delta_usec() { return nv_amba_delta_usec_; };
	static std::mutex mtx_;

	cam_manager();
	~cam_manager();
	bool is_all_open();
	bool is_all_close();
	int32_t open_all_cam();
	void close_all_cam();
	int32_t exception();
	int32_t open_cam(std::vector<uint32_t>& index);
	int32_t start_video_rec(uint32_t index, const cam_video_param& param, const cam_video_param* sec_param, const std::shared_ptr<cam_video_buff_i>& queue);
	int32_t stop_video_rec(uint32_t index);
	int32_t set_all_video_param(const cam_video_param& param, const cam_video_param* sec_param);
	int32_t start_all_video_rec(const std::map<int32_t,std::shared_ptr<cam_video_buff_i>>& queue);
	int32_t stop_all_video_rec();
	int32_t start_all_timelapse_ex(const cam_photo_param& param, const std::shared_ptr<cam_img_repo>& img_repo);
	int32_t stop_all_timelapse_ex();
	std::future<int32_t> start_all_still_capture(const cam_photo_param& param, const std::shared_ptr<cam_img_repo>& img_repo);
	int32_t start_still_capture(uint32_t index, const cam_photo_param& param);
	int32_t set_all_capture_mode(uint32_t mode);
	int32_t get_version(int32_t index, std::vector<std::string>& version);
	int32_t upgrade(std::string file_name, std::string version);
	int32_t set_options(int32_t index, std::string property, int32_t value, int32_t value2 = 0);
	int32_t get_options(int32_t index, std::string property, std::string& value);
	int32_t get_all_options(std::string property, std::vector<std::string>& m_value);
	int32_t format_flash(int32_t index);
	int32_t test_spi(int32_t index);
	int32_t master_index() { return master_index_; };
	int32_t get_log_file(std::string file_name);
	int32_t get_module_hw_version(int32_t& hw_version);
	int32_t send_all_file_data(std::string file_name, int32_t type, int32_t timeout);
	int32_t send_all_buff_data(const uint8_t* data, uint32_t size, int32_t type, int32_t timeout);
	int32_t get_video_param(cam_video_param& param, std::shared_ptr<cam_video_param>& sec_param);
	int32_t change_all_usb_mode();
	int32_t set_uuid(uint32_t index, std::string uuid);
	int32_t get_uuid(uint32_t index, std::string& uuid, std::string& sensor_id);
	int32_t calibration_awb(uint32_t index, int32_t x, int32_t y, int32_t r, std::string& sensor_id, int32_t* maxvalue, std::shared_ptr<insbuff>& buff);
	int32_t calibration_bpc_all();
	int32_t calibration_blc_all(bool b_reset);
	int32_t gyro_calibration();
	int32_t magmeter_calibration();
	int32_t start_magmeter_calibration();
	int32_t stop_magmeter_calibration();
	int32_t vig_min_change();
	int32_t delete_file(std::string dir);
	int32_t storage_speed_test(std::map<int32_t, bool>& map_res);
	int32_t send_data(uint32_t index, uint8_t* data, uint32_t size);

	int64_t get_start_pts(int32_t index);
	uint32_t get_pid(uint32_t index) 
	{ 
		if (index < pid_.size()) return pid_[index]; 
		else return -1;
	};
	uint32_t get_index(uint32_t pid) 
	{ 
		for (uint32_t i = 0; i < pid_.size(); i++)
		{
			if (pid == pid_[i]) return i;
		}
		return -1;
	};
	
private:
	int32_t set_one_image_property(int32_t index,cam_image_property& param);	
	int32_t set_all_image_property(cam_image_property& param);
	int32_t get_one_version(int32_t index, std::vector<std::string>& version);
	int32_t get_all_version(std::vector<std::string>& version);
	int32_t set_one_options(int32_t index, std::string property, int32_t value, int32_t value2 = 0);
	int32_t set_all_options(std::string property, int32_t value, int32_t value2 = 0);
	int32_t format_one_flash(int32_t index);
	int32_t format_all_flash();
	int32_t do_open_cam(std::vector<uint32_t>& index);
	int32_t still_capture_task(const cam_photo_param& param, std::shared_ptr<cam_img_repo> img_repo);
	void timelapse_task(cam_photo_param param, const std::shared_ptr<cam_img_repo>& img_repo);
	void reset_base_ref();
	int32_t reboot_all_camera();
	int32_t check_fw_version(std::string version);
	void time_sync_task(bool b_record);
	void stop_sync_time();
	int32_t timelapse_take_pic(const cam_photo_param& param, const std::shared_ptr<cam_img_repo>& img_repo);
	int32_t parse_module_version(const std::string& version, std::string& hw_ver);
	void load_config_from_storage(std::string path);

	std::thread th_still_cap_;
	std::thread th_time_sync_;
	std::thread th_timelapse_;
	const uint32_t cam_num_ = INS_CAM_NUM;
	//uint32_t vid_ = 0x4255;
	int32_t master_index_ = 0;
	std::vector<uint32_t> pid_;
	std::map<uint32_t, std::shared_ptr<usb_camera>, std::greater<uint32_t>> map_cam_;
	uint32_t still_cap_status_ = INS_UNSED; //>0 表示没有结束 <=0 表示错误码
	bool sync_quit_ = false;
	bool timelapse_quit_ = false;
	std::condition_variable cv_;
	std::mutex mtx_cv_;

	static int32_t nv_amba_delta_usec_;
};

#endif
