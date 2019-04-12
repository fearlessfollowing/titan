#ifndef _USB_CAMERA_H_
#define _USB_CAMERA_H_

#include <thread>
#include <string>
#include <deque>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <future>
#include <unordered_map>
#include "cam_data.h"
#include <atomic>
#include "cam_video_buff_i.h"
#include "cam_img_repo.h"
#include "obj_pool.h"
#include "MagneticCalibration.hpp"

struct amba_frame_info;

#define RECV_PIC_TIMEOUT 20000

/*
 * usb_camera - USB模组
 */
class usb_camera {

public:
				usb_camera(uint32_t pid, uint32_t index): pid_(pid), index_(index) { 
					pool_ = std::make_shared<obj_pool<ins_frame>>(-1, "usb frame");		/* 构造一个ins_frame缓冲池(默认含有16个对象) */
					th_cmd_read_ = std::thread(&usb_camera::read_cmd_task, this);		/* 创建一个读模组命令响应的线程 */
				};

				~usb_camera() { close(); };

	void 		close();
	int32_t 	exception() { return exception_; };
	int32_t 	set_camera_time();
	int32_t 	get_camera_time(int32_t& delta_time);
	int32_t 	start_video_rec(const std::shared_ptr<cam_video_buff_i>& queue);
	int32_t 	stop_video_rec();
	int32_t 	start_still_capture(const cam_photo_param& param, std::shared_ptr<cam_img_repo> pic_buff = nullptr);
	int32_t 	set_video_param(const cam_video_param& param, const cam_video_param* sec_param = nullptr);
	int32_t 	get_video_param(cam_video_param& param, std::shared_ptr<cam_video_param>& sec_param);
	int32_t 	set_photo_param(const cam_photo_param& param);
	int32_t 	set_capture_mode(int32_t mode);
	int32_t 	set_options(std::string property,int32_t value);
	int32_t 	get_options(std::string property, std::string& value);
	int32_t 	get_version(std::string& version);
	int32_t 	test_spi();
	int32_t 	reboot();
	int32_t 	format_flash();
	int32_t 	upgrade(std::string file_name, const std::string& md5);
	int32_t 	set_uuid(std::string uuid);
	int32_t 	get_uuid(std::string& uuid, std::string& sensorId);
	int32_t 	calibration_awb_req(int32_t x, int32_t y, int32_t r, std::string& sensor_id, int32_t* maxvalue, std::shared_ptr<insbuff>& buff);
	int32_t 	calibration_bpc_req();
	int32_t 	calibration_blc_req();
	int32_t 	calibration_blc_reset();
	int32_t 	send_buff_data(const uint8_t* data, uint32_t size, int32_t type, int32_t timeout);
	int32_t 	gyro_calibration();
	int32_t	 	magmeter_calibration();
	int32_t 	start_magmeter_calibration();
	int32_t 	stop_magmeter_calibration();
	int32_t 	do_magmeter_calibration();
	int32_t 	get_vig_min_value(int32_t& value);
	int32_t 	set_vig_min_value(int32_t value);
	int32_t 	storage_speed_test();
	int32_t 	delete_file(std::string dir);
	int32_t 	send_data(uint8_t* data, uint32_t size);
	
	std::future<int32_t> 	wait_cmd_over();
	int32_t 				do_wait_cmd_over();
	uint32_t 				get_sequence();
	void 					set_delta_time(uint32_t sequence, int32_t delta_time);
	int32_t 				get_log_file(std::string file_name);
	int32_t 				change_usb_mode();
	int64_t 				get_start_pts() { return start_pts_; };

	std::string 			get_result();
	
	std::shared_ptr<ins_frame> deque_pic();
	
	static std::atomic_llong base_ref_pts_;

private:
	void 		clear_rec_context();
	void 		read_cmd_task();
	int32_t 	read_cmd_rsp(int32_t timeout, int32_t cmd = -1);
	int32_t 	read_cmd(int32_t timeout);
	int32_t 	send_cmd(uint32_t cmd, std::string content = "");

	void 		queue_video(const std::shared_ptr<page_buffer>& buff, uint8_t frametype, int64_t timestamp, uint32_t sequence, uint32_t extra_size);
	void 		enque_pic(const std::shared_ptr<page_buffer>& buff, int64_t timestamp, uint32_t sequence, uint32_t extra_size);
	void 		queue_pic_raw(const std::shared_ptr<insbuff>& buff, int32_t sequece, bool b_end, int64_t timestamp);
	int32_t 	parse_sps_pps(const uint8_t* data, uint32_t data_size);
	void 		parse_nal_pos(const uint8_t* data, uint32_t data_size, uint8_t nal_type, int32_t& start_pos, int32_t& size);
	void 		start_data_read_task();
	int32_t 	stop_data_read_task(bool b_wait = false);
	int32_t 	read_data_task();
	int32_t 	read_data_head(amba_frame_info* head);
	int32_t 	read_data();
	int32_t 	send_fw(std::string file_name, const std::string& md5);
	void 		parse_sersor_info(const std::string& info, std::string& sensor_id, int32_t* maxvalue);
	void 		pre_process_timestamp(uint32_t sequence, int64_t timestamp);
	void 		pasre_exif_info(const uint8_t* data, uint32_t size, jpeg_metadata* metadata);
	void 		parse_extra_data(const uint8_t* data, uint32_t size, uint32_t seq, jpeg_metadata* metadata = nullptr);
	int32_t 	process_magmeter_data(std::shared_ptr<insbuff> buff, std::string& res, ins::magneticCalibrate* cal);
	void 		send_rec_over_msg(int32_t errcode) const;
	void 		send_pic_origin_over() const;
	void 		send_timelapse_pic_take(uint32_t sequence) const;
	void 		send_storage_state(std::string state) const;
	
	void 		send_first_frame_ts(int32_t rec_seq, int64_t ts) const;

	void 		send_video_fragment_msg(int32_t frament_index) const;
	void 		send_vig_min_value_change_msg() const;
	int32_t 	set_image_property(std::string property,int32_t value);
	int32_t 	set_param(std::string property, int32_t value);
	int32_t 	req_retransmit(bool read_pre_data);
	int32_t 	send_data_by_ep_cmd(uint8_t* data, uint32_t size);
	void 		print_fps_info();
	bool 		is_exception_cmd(int32_t cmd);

    const char* get_cmd_str(unsigned int uCmd);

	std::mutex 								mtx_send_; //发送数据锁
	std::mutex 								mtx_pic_;
	// std::thread 							th_data_read_;

	std::future<int32_t> 					f_data_read_;
	std::future<int32_t> 					f_magmeter_cal_;

	std::thread 							th_cmd_read_;

	std::deque<std::shared_ptr<ins_frame>> 	pic_queue_;		/* 接收照片数据的deque */

	std::shared_ptr<insbuff> 				ppsItem;
	std::shared_ptr<insbuff> 				spsItem;

	std::deque<std::shared_ptr<insbuff>> 	raw_buff_;

	std::shared_ptr<insbuff> 				buff_; //using for somthing else, temp

	uint32_t 								pid_ = -1;
	uint32_t 								index_ = -1;

	bool 									is_recording_ = false;
	bool 									is_data_thread_quit_ = true;
	bool 									quit_ = false;
	bool 									mag_cal_quit_ = false;
	uint32_t 								last_seq_ = 0;
	bool 									wait_idr_ = false;
	int64_t 								delta_pts_ = INS_PTS_NO_VALUE; //delta time between six module

	std::shared_ptr<cam_video_buff_i> 		video_buff_;		/* 存储视频数据的buff */
	
	std::shared_ptr<cam_img_repo> 			img_repo_;			/* 存储照片数据的repo */

	int32_t 								send_cmd_ = -1;		/* 当前正在发送的命令 */
	
	std::unordered_map<uint32_t, int32_t> 	cmd_rsp_map_;
	std::mutex 								mtx_cmd_rsp_;
	std::string 							cmd_result_; 			// json, data content
	
	double 									fps_ = 30;
	int32_t 								delta_time_cur_ = 0; 	//delta time between tx1 and a12
	
	int32_t 								delta_time_new_ = 0; 	/* 下次对时时,模组应该设置的时间戳 */

	uint32_t 								sequence_cur_ = 0;		/* 当前的序列号 */

	uint32_t 								sequence_delta_time_ = -1; /* 下一次对应对应的帧序列号 */

	int32_t 								pic_cnt_ = 0; 			/* BURST模式下,模组会拍超过一组照片数据 */

	int32_t 								single_pic_timeout_ = RECV_PIC_TIMEOUT; //3s
	int32_t 								total_pic_timeout_ 	= RECV_PIC_TIMEOUT; //3s

	std::string 							pic_type_ = INS_PIC_TYPE_PHOTO;

	static uint32_t 						timelape_seq = 0;

	uint32_t 								pic_seq_ = 0;
	uint32_t 								raw_seq_ = 0;
	FILE* 									log_file_fp_ = nullptr;
	uint32_t 								frame_seq_ = 0;
	bool 									b_retransmit_ = false;
	int64_t 								start_pts_ = INS_PTS_NO_VALUE;

	std::shared_ptr<obj_pool<ins_frame>> 	pool_;
	std::shared_ptr<insbuff> 				gyro_buff_;
	struct timeval 							tm_module_end_;
	
	int32_t 								exception_ = INS_OK; //作为异常的标记表示退出
	bool 									b_record_ = false;
	int32_t 								rec_seq_ = -1;
	uint64_t 								temp_cnt_ = 0;

	int32_t 								cnt = -1;
	struct timeval 							start_time;
	struct timeval 							end_time;
};

#endif
