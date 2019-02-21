#ifndef _USB_CAMERA_H_
#define _USB_CAMERA_H_
#include <thread>
#include <string>
#include <deque>
#include <mutex>
#include <map>
#include <unordered_map>
#include <vector>
#include <future>

struct amba_frame_info;

class usb_camera
{
public:
	usb_camera(uint32_t pid, uint32_t index) : pid_(pid) , index_(index) 
	{
		th_cmd_read_ = std::thread(&usb_camera::read_cmd_task, this);
	}
	~usb_camera() { close(); };
	void close();
	int32_t get_version(std::string& version);
	int32_t reboot();
	int32_t upgrade(std::string file_name, const std::string& md5);
	int32_t get_uuid_sensorid();
	int32_t send_calibration_data(const std::string& file_name, int32_t type);
	int32_t send_calibration_data(uint8_t* data, uint32_t size, int32_t type);
	int32_t test_spi();
	int32_t calibration_bpc_req();
	int32_t calibration_blc_req();
	int32_t calibration_blc_reset();
	int32_t test_connection();
	int32_t get_options(std::string property, std::string& value);

	std::future<int32_t> wait_cmd_over();
	std::string get_result();

private:
	void read_cmd_task();
	int32_t read_cmd_rsp(int32_t timeout, int32_t cmd = -1);
	int32_t read_cmd(int32_t timeout);
	int32_t send_cmd(uint32_t cmd, std::string content = "");
	int32_t send_data(uint8_t* data, uint32_t size);
	int32_t send_file(const std::string& file_name, int32_t type, const std::string md5 = "");
	int32_t send_buff(uint8_t* data, uint32_t size, int32_t type);
	int32_t do_wait_cmd_over();
	
	uint32_t pid_ = -1;
	int32_t index_ = -1;
	bool quit_ = false;
	bool rebooting_ = false;

	int32_t send_cmd_ = -1;
	std::thread th_cmd_read_;
	std::string cmd_result_; //json, data content
	std::unordered_map<uint32_t, int32_t> cmd_rsp_map_;
	std::mutex mtx_cmd_rsp_;
};

#endif
