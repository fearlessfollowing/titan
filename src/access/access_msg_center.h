
#ifndef _ACCESS_MSG_CENTER_H_
#define _ACCESS_MSG_CENTER_H_

#include <unordered_map>
#include "access_msg_parser.h"
#include "access_msg_receiver.h"
#include "access_msg_sender.h"
#include "access_state.h"
#include "json_obj.h"

class cam_manager;
class video_rtstitch_mgr;
class ins_timer;
class qrcode_scan;
class timelapse_mgr;
class pic_rtstitch_mgr;
class singlen_mgr;
class audio_capture_mgr;
class temp_monitor;
class box_task_mgr;
class video_mgr;
class image_mgr;
class timelapse_mgr;
class singlen_mgr;
class qr_scanner;
class audio_record;
class audio_dev_monitor;
class hdmi_monitor;

class access_msg_center {
public:
				~access_msg_center();
	int32_t 	setup();
	static void queue_msg(uint32_t sequence, const std::string& content);
	static void send_msg(std::string cmd, const json_obj* param);
	static bool is_idle_state();

private:

	void 	handle_msg(const std::shared_ptr<access_msg_buff> msg);
	void 	handle_msg_from_father(int32_t fd);
	void 	register_all_msg();

	//from client msg
	void 	start_preview(const char* msg, std::string cmd, int32_t sequence);
	void 	stop_preview(const char* msg, std::string cmd, int32_t sequence);
	void 	restart_preview(const char* msg, std::string cmd, int32_t sequence);
	void 	start_record(const char* msg, std::string cmd, int32_t sequence);
	void 	stop_record(const char* msg, std::string cmd, int32_t sequence);
	void 	start_live(const char* msg, std::string cmd, int32_t sequence);
	void 	stop_live(const char* msg, std::string cmd, int32_t sequence);
	void 	take_picture(const char* msg, std::string cmd, int32_t sequence);
	void 	query_state(const char* msg, std::string cmd, int32_t sequence);
	void 	set_offset(const char* msg, std::string cmd, int32_t sequence);
	void 	get_offset(const char* msg, std::string cmd, int32_t sequence);
	void 	get_image_param(const char* msg, std::string cmd, int32_t sequence);
	void 	change_storage_path(const char* msg, std::string cmd, int32_t sequence);
	void 	calibration(const char* msg, std::string cmd, int32_t sequence);
	void 	upgrade_fw(const char* msg, std::string cmd, int32_t sequence);
	void 	start_qr_scan(const char* msg, std::string cmd, int32_t sequence);
	void 	stop_qr_scan(const char* msg, std::string cmd, int32_t sequence);
	void 	do_stop_qr_scan();
	void 	format_camera_moudle(const char* msg, std::string cmd, int32_t sequence);
	void 	set_option(const char* msg, std::string cmd, int32_t sequence);
	int32_t set_one_option(std::string property, int32_t value, const std::string& value2 = "");
	void 	get_option(const char* msg, std::string cmd, int32_t sequence);
	void 	power_off_req(const char* msg, std::string cmd, int32_t sequence);
	void 	test_storage_speed(const char* msg, std::string cmd, int32_t sequence);
	void 	gyro_calibration(const char* msg, std::string cmd, int32_t sequence);
	void 	magmeter_calibration(const char* msg, std::string cmd, int sequence);
	void 	test_module_communication(const char* msg, std::string cmd, int32_t sequence);
	void 	test_module_spi(const char* msg, std::string cmd, int32_t sequence);
	void 	get_module_log_file(const char* msg, std::string cmd, int32_t sequence);
	void 	calibration_awb(const char* msg, std::string cmd, int32_t sequence);
	void 	calibration_bpc(const char* msg, std::string cmd, int32_t sequence);
	void 	calibration_blc(const char* msg, std::string cmd, int32_t sequence);
	void 	start_capture_audio(const char* msg, std::string cmd, int32_t sequence);
	void 	stop_capture_audio(const char* msg, std::string cmd, int32_t sequence);
	void 	system_time_change(const char* msg, std::string cmd, int32_t sequence);
	void 	storage_test_1(const char* msg, std::string cmd, int32_t sequence);
	void 	update_gamma_curve(const char* msg, std::string cmd, int32_t sequence);
	void 	query_storage(const char* msg, std::string cmd, int32_t sequence);
	void 	change_module_usb_mode(const char* msg, std::string cmd, int sequence);
	void 	query_gps_status(const char* msg, std::string cmd, int sequence);
	void 	set_gyro_calibration_res(const char* msg, std::string cmd, int sequence);
	void 	get_gyro_calibration_res(const char* msg, std::string cmd, int sequence);
	void 	query_battery_temp(const char* msg, std::string cmd, int sequence);
	void 	delete_file(const char* msg, std::string cmd, int sequence);
	void 	start_magmeter_calibration(const char* msg, std::string cmd, int sequence);
	void 	stop_magmeter_calibration(const char* msg, std::string cmd, int sequence);
	void 	module_power_on(const char* msg, std::string cmd, int sequence);
	void 	module_power_off(const char* msg, std::string cmd, int sequence);
	void 	change_udisk_mode(const char* msg, std::string cmd, int sequence);

	//stitching box msg
	#if 0
	void s_start_box(const char* msg, std::string cmd, int32_t sequence);
	void s_stop_box(const char* msg, std::string cmd, int32_t sequence);
	void s_query_task_list(const char* msg, std::string cmd, int32_t sequence);
	void s_start_task_list(const char* msg, std::string cmd, int32_t sequence);
	void s_stop_task_list(const char* msg, std::string cmd, int32_t sequence);
	void s_add_task(const char* msg, std::string cmd, int32_t sequence);
	void s_delete_task(const char* msg, std::string cmd, int32_t sequence);
	void s_start_task(const char* msg, std::string cmd, int32_t sequence);
	void s_stop_task(const char* msg, std::string cmd, int32_t sequence);
	void s_update_task(const char* msg, std::string cmd, int32_t sequence);
	void s_query_task(const char* msg, std::string cmd, int32_t sequence);
	#endif
	
	//internal msg
	void internal_sink_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_net_link_state(const char* msg, std::string cmd, int32_t sequence);
	void internal_live_stats(const char* msg, std::string cmd, int32_t sequence);
	void internal_pic_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_pic_origin_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_fifo_close(const char* msg, std::string cmd, int32_t sequence);
	void internal_timelapse_pic_take(const char* msg, std::string cmd, int32_t sequence);
	void internal_storage_st_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_gyro_calibration_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_magmeter_calibration_finish(const char* msg, std::string cmd, int sequence);
	void internal_calibration_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_qr_scan_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_capture_audio_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_stop_rec_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_stop_live_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_reset(const char* msg, std::string cmd, int32_t sequence);
	void internal_blc_calibration_finish(const char* msg, std::string cmd, int32_t sequence);
	void internal_bpc_calibration_finish(const char* msg, std::string cmd, int sequence);
	void internal_snd_dev_change(const char* msg, std::string cmd, int32_t sequence);
	void internal_first_frame_ts(const char* msg, std::string cmd, int sequence);
	void internal_video_fragment(const char* msg, std::string cmd, int sequence);
	void internal_vig_min_change(const char* msg, std::string cmd, int sequence);
	void internal_delete_file_finish(const char* msg, std::string cmd, int sequence);

	//other
	int32_t do_stop_record(int32_t ret);
	int32_t do_stop_live(int32_t ret, bool b_stop_rec);
	int32_t do_stop_preview();
	void do_test_storage_speed(std::string path);
	void do_gyro_calibration();
	void do_magmeter_calibration();
	void do_delete_file(std::string dir);
	int32_t do_calibration(std::string mode, int32_t delay);
	void do_blc_calibration(bool b_reset);
	void do_bpc_calibration();
	int32_t open_camera(int32_t index);
	void close_camera();
	int32_t do_camera_operation_stop(bool preview_restart);
	int32_t do_ge_fw_version();
	void send_box_result(std::string cmd, int32_t sequence, const std::map<std::string,int32_t>& res);
	int32_t set_depth_map(const std::string& depth_map);

	std::shared_ptr<cam_manager>			camera_;			/* Camera管理器(用于管理所有的模组) */
	std::shared_ptr<ins_timer> 				timer_;
	std::shared_ptr<timelapse_mgr>	 		timelapse_;
	std::shared_ptr<temp_monitor> 			t_monitor_;
	std::shared_ptr<box_task_mgr> 			task_mgr_;
	std::shared_ptr<video_mgr> 				video_mgr_;
	std::shared_ptr<image_mgr> 				img_mgr_;			/* 拍照管理器 */
	std::shared_ptr<timelapse_mgr> 			timelapse_mgr_;		/* timelapse管理器 */
	std::shared_ptr<singlen_mgr> 			singlen_mgr_;
	std::shared_ptr<qr_scanner> 			qr_scanner_;
	std::shared_ptr<audio_record> 			audio_rec_;
	std::shared_ptr<audio_dev_monitor> 		snd_monitor_;
	std::shared_ptr<hdmi_monitor> 			hdmi_monitor_;
	
	access_msg_parser 		msg_parser_;						/* 消息解析器 */
	access_state 			state_mgr_;

	static uint32_t 		state_;								/* 系统的状态 */

	bool 					b_need_stop_live_ = false;
	bool 					b_stop_live_rec_ = false;
	std::mutex 				camera_operation_stop_mtx_;

	std::thread 			th_;
	std::unordered_map<std::string, std::function<void(const char*, std::string, int32_t)>> handler_;
	static std::shared_ptr<access_msg_receiver> 	receiver_;
	static std::shared_ptr<access_msg_sender> 		sender_;
};

#endif