
#include "access_msg_center.h"
#include "inslog.h"
#include "common.h"
#include <sstream>
#include <fstream>
#include "cam_data.h"
#include "xml_config.h"
#include "access_msg_center_head.h"
#include "offset_wrap.h"
#include "ins_uuid.h"
#include "gps_mgr.h"
#include "ins_util.h"
#include "ffutil.h" 
#include "upload_cal_file.h"
#include "storage_test.h"
#include "usb_msg.h"
#include "ins_base64.h"
#include "ins_len_circle.h"
#include "camera_info.h"
#include "hw_util.h"

#include "cam_manager.h"
#include "ins_timer.h"
#include "storage_speed_test.h"
#include "temp_monitor.h"
#include "box_task_mgr.h"
#include "video_mgr.h"
#include "image_mgr.h"
#include "timelapse_mgr.h"
#include "singlen_mgr.h"
#include "qr_scanner.h"
#include "audio_record.h"
#include "audio_dev_monitor.h"
#include "ins_battery.h"
#include "camera_info.h"
#include "insx11.h"
#include "hdmi_monitor.h"
#include <system_properties.h>


std::shared_ptr<access_msg_receiver> access_msg_center::receiver_ = nullptr;
std::shared_ptr<access_msg_sender> access_msg_center::sender_ = nullptr; 
uint32_t access_msg_center::state_ = 0;


#define INS_REG_ACCESS_MSG(msg, func) handler_[msg] = std::bind(func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);


void access_msg_center::register_all_msg()
{
	/*
 	 * 来自外部的消息(web_server) 
 	 */
	INS_REG_ACCESS_MSG(ACCESS_CMD_START_PREVIEW, 			&access_msg_center::start_preview)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STOP_PREVIEW, 			&access_msg_center::stop_preview)
	INS_REG_ACCESS_MSG(ACCESS_CMD_RESTART_PREVIEW, 			&access_msg_center::restart_preview)
	INS_REG_ACCESS_MSG(ACCESS_CMD_START_RECORD, 			&access_msg_center::start_record)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STOP_RECORD, 				&access_msg_center::stop_record)
	INS_REG_ACCESS_MSG(ACCESS_CMD_START_LIVE, 				&access_msg_center::start_live)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STOP_LIVE, 				&access_msg_center::stop_live)
	INS_REG_ACCESS_MSG(ACCESS_CMD_TAKE_PICTURE, 			&access_msg_center::take_picture)
	INS_REG_ACCESS_MSG(ACCESS_CMD_SET_OFFSET, 				&access_msg_center::set_offset)
	INS_REG_ACCESS_MSG(ACCESS_CMD_GET_OFFSET, 				&access_msg_center::get_offset)
	INS_REG_ACCESS_MSG(ACCESS_CMD_GET_IMAGE_PARAM, 			&access_msg_center::get_image_param)
	INS_REG_ACCESS_MSG(ACCESS_CMD_CHANGE_STORAGE_PATH, 		&access_msg_center::change_storage_path)
	INS_REG_ACCESS_MSG(ACCESS_CMD_QUERY_STORAGE, 			&access_msg_center::query_storage)
	INS_REG_ACCESS_MSG(ACCESS_CMD_QUREY_STATE, 				&access_msg_center::query_state)
	INS_REG_ACCESS_MSG(ACCESS_CMD_CALIBRATION, 				&access_msg_center::calibration)
	INS_REG_ACCESS_MSG(ACCESS_CMD_UPGRADE_FW, 				&access_msg_center::upgrade_fw)
	INS_REG_ACCESS_MSG(ACCESS_CMD_CALIBRATION_AWB, 			&access_msg_center::calibration_awb)
	INS_REG_ACCESS_MSG(ACCESS_CMD_CALIBRATION_BPC, 			&access_msg_center::calibration_bpc)
	INS_REG_ACCESS_MSG(ACCESS_CMD_CALIBRATION_BLC, 			&access_msg_center::calibration_blc)
	INS_REG_ACCESS_MSG(ACCESS_CMD_START_QRCODE_SCAN, 		&access_msg_center::start_qr_scan)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STOP_QRCODE_SCAN, 		&access_msg_center::stop_qr_scan)
	INS_REG_ACCESS_MSG(ACCESS_CMD_FORMAT_CAMERA_MOUDLE, 	&access_msg_center::format_camera_moudle)
	INS_REG_ACCESS_MSG(ACCESS_CMD_GET_MODULE_LOG_FILE, 		&access_msg_center::get_module_log_file)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STORAGE_SPEED_TEST, 		&access_msg_center::test_storage_speed)
	INS_REG_ACCESS_MSG(ACCESS_CMD_POWEROFF, 				&access_msg_center::power_off_req)
	INS_REG_ACCESS_MSG(ACCESS_CMD_LOW_BAT_ACT, 				&access_msg_center::power_off_req)    
	INS_REG_ACCESS_MSG(ACCESS_CMD_GYRO_CALIBRATION, 		&access_msg_center::gyro_calibration)
	INS_REG_ACCESS_MSG(ACCESS_CMD_MAGMETER_CALIBRATION, 	&access_msg_center::magmeter_calibration)
	INS_REG_ACCESS_MSG(ACCESS_CMD_SET_OPTION, 				&access_msg_center::set_option)
	INS_REG_ACCESS_MSG(ACCESS_CMD_GET_OPTION, 				&access_msg_center::get_option) 
	INS_REG_ACCESS_MSG(ACCESS_CMD_TEST_MODULE_COMMUNICATION, &access_msg_center::test_module_communication)
	INS_REG_ACCESS_MSG(ACCESS_CMD_TEST_MODULE_SPI, 			&access_msg_center::test_module_spi)
	INS_REG_ACCESS_MSG(ACCESS_CMD_START_CAPTURE_AUDIO, 		&access_msg_center::start_capture_audio)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STOP_CAPTURE_AUDIO, 		&access_msg_center::stop_capture_audio)
	INS_REG_ACCESS_MSG(ACCESS_CMD_SYSTEM_TIME_CHANGE, 		&access_msg_center::system_time_change)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STORAGE_TEST,	 			&access_msg_center::storage_test_1)
	INS_REG_ACCESS_MSG(ACCESS_CMD_UPDATE_GAMMA_CURVE, 		&access_msg_center::update_gamma_curve)
	INS_REG_ACCESS_MSG(ACCESS_CMD_CHANGE_MODULE_USB_MODE, 	&access_msg_center::change_module_usb_mode)
	INS_REG_ACCESS_MSG(ACCESS_CMD_QUERY_GPS_STATUS, 		&access_msg_center::query_gps_status)
	INS_REG_ACCESS_MSG(ACCESS_CMD_SET_GYRO_CAL_RES, 		&access_msg_center::set_gyro_calibration_res)
	INS_REG_ACCESS_MSG(ACCESS_CMD_GET_GYRO_CAL_RES, 		&access_msg_center::get_gyro_calibration_res)
	INS_REG_ACCESS_MSG(ACCESS_CMD_QUERY_BATTERY_TEMP, 		&access_msg_center::query_battery_temp)
	INS_REG_ACCESS_MSG(ACCESS_CMD_DELETE_FILE, 				&access_msg_center::delete_file)
	INS_REG_ACCESS_MSG(ACCESS_CMD_MODULE_POWON, 			&access_msg_center::module_power_on)
	INS_REG_ACCESS_MSG(ACCESS_CMD_MODULE_POWOFF, 			&access_msg_center::module_power_off)
	INS_REG_ACCESS_MSG(ACCESS_CMD_START_MAGMETER_CALIBRATION, &access_msg_center::start_magmeter_calibration)
	INS_REG_ACCESS_MSG(ACCESS_CMD_STOP_MAGMETER_CALIBRATION, &access_msg_center::stop_magmeter_calibration)
	INS_REG_ACCESS_MSG(ACCESS_CMD_CHANGE_UDISK_MODE, 		&access_msg_center::change_udisk_mode)
	
	
	/*
	 * stitching box消息
	 */
	#if 0
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_START_BOX, 				&access_msg_center::s_start_box)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_STOP_BOX, 				&access_msg_center::s_stop_box)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_QUERY_TASK_LIST, 		&access_msg_center::s_query_task_list)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_START_TASK_LIST, 		&access_msg_center::s_start_task_list)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_STOP_TASK_LIST, 		&access_msg_center::s_stop_task_list)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_ADD_TASK, 				&access_msg_center::s_add_task)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_DELETE_TASK, 			&access_msg_center::s_delete_task)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_START_TASK, 			&access_msg_center::s_start_task)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_STOP_TASK, 				&access_msg_center::s_stop_task)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_UPDATE_TASK, 			&access_msg_center::s_update_task)
	INS_REG_ACCESS_MSG(ACCESS_CMD_S_QUERY_TASK, 			&access_msg_center::s_query_task)
	#endif
	
	/*
	 * 进程内部消息
	 */
	INS_REG_ACCESS_MSG(INTERNAL_CMD_FIFO_CLOSE, 		&access_msg_center::internal_fifo_close)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_SINK_FINISH, 		&access_msg_center::internal_sink_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_PIC_FINISH, 		&access_msg_center::internal_pic_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_QR_SCAN_FINISH, 	&access_msg_center::internal_qr_scan_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_PIC_ORIGIN_F, 		&access_msg_center::internal_pic_origin_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_STORAGE_ST_F, 		&access_msg_center::internal_storage_st_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_GYRO_CALIBRATION_F, &access_msg_center::internal_gyro_calibration_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_MAGMETER_CALIBRATION_F, &access_msg_center::internal_magmeter_calibration_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_CAPTURE_AUDIO_F, 	&access_msg_center::internal_capture_audio_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_STOP_REC_F, 		&access_msg_center::internal_stop_rec_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_STOP_LIVE_F, 		&access_msg_center::internal_stop_live_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_RESET, 				&access_msg_center::internal_reset)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_BLC_CAL_F, 			&access_msg_center::internal_blc_calibration_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_BPC_CAL_F, 			&access_msg_center::internal_bpc_calibration_finish)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_SND_DEV_CHANGE, 	&access_msg_center::internal_snd_dev_change)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_FIRST_FRAME_TS, 	&access_msg_center::internal_first_frame_ts)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_VIDEO_FRAGMENT, 	&access_msg_center::internal_video_fragment)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_VIG_MIN_CHANGE, 	&access_msg_center::internal_vig_min_change)
	INS_REG_ACCESS_MSG(INTERNAL_CMD_DEL_FILE_F, 		&access_msg_center::internal_delete_file_finish)
}


void access_msg_center::queue_msg(unsigned int sequence, const std::string& content) 
{
	//LOGINFO("internal send msg:%s", content.c_str());
	auto msg = std::make_shared<access_msg_buff>(0, content.c_str(), content.length()); 
	receiver_->queue_msg(msg); 
};

void access_msg_center::send_msg(std::string cmd, const json_obj* param) 
{
	sender_->send_ind_msg_(cmd, param);
}

bool access_msg_center::is_idle_state()
{
	return (state_ == CAM_STATE_IDLE);
}

access_msg_center::~access_msg_center()
{
	LOGINFO("center destroy 1");
	snd_monitor_ = nullptr;
	t_monitor_ = nullptr;
	hdmi_monitor_ = nullptr;
	video_mgr_ = nullptr;
	img_mgr_ = nullptr;
	timelapse_mgr_ = nullptr;
	singlen_mgr_ = nullptr;
	qr_scanner_ = nullptr;
	camera_ = nullptr;
	task_mgr_ = nullptr;
	receiver_ = nullptr;
	sender_ = nullptr;
	insx11::release_x();
	hw_util::switch_fan(false);
	cam_manager::power_off_all();
	gps_mgr::destroy();
	INS_THREAD_JOIN(th_);
	LOGINFO("center destroy 2");
}


int access_msg_center::setup()
{
	int ret;
	
	register_all_msg();				/* 注册所有的消息处理 */
	ffutil::init();					/* 初始化通信fifo */
	
	cam_manager::power_off_all(); 	/* 给模组断电 */ 
	hw_util::switch_fan(true);		/* 开风扇 */

	state_ = CAM_STATE_IDLE; 		/* 初始化系统状态为IDLE状态 */

	msg_parser_.setup();			/* 初始化消息解析器 */
	camera_info::setup();			/* 获取系统信息 */

	sender_ = std::make_shared<access_msg_sender>();
	ret = sender_->setup();
	RETURN_IF_NOT_OK(ret);

	camera_info::set_volume(INS_DEFAULT_AUDIO_GAIN); 	/* 恢复默认gain */

	std::function<void(const std::shared_ptr<access_msg_buff>)> func = [this](const std::shared_ptr<access_msg_buff> msg)
	{
		handle_msg(msg);
	};


	/*
	 * receiver中创建用于处理所有收到的消息(handle_msg来处理所有收到的消息)
	 * - 从FIFO读到的消息
	 * - 进程内部其他线程发送的消息
	 */
	receiver_ = std::make_shared<access_msg_receiver>();
	ret = receiver_->start(INS_FIFO_TO_SERVER, func);
	RETURN_IF_NOT_OK(ret);


	/* 用黑色背景遮盖ubuntu界面 */
	insx11::setup_x(); 

	auto gps_mgr = gps_mgr::create();
	RETURN_IF_TRUE(!gps_mgr, "gps mgr create fail", INS_ERR);


	/*
	 * 启动温度监听
	 */
	std::function<bool()> temp_cb = [this]()->int
	{
		return (state_ & CAM_STATE_RECORD) || (state_ & CAM_STATE_LIVE);
	};
	t_monitor_ = std::make_shared<temp_monitor>(temp_cb);


	/*
	 * 创建并启动音频设备监听线程
	 */
	snd_monitor_ = std::make_shared<audio_dev_monitor>();
	snd_monitor_->start();
	
	/*
	 * HDMI监控线程
	 */
	hdmi_monitor_ = std::make_shared<hdmi_monitor>();
	hdmi_monitor_->start();

	return INS_OK;
}
 



/***********************************************************************************************
** 函数名称: handle_msg
** 函数功能: 消息处理中心线程处理消息
** 入口参数:
**		msg - 消息对象
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::handle_msg(const std::shared_ptr<access_msg_buff> msg)
{
	/* 调用msg_parser_解析出消息的"name" */
	std::string cmd = msg_parser_.parse_cmd_name(msg->content);
	if (handler_.count(cmd) <= 0) {
		LOGINFO("unsupport cmd:%s", cmd.c_str());
		sender_->send_rsp_msg(msg->sequence, INS_ERR_INVALID_MSG_FMT, cmd);
	} else {
		if (cmd != ACCESS_CMD_S_QUERY_TASK_LIST) {	/* 轮询消息不打印 */
			LOGINFO("[----MESSAGE----] seq:%d %s", msg->sequence, msg->content);
		}
		handler_[cmd](msg->content, cmd, msg->sequence);
	}
}


void access_msg_center::internal_fifo_close(const char* msg, std::string cmd, int sequence)
{
	sender_->re_setup();
}

void access_msg_center::internal_stop_rec_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);

	int ret = 0;
	auto root_obj = std::make_shared<json_obj>(msg);
	root_obj->get_int("code", ret);

	if (state_ & CAM_STATE_STOP_RECORD) {
		sender_->send_ind_msg(ACCESS_CMD_RECORD_FINISH_, ret);
	} else {
		LOGERR("revc internal_stop_rec_finish but not in stop rec state");
	}

	state_ &= ~CAM_STATE_STOP_RECORD;
}


void access_msg_center::internal_stop_live_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);

	int ret = 0;
	auto root_obj = std::make_shared<json_obj>(msg);
	root_obj->get_int("code", ret);

	if (state_ & CAM_STATE_STOP_LIVE) {
		sender_->send_ind_msg(ACCESS_CMD_LIVE_FINISH_, ret);
		state_ &= ~CAM_STATE_STOP_LIVE;
		b_need_stop_live_ = false;
	} else if (state_ & CAM_STATE_LIVE) {
		b_stop_live_rec_ = false;
		if (b_need_stop_live_) {
			do_stop_live(ret, false);
		} else {
			sender_->send_ind_msg(ACCESS_CMD_LIVE_REC_FINISH_, ret);
		}
	} else {
		LOGERR("revc internal_stop_live_finish but not in stop live state");
	}
}



void access_msg_center::internal_sink_finish(const char* msg, std::string cmd, int sequence)
{
	int ret = 0;
	bool b_preview = false;
	auto root_obj = std::make_shared<json_obj>(msg);
	root_obj->get_int("code", ret);
	root_obj->get_boolean("preview", b_preview);

	if (b_preview) {	/* 如果是在预览状态 */
		if (state_ & CAM_STATE_PREVIEW) {
			do_stop_preview(); /* 暂时同步处理,后续修改为异步处理 */
			sender_->send_ind_msg(ACCESS_CMD_PREVIEW_FINISH_, ret);
		}
	} else {
		if (state_ & CAM_STATE_RECORD) {
			do_stop_record(ret);
		} else if (state_ & CAM_STATE_LIVE) {
			bool b_stop_rec = false;
			if (ret == INS_ERR_NO_STORAGE_SPACE || ret == INS_ERR_UNSPEED_STORAGE) 
				b_stop_rec = true;
			do_stop_live(ret, b_stop_rec);
		} else if (state_ == CAM_STATE_PREVIEW) {
			if (ret == INS_ERR_MICPHONE_EXCEPTION) {
				bool dev_gone = false;
				root_obj->get_boolean("dev_gone", dev_gone);
				if (dev_gone) return; /* 如果是设备不见了，那么会收到设备更改消息，在那里处理这里不用处理 */
			}

			do_stop_preview();
			sender_->send_ind_msg(ACCESS_CMD_PREVIEW_FINISH_, ret);
		}
	}
}


void access_msg_center::internal_reset(const char* msg, std::string cmd, int sequence)
{
	//if (video_mgr_) video_mgr_->close_all_sink();

	int32_t ret = 0;
	auto root_obj = std::make_shared<json_obj>(msg);
	auto param_obj = root_obj->get_obj(ACCESS_MSG_PARAM);
	if (param_obj) param_obj->get_int("code", ret);

	if (state_ & CAM_STATE_PREVIEW) 
		sender_->send_ind_msg(ACCESS_CMD_PREVIEW_FINISH_, ret);
	
	if (state_ & CAM_STATE_RECORD || state_ & CAM_STATE_STOP_RECORD) 
		sender_->send_ind_msg(ACCESS_CMD_RECORD_FINISH_, ret);
	
	if (state_ & CAM_STATE_LIVE || state_ & CAM_STATE_STOP_LIVE) 
		sender_->send_ind_msg(ACCESS_CMD_LIVE_FINISH_, ret);
	
	if (state_ & CAM_STATE_PIC_SHOOT || state_ & CAM_STATE_PIC_PROCESS) 
		sender_->send_ind_msg(ACCESS_CMD_PIC_FINISH_, ret);
	
	if (state_ & CAM_STATE_QRCODE_SCAN) 
		sender_->send_ind_msg(ACCESS_CMD_QR_SCAN_FINISH_, ret);
	
	if (state_ & CAM_STATE_GYRO_CALIBRATION) 
		sender_->send_ind_msg(ACCESS_CMD_GYRO_CAL_FINISH_, ret);
	
	if (state_ & CAM_STATE_CALIBRATION) 
		sender_->send_ind_msg(ACCESS_CMD_CALIBRATION, ret);

	usleep(100*1000);
	
	cam_manager::power_off_all();
	kill(getpid(), 9);
	system("killall camerad");
	exit(-1);
}

void access_msg_center::internal_snd_dev_change(const char* msg, std::string cmd, int sequence)
{
	if (state_ != CAM_STATE_PREVIEW) return;
	if (video_mgr_ == nullptr) return;

	auto dev_cur = video_mgr_->get_snd_type();
 
	int dev = INS_SND_TYPE_NONE;
	json_obj root_obj(msg);
	root_obj.get_int("dev", dev);

	if (dev_cur != INS_SND_TYPE_NONE && dev != dev_cur) {
		do_camera_operation_stop(true);   
	} else {
		LOGINFO("no need change snd device");
	}
}


/***********************************************************************************************
** 函数名称: internal_first_frame_ts
** 函数功能: 处理设置第一帧的时间戳(内部消息)
** 入口参数:
**		msg 	 - 消息数据
**		cmd		 - 拍照命令"camera._takePicture"
**		sequence - 通信序列值
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::internal_first_frame_ts(const char* msg, std::string cmd, int sequence)
{
	if (video_mgr_ == nullptr) 
		return;
	
	int64_t ts = 0;
	int32_t rec_seq = -1;
	json_obj root_obj(msg);
	root_obj.get_int64("timestamp", ts);
	root_obj.get_int("rec_seq", rec_seq);
	video_mgr_->set_first_frame_ts(rec_seq, ts);
}


void access_msg_center::internal_video_fragment(const char* msg, std::string cmd, int sequence)
{
	if (video_mgr_ == nullptr) 
		return;
	
	int32_t seq = 0;
	json_obj root_obj(msg);
	root_obj.get_int("sequence", seq);
	video_mgr_->notify_video_fragment(seq);
}


void access_msg_center::internal_vig_min_change(const char* msg, std::string cmd, int sequence)
{
	if (camera_ == nullptr) 
		return;

	camera_->vig_min_change();
}

void access_msg_center::internal_pic_origin_finish(const char* msg, std::string cmd, int sequence)
{
	if (state_ & CAM_STATE_PIC_SHOOT) {
		sender_->send_ind_msg(ACCESS_CMD_PIC_ORIGIN_FINISH_, INS_OK);
		state_ &= ~CAM_STATE_PIC_SHOOT;
		state_ |= CAM_STATE_PIC_PROCESS;
	} else if (state_ & CAM_STATE_CALIBRATION) {
		sender_->send_ind_msg(ACCESS_CMD_CALIBRATION_ORIGIN_FINISH_, INS_OK);
	}
}

void access_msg_center::internal_pic_finish(const char* msg, std::string cmd, int sequence)
{
	if (!(state_ & CAM_STATE_PIC_SHOOT 
		|| state_ & CAM_STATE_PIC_PROCESS 
		|| state_ & CAM_STATE_CALIBRATION)) {
		LOGERR("recv pic finish msg, but not in pic state");
		return;
	}

	int ret;
	std::string path;
	auto root_obj = std::make_shared<json_obj>(msg);
	root_obj->get_int("code", ret);
	root_obj->get_string("path", path);

	bool b_calibration = (state_ & CAM_STATE_CALIBRATION) ? true : false;

	state_ &= ~CAM_STATE_PIC_SHOOT;
	state_ &= ~CAM_STATE_PIC_PROCESS;
	state_ &= ~CAM_STATE_CALIBRATION;

	if (ret != INS_OK) {	/* 拍照出错,如果文件夹已经创建,删除该文件夹 */
		if (access(path.c_str(), F_OK) == 0) {
			LOGINFO("take picture finish error, clean tmp dir: %s", path.c_str());
			std::string cmd = "rm -rf " + path;
			system(cmd.c_str());			
		}
	}
	
	do_camera_operation_stop(true);


	/* 等所有操作完成后再回响应，避免客户端快速进行其他操作容易造成模组异常 */
	if (b_calibration) {
		sender_->send_ind_msg(ACCESS_CMD_CALIBRATION_FINISH_, ret);
	} else {
		usleep(1000*1000);
		json_obj res_obj;
		if (ret == INS_OK) {
			res_obj.set_string(ACCESS_MSG_OPT_PIC_URL, path);
		}
		sender_->send_ind_msg(ACCESS_CMD_PIC_FINISH_, ret, &res_obj);
	}
}



void access_msg_center::get_option(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	std::string property;
	ret = msg_parser_.option_get_option(msg, property);
	BREAK_IF_NOT_OK(ret);

	if (property == ACCESS_MSG_OPT_MODULE_VERSION) {	
		res_obj.set_string("value", camera_info::get_m_ver());
	} else if (property == ACCESS_MSG_OPT_IMGPARAM) {
		BREAK_IF_CAM_NOT_OPEN(INS_ERR_NOT_ALLOW_OP_IN_STATE);

		std::string value;
		ret  = camera_->get_options(camera_->master_index(), property, value);
		BREAK_IF_NOT_OK(ret);

		res_obj.set_string("value", value);
	} else if (property == ACCESS_MSG_OPT_FLICKER) {
		res_obj.set_int("value", camera_info::get_flicker());
	} else if (property == ACCESS_MSG_OPT_VERSION) {
		res_obj.set_string("value", camera_info::get_ver());
	} else if (property == ACCESS_MSG_OPT_STORAGE_PATH) {
		res_obj.set_string("value", msg_parser_.storage_path_);
	} else if (property == ACCESS_MSG_OPT_STABILIZATION) {
		bool value = false;
		if (video_mgr_) value = video_mgr_->get_stablz_status();
		res_obj.set_bool("value", value);
	} else if (property == ACCESS_MSG_OPT_FANLESS) {
		res_obj.set_bool("value", msg_parser_.b_fanless_);
	} else if (property == ACCESS_MSG_OPT_PANO_AUDIO) {
		int value = 0;
		if (msg_parser_.b_pano_audio_) value++;
		if (msg_parser_.b_audio_to_stitch_) value++;
		res_obj.set_int("value", value);
	} else if (property == ACCESS_MSG_OPT_STABLIZATION_CFG) {
		res_obj.set_bool("value", msg_parser_.b_stab_rt_);
	} else if (property == ACCESS_MSG_OPT_STABLIZATION_TYPE) {
		res_obj.set_int("value", camera_info::get_stablz_type());
	} else if (property == ACCESS_MSG_OPT_LOGO) {
		res_obj.set_bool("value", msg_parser_.b_logo_on_);
	} else if (property == ACCESS_MSG_OPT_VIDEO_FRAGMENT) {
		int value = 1;
		xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_VIDEO_FRAGMENT, value);
		res_obj.set_int("value", value);
	} else if (property == ACCESS_MSG_OPT_BLC_STATE) {
		BREAK_IF_CAM_NOT_OPEN(INS_ERR_NOT_ALLOW_OP_IN_STATE);

		std::string value;
		ret  = camera_->get_options(camera_->master_index(), property, value);
		BREAK_IF_NOT_OK(ret);
		res_obj.set_string("value", value);
	} else if (property == ACCESS_MSG_OPT_DEPTH_MAP) {	
		std::string depth_map_content;
		std::ifstream fs(INS_DEPTH_MAP_FILE, std::ios::ate);
		if (fs.is_open()) {
			int32_t size = fs.tellg();
			fs.seekg(0, std::ios::beg);
			std::unique_ptr<char[]> buff(new char[size]());
			fs.read(buff.get(), size);
			fs.close();
			buff.get()[size-1] = 0; //最后一个换行符去掉
			depth_map_content = buff.get();
		} else {
			LOGERR("file:%s open fail", INS_DEPTH_MAP_FILE);
		}

		res_obj.set_string("value", depth_map_content);
	} else if (property == ACCESS_MSG_OPT_SUPPORT_FUNCTION) {
		json_obj val_obj;

		auto livefmt_array = json_obj::new_array();
		livefmt_array->array_add(std::string("hls"));
		livefmt_array->array_add(std::string("rtsp"));
		livefmt_array->array_add(std::string("rtmp"));
		val_obj.set_obj("liveFormat", livefmt_array.get());
		
		auto projection_array = json_obj::new_array();
		projection_array->array_add(std::string("flat"));
		projection_array->array_add(std::string("cube"));
		val_obj.set_obj("projection", projection_array.get());

		val_obj.set_bool("audioGain", true);
		val_obj.set_bool("gammaCurve", true);
		val_obj.set_bool("pmode", true);
		val_obj.set_bool("p8k5fps", true);
		val_obj.set_bool("blc", true);
		val_obj.set_bool("depthMap", true);
		res_obj.set_obj("value", &val_obj);
	} else if (property == ACCESS_MSG_OPT_AUDIO_GAIN) {
		res_obj.set_int("value", camera_info::get_volume());
	}

	// else if (property == ACCESS_MSG_OPT_IQTYPE)
	// {
	// 	std::string value = camera_info::get_iq_type() + "_" +  camera_info::get_iq_index();
	// 	res_obj.set_string("value", value);
	// }
	else {
		LOGERR("get unsupport option:%s", property.c_str());
		ret = INS_ERR_INVALID_MSG_PARAM;
	}

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);
}



int access_msg_center::set_one_option(std::string property, int value, const std::string& value2)
{
	int ret = INS_OK;

	if (property == ACCESS_MSG_OPT_STABILIZATION) {
		if (video_mgr_) video_mgr_->switch_stablz(value);
		state_mgr_.preview_opt_.b_stabilization = value;
	} else if (property == ACCESS_MSG_OPT_AUDIO_GAIN) {
		ret = camera_info::set_volume(value); 
	} else if (property == "aaa_mode" 
				|| property == "wb" 
				|| property == "iso_value" 
				|| property == "shutter_value" 
				|| property == "brightness"
				|| property == "contrast" 
				|| property == "saturation" 
				|| property == "hue" 
				|| property == "sharpness" 
				|| property == "ev_bias" 
				|| property == "ae_meter"
				|| property == "iso_cap"
				|| property == "long_shutter"
				|| property == "half_sync_level") {
		if (camera_ == nullptr) {
			LOGERR("camera not open");
			return INS_ERR_NOT_ALLOW_OP_IN_STATE;
		}
		ret = camera_->set_options(INS_CAM_ALL_INDEX, property, value);
	} else if (property == ACCESS_MSG_OPT_FLICKER) {
		if (camera_info::get_flicker() == value) {
			return INS_OK;
		} else {
			camera_info::set_flicker(value);
		}
		if (camera_) 
			camera_->set_options(INS_CAM_ALL_INDEX, property, value);
		
	} else if (property == ACCESS_MSG_OPT_FANLESS) {
		msg_parser_.b_fanless_ = (value == 0) ? false : true;
		xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_FANLESS, value);
		LOGINFO("set fanless:%d", msg_parser_.b_fanless_);
	} else if (property == ACCESS_MSG_OPT_PANO_AUDIO) {
		if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_PREVIEW) {
			LOGERR("not allow set audio type in state:0x%x", state_);
			return INS_ERR_NOT_ALLOW_OP_IN_STATE;
		}

		bool b_audio_to_stitch;
		if (value == 0) {
			msg_parser_.b_pano_audio_ = false;
			b_audio_to_stitch = false;
		} else if (value == 1) {
			msg_parser_.b_pano_audio_ = false;
			b_audio_to_stitch = true;
		} else {
			msg_parser_.b_pano_audio_ = true;
			b_audio_to_stitch = true;
		}
		
		xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_PANO_AUDIO, msg_parser_.b_pano_audio_);
		xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_AUDIO_TO_STITCH, b_audio_to_stitch);
		LOGINFO("set pano audio:%d audio to stitch:%d", msg_parser_.b_pano_audio_, b_audio_to_stitch);

		if (msg_parser_.b_audio_to_stitch_ != b_audio_to_stitch) {
			msg_parser_.b_audio_to_stitch_ = b_audio_to_stitch;
			if (state_ == CAM_STATE_PREVIEW) {
				state_mgr_.preview_opt_.b_audio = b_audio_to_stitch;
				do_camera_operation_stop(false); //关预览
				do_camera_operation_stop(true);  //启预览
			}
		}
	} else if (property == ACCESS_MSG_OPT_STABLIZATION_CFG) {
		msg_parser_.b_stab_rt_ = (value == 0)?false:true;
		xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_RT_STAB, value);
		LOGINFO("set rt stablization cfg:%d", msg_parser_.b_stab_rt_);
	} else if (property == ACCESS_MSG_OPT_STABLIZATION_TYPE) {
		xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_STABLZ_TYPE, value);
		camera_info::set_stablz_type(value); 
		LOGINFO("set stablization type:%d", value);
	} else if (property == ACCESS_MSG_OPT_LOGO) {
		msg_parser_.b_logo_on_ = (value == 0)?false:true;
		xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_LOGO, value);
		LOGINFO("set logo:%d", msg_parser_.b_logo_on_);
	} else if (property == ACCESS_MSG_OPT_VIDEO_FRAGMENT) {
		xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_VIDEO_FRAGMENT, value);
		LOGINFO("set video fragment:%d", value);
	} else if (property == ACCESS_MSG_OPT_DEPTH_MAP) {
		ret = set_depth_map(value2);
	}
	// else if (property == ACCESS_MSG_OPT_IQTYPE)
	// {	
	// 	ret = camera_info::set_iq_type(value2);
	// 	if (ret != INS_OK)
	// 	{
	// 		ret = INS_ERR_INVALID_MSG_PARAM;
	// 	}
	// 	else
	// 	{
	// 		LOGINFO("set iq type:%s", value2.c_str());
	// 		if (state_ == CAM_STATE_PREVIEW)
	// 		{
	// 			do_camera_operation_stop(false); //关预览
	// 			do_camera_operation_stop(true);  //启预览
	// 		}
	// 	}
	// }
	else {
		LOGERR("unsupport property:%s", property.c_str());
		ret = INS_ERR_INVALID_MSG_PARAM;
	}

	return ret;
}



void access_msg_center::set_option(const char* msg, std::string cmd, int sequence)
{
	std::map<std::string, int> opt;
	std::map<std::string, std::string> opt2;
	std::map<std::string, int> res;

	DECLARE_AND_DO_WHILE_0_BEGIN

	ret = msg_parser_.option_option(msg, opt, opt2);
	BREAK_IF_NOT_OK(ret);

	for (auto it = opt.begin(); it != opt.end(); it++) {
		int ret_tmp;
		auto it2 = opt2.find(it->first);
		if (it2 != opt2.end()) {
			ret_tmp = set_one_option(it->first, it->second, it2->second);
		} else {
			ret_tmp = set_one_option(it->first, it->second);
		}

		if (ret_tmp != INS_OK) {
			res.insert(std::make_pair(it->first, ret_tmp));
			ret = ret_tmp;
		}
	}

	DO_WHILE_0_END

	//LOGINFO("opt size:%d res size:%d", opt.size(), res.size());

	//1.设置一个 2.设置多个但是全部ok 3.设置多个，但是解析参数出错
	if (opt.size() <= 1 || res.empty()) {
		sender_->send_rsp_msg(sequence, ret, cmd);
	} else {
		json_obj res_obj;
		auto array = json_obj::new_array();
		for (auto it = res.begin(); it != res.end(); it++) { 
			json_obj obj;
			obj.set_string("property", it->first);
			obj.set_int("code", it->second);
			auto des = inserr_to_str(it->second);
			obj.set_string("description", des);
			array->array_add(&obj);
		} 
		
		res_obj.set_obj("detail", array.get());
		if (ret != INS_OK) 
			ret = INS_ERR;
		sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);
	}
}



int access_msg_center::set_depth_map(const std::string& depth_map)
{
	if (depth_map.length() <= 0) {
		LOGERR("set depth map, but depth map is null");
		return INS_ERR_INVALID_MSG_PARAM;
	} 

	std::ofstream fs(INS_DEPTH_MAP_FILE, std::ios::trunc);
	if (fs.is_open()) {
		fs.write(depth_map.data(), depth_map.size());
		fs.close();
		LOGINFO("set depth map");
		return INS_OK;
	} else {
		LOGERR("file:%s open fail", INS_DEPTH_MAP_FILE);
		return INS_ERR_FILE_OPEN;
	}
}



void access_msg_center::restart_preview(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	if (state_ != CAM_STATE_PREVIEW) {
		ret = INS_ERR_NOT_ALLOW_OP_IN_STATE; break;
	}

	ins_video_option opt;
	ret = msg_parser_.preview_option(msg, opt);
	BREAK_IF_NOT_OK(ret);

	state_mgr_.preview_opt_ = opt;

	do_camera_operation_stop(false); 	/* 停止预览 */
	do_camera_operation_stop(true);  	/* 开启预览 */

	res_obj.set_string(ACCESS_MSG_OPT_PREVIEW_URL, opt.stiching.url);

	DO_WHILE_0_END

	sender_->set_ind_msg_sequece(ACCESS_CMD_PREVIEW_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);
}




/***********************************************************************************************
** 函数名称: start_preview
** 函数功能: 开启预览
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
** {
**		"name": 
**		"camera._startPreview", 
**		"parameters": {
**			"audio": {
**				"bitrate": 128, 
**				"channelLayout": "stereo", 
**				"mime": "aac", 
**				"sampleFormat": "s16", 
**				"samplerate": 48000
**			}, 
**			"origin": {
**				"bitrate": 20000, 
**				"framerate": 30, 
**				"height": 1440, 
**				"mime": "h264", 
**				"width": 1920
**			}, 
**			"stiching": {
**				"bitrate": 5000, 
**				"framerate": 30, 
**				"height": 960, 
**				"map": "flat", 
**				"mime": "h264", 
**				"mode": "pano", 
**				"width": 1920
**			}
**		}, 
**		"requestSrc": "ui"
** }
*************************************************************************************************/
void access_msg_center::start_preview(const char* msg, std::string cmd, int sequence)
{	
	DECLARE_AND_DO_WHILE_0_BEGIN

	ins_video_option opt;
	ret = msg_parser_.preview_option(msg, opt);
	BREAK_IF_NOT_OK(ret);

	if (opt.index != -1) {		/* 单镜头合焦HDMI预览(相机需处于IDLE状态) */
		BREAK_NOT_IN_IDLE();			/* 1.状态检查 */
		OPEN_CAMERA_IF_ERR_BREAK(-1);	/* 2.打开模组 */
		singlen_mgr_ = std::make_shared<singlen_mgr>();
		ret = singlen_mgr_->start_focus(camera_.get(), opt);
		if (ret != INS_OK) {
			do_camera_operation_stop(false); break;
		}
	} else {
		BREAK_EXCPET_STATE_2(CAM_STATE_LIVE, CAM_STATE_RECORD);
		if (video_mgr_) {
			ret = video_mgr_->start_preview(opt);
			BREAK_IF_NOT_OK(ret);
		} else {				/* 未启动视频管理器 */
			OPEN_CAMERA_IF_ERR_BREAK(-1);	/* open_camera - 打开模组 */
			video_mgr_ = std::make_shared<video_mgr>();		/* 构造视频管理器 */
			ret = video_mgr_->start(camera_.get(), opt);	/* 启动视频管理器 */
			if (ret != INS_OK) {	/* 启动失败,停止预览 */
				do_camera_operation_stop(false); break;
			}
		}
	}

	state_ |= CAM_STATE_PREVIEW;
	state_mgr_.preview_opt_ = opt;
	res_obj.set_string(ACCESS_MSG_OPT_PREVIEW_URL, opt.stiching.url);
	
	DO_WHILE_0_END

	sender_->set_ind_msg_sequece(ACCESS_CMD_PREVIEW_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);	/* 回复预览请求完成响应 */

	LOGINFO("------ successful enter preview cnt: %d", ++enter_preview_cnt_);
	
}



void access_msg_center::stop_preview(const char* msg, std::string cmd, int sequence)
{
	auto ret = do_stop_preview();
	sender_->send_rsp_msg(sequence, ret, cmd);
}

int access_msg_center::do_stop_preview()
{
	RETURN_NOT_IN_STATE(CAM_STATE_PREVIEW);

	int ret = INS_OK;
	if (state_ & CAM_STATE_RECORD || state_ & CAM_STATE_LIVE) {
		state_ &= ~CAM_STATE_PREVIEW;
		if (video_mgr_) video_mgr_->stop_preview();
	} else if (state_ & CAM_STATE_PIC_SHOOT 
		|| state_ & CAM_STATE_PIC_PROCESS
		|| state_ & CAM_STATE_GYRO_CALIBRATION
		|| state_ & CAM_STATE_QRCODE_SCAN
		|| state_ & CAM_STATE_BLC_CAL 
		|| state_ & CAM_STATE_CALIBRATION 
		|| state_ & CAM_STATE_STOP_RECORD
		|| state_ & CAM_STATE_STOP_LIVE) {
		//这里和do_camera_operation_stop可能会并发，要加锁
		camera_operation_stop_mtx_.lock();
		state_ &= ~CAM_STATE_PREVIEW;
		video_mgr_ = nullptr; //合成的预览
		singlen_mgr_ = nullptr; //单镜头预览
		camera_operation_stop_mtx_.unlock();
	} else if (state_ & CAM_STATE_STORAGE_ST
		|| state_ & CAM_STATE_GYRO_CALIBRATION
		|| state_ & CAM_STATE_MAGMETER_CAL) {
		state_ &= ~CAM_STATE_PREVIEW; //只标记状态
	} else {
		state_ &= ~CAM_STATE_PREVIEW;
		do_camera_operation_stop(true);	 /* 这个函数失败也返回ok,因为预览已经停了 */
	}

	return INS_OK;
}


void access_msg_center::start_record(const char* msg, std::string cmd, int sequence)
{
	ins_video_option opt;
	DECLARE_AND_DO_WHILE_0_BEGIN

	ret = msg_parser_.record_option(msg, opt);
	BREAK_IF_NOT_OK(ret);

	BREAK_EXCPET_STATE(CAM_STATE_PREVIEW);

	if (state_ & CAM_STATE_PREVIEW) {
		ret = do_camera_operation_stop(false);
		BREAK_IF_NOT_OK(ret);
	} else {
		OPEN_CAMERA_IF_ERR_BREAK(-1);
	}
	
	if (opt.timelapse.enable) {		/* 拍摄的是timelapse */
		timelapse_mgr_ = std::make_shared<timelapse_mgr>();
		ret = timelapse_mgr_->start(camera_.get(), opt);
		if (ret != INS_OK) {
			do_camera_operation_stop(true); break;
		}
	} else {
		hw_util::switch_fan(!opt.audio.fanless);

		video_mgr_ = std::make_shared<video_mgr>();
		ret = video_mgr_->start(camera_.get(), opt);
		if (ret != INS_OK) {
			do_camera_operation_stop(true); break;
		}

		// if (opt.audio.fanless) {
		// 	int stop_temp = INS_FANLESS_STOP_TEMP;
		// 	xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_STOP_TEMP, stop_temp);
		// 	LOGINFO("config max temp:%d", stop_temp);
		// 	t_monitor_ = std::make_shared<temp_monitor>(stop_temp);
		// }

		if (state_ & CAM_STATE_PREVIEW) {
			video_mgr_->start_preview(state_mgr_.preview_opt_);
		}
	}

	state_mgr_.option_ = opt;
	state_ |= CAM_STATE_RECORD;

	if (opt.duration > 0) {
		LOGINFO("record duration:%d", opt.duration);
		std::function<void()> func = [this]() 
		{
			LOGINFO("record timeout");
			json_obj obj;
			obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
			obj.set_int("code", INS_OK);
			queue_msg(0, obj.to_string());
		};
		timer_ = std::make_shared<ins_timer>(func, opt.duration);
	}

	state_mgr_.set_time();
	if (opt.index != -1) {
		std::stringstream ss;
		ss << opt.path << "/origin_" << opt.index << ".mp4";
		res_obj.set_string(ACCESS_MSG_OPT_RECORD_URL, ss.str());
	} else {
		res_obj.set_string(ACCESS_MSG_OPT_RECORD_URL, opt.path);
	}
	
	DO_WHILE_0_END

	sender_->set_ind_msg_sequece(ACCESS_CMD_RECORD_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);

	if (ret != INS_OK && opt.path != "") {	/* 由于文件夹是先创建的,所以如果失败删除文件夹,避免出现空文件夹 */
		std::string cmd = "rm -rf " + opt.path;
		system(cmd.c_str());
	}
}


void access_msg_center::stop_record(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_RECORD_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	if (state_ & CAM_STATE_RECORD) {
		long long elapse = state_mgr_.get_elapse_usec();
		if (elapse < 2000000) 
			usleep(2000000 - elapse);//开始录像后马上停止编码器会概率卡死，所以至少跑2s
		do_stop_record(INS_OK);
	} else if (!(state_ & CAM_STATE_STOP_RECORD)) {
		sender_->send_ind_msg(ACCESS_CMD_RECORD_FINISH_, INS_OK);
	}
}



int access_msg_center::do_stop_record(int ret)
{
	timer_ = nullptr;
	//t_monitor_ = nullptr;
	state_ |= CAM_STATE_STOP_RECORD;
	state_ &= ~CAM_STATE_RECORD;
	hw_util::switch_fan(true);

	th_ = std::thread([this, ret] {
		LOGINFO("do stop record");
		do_camera_operation_stop(true);
		usleep(1000*1000); //等待文件存储完成
		json_obj obj;
		obj.set_string("name", INTERNAL_CMD_STOP_REC_F);
		obj.set_int("code", ret);
		queue_msg(0, obj.to_string());
	});
	
	return INS_OK;
}



/***********************************************************************************************
** 函数名称: start_live
** 函数功能: 启动直播
** 入口参数:
**		msg 	 - 消息数据
**		cmd		 - 拍照命令"camera._takePicture"
**		sequence - 通信序列值
** 返 回 值: 成功返回INS_OK;失败返回错误码
*************************************************************************************************/
void access_msg_center::start_live(const char* msg, std::string cmd, int sequence)
{
	ins_video_option opt;
	DECLARE_AND_DO_WHILE_0_BEGIN

	ret = msg_parser_.live_option(msg, opt);	/* 1.解析直播参数 */
	BREAK_IF_NOT_OK(ret);

	BREAK_EXCPET_STATE(CAM_STATE_PREVIEW);		/* 2.相机状态检查 */

	if (state_ & CAM_STATE_PREVIEW) {			/* 如果相机之前处于预览状态下, 先停止预览 */
		ret = do_camera_operation_stop(false);
		BREAK_IF_NOT_OK(ret);
	} else {	/* 如果相机之前处于非预览状态下,需要先开启模组 */
		OPEN_CAMERA_IF_ERR_BREAK(-1);
	}

	video_mgr_ = std::make_shared<video_mgr>();	/* 构建video_mgr并启动 */
	ret = video_mgr_->start(camera_.get(), opt);
	if (ret != INS_OK) {
		do_camera_operation_stop(true); break;
	}

	if (state_ & CAM_STATE_PREVIEW) {			/* 如果之前处于预览状态,重启开启预览流 */
		video_mgr_->start_preview(state_mgr_.preview_opt_);
	}

	state_mgr_.set_time();
	state_mgr_.option_ = opt;
	state_ |= CAM_STATE_LIVE;
	
	b_need_stop_live_ = false;
	b_stop_live_rec_ = false;

	if (opt.path != "") {
		res_obj.set_string(ACCESS_MSG_OPT_FILE_URL, opt.path);
		state_mgr_.b_live_rec_ = true;
	} else {
		state_mgr_.b_live_rec_ = false;
	}

	res_obj.set_string(ACCESS_MSG_OPT_LIVE_URL, opt.stiching.url);

	DO_WHILE_0_END

	sender_->set_ind_msg_sequece(ACCESS_CMD_LIVE_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);

	if (ret != INS_OK && opt.path != "") {	/* 由于文件夹是先创建的,所以如果失败删除文件夹,避免出现空文件夹 */
		std::string cmd = "rm -rf " + opt.path;
		system(cmd.c_str());
	}
	
}


void access_msg_center::stop_live(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_LIVE_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	if (state_ & CAM_STATE_LIVE) {
		long long elapse = state_mgr_.get_elapse_usec();
		if (elapse < 2000000) 
			usleep(2000000 - elapse); /* 开始录像后马上停止编码器会概率卡死，所以至少跑2s */
		do_stop_live(INS_OK, false);
	} else if (!(state_ & CAM_STATE_STOP_LIVE)) {
		sender_->send_ind_msg(ACCESS_CMD_LIVE_FINISH_, INS_OK);
	}
}


int access_msg_center::do_stop_live(int ret, bool b_stop_rec)
{
	if (b_stop_rec) {
		if (b_stop_live_rec_)  {
			return INS_OK;
		} else  {
			b_stop_live_rec_ = true;
		}
	} else {
		if (b_stop_live_rec_) {
			b_need_stop_live_ = true;
			return INS_OK;
		} else {
			state_ &= ~CAM_STATE_LIVE;
			state_ |= CAM_STATE_STOP_LIVE;
		}
	}

	th_ = std::thread([this, ret, b_stop_rec]
	{
		LOGINFO("do stop live, b_stop_rec:%d", b_stop_rec);

		if (b_stop_rec) {
			if (video_mgr_) video_mgr_->stop_live_file();
		} else {
			do_camera_operation_stop(true);
		}

		if (state_mgr_.b_live_rec_) 
			usleep(1000*1000); 	/* 等待文件存储完成 */

		state_mgr_.b_live_rec_ = false;

		json_obj obj;
		obj.set_string("name", INTERNAL_CMD_STOP_LIVE_F);
		obj.set_int("code", ret);
		queue_msg(0, obj.to_string());
	});
	
	return INS_OK;
}



/***********************************************************************************************
** 函数名称: take_picture
** 函数功能: 处理拍照请求
** 入口参数:
**		msg 	 - 消息数据
**		cmd		 - 拍照命令"camera._takePicture"
**		sequence - 通信序列值
** 返 回 值: 成功返回INS_OK;失败返回错误码
*************************************************************************************************/
void access_msg_center::take_picture(const char* msg, std::string cmd, int sequence)
{
	ins_picture_option opt;
	
	DECLARE_AND_DO_WHILE_0_BEGIN

	/*
	 * 1.根据拍照参数来初始化ins_picture_option
	 */
	ret = msg_parser_.take_pic_option(msg, opt);
	BREAK_IF_NOT_OK(ret);

	/*
	 * 2.状态检查(空闲,预览状态下才可以拍照)
	 */
	BREAK_EXCPET_STATE(CAM_STATE_PREVIEW);


	/* 
	 * 3.如果已经处于预览状态(模组已经开启)
	 */
	if (state_ & CAM_STATE_PREVIEW) {
		ret = do_camera_operation_stop(false);
		if (ret != INS_OK) {
			do_camera_operation_stop(true); break;
		}
	} else {	/* 非预览状态(需要先启动模组) */
		OPEN_CAMERA_IF_ERR_BREAK(-1);
	}


	/** 启动img_mgr进行拍照动作 */
	img_mgr_ = std::make_shared<image_mgr>();
	ret = img_mgr_->start(camera_.get(), opt);	/* 启动拍照线程(image_mgr::task) */
	if (ret != INS_OK) {
		do_camera_operation_stop(true); break;
	}

	state_ |= CAM_STATE_PIC_SHOOT;	/* camer进入状态拍照状态 */

	DO_WHILE_0_END

	/* 设置ACCESS_CMD_PIC_FINISH_/ACCESS_CMD_PIC_ORIGIN_FINISH_对应的sequence */
	sender_->set_ind_msg_sequece(ACCESS_CMD_PIC_FINISH_, sequence);
	sender_->set_ind_msg_sequece(ACCESS_CMD_PIC_ORIGIN_FINISH_, sequence);

	/* 给客户端发送一个接收到了拍照请求的响应 */
	sender_->send_rsp_msg(sequence, ret, cmd);

	/* 由于文件夹是先创建的,所以如果失败删除文件夹,避免出现空文件夹 */
	if (ret != INS_OK && opt.path != "") {	
		std::string cmd = "rm -rf " + opt.path;
		system(cmd.c_str());
	}
	
}



void access_msg_center::query_state(const char* msg, std::string cmd, int sequence)
{
	json_obj res_obj;

	std::string state = state_mgr_.state_to_string(state_);

	res_obj.set_string("state", state);
	res_obj.set_string("version", camera_info::get_ver());
	res_obj.set_string("moduleVersion", camera_info::get_m_ver());
	res_obj.set_int("GPSState", gps_mgr::get()->get_state());

	/** 获取当前的options参数 */
	state_mgr_.get_state_param(state_, res_obj);

	sender_->send_rsp_msg(sequence, INS_OK, cmd, &res_obj);
}



//任何时候都可以更新offset，预览的时候可以实时更新，   
//但是录像/直播的时候不能实时更新，只更新到配置文件
void access_msg_center::set_offset(const char* msg, std::string cmd, int sequence)
{
	ins_offset_option opt;
	int ret = msg_parser_.offset_option(msg, opt);
	if (ret != INS_OK) {
		sender_->send_rsp_msg(sequence, ret, cmd);
		return;
	}

	if (opt.using_factory_offset) {
		auto offset = xml_config::get_factory_offset();
		xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3, offset);
		LOGINFO("restore user offset to factory:%s", offset.c_str());
	} else {
		if (opt.factory_setting) {	// 如果是工厂出厂设置 单独存一份
			LOGINFO("set factory setting offset");
			xml_config::set_factory_offset(opt.pano_4_3);
		}

		if (opt.pano_4_3 != "") {
			LOGINFO("pano 4:3 offset:%s", opt.pano_4_3.c_str());
			xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3, opt.pano_4_3);
		}

		// if (opt.pano_16_9 != "") {
		// 	LOGINFO("pano 16:9 offset:%s", opt.pano_16_9.c_str());
		// 	xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9, opt.pano_16_9);
		// }

		// if (opt.left_3d != "")
		// {
		// 	LOGINFO("3d left offset:%s", opt.left_3d.c_str());
		// 	xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT, opt.left_3d);
		// }

		// if (opt.right_3d != "")
		// {
		// 	LOGINFO("3d right offset:%s", opt.right_3d.c_str());
		// 	xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT, opt.right_3d);
		// }
	}

	if (state_ == CAM_STATE_PREVIEW)  {	/* 预览的时候要重启预览立即生效 */
		do_camera_operation_stop(true);
	}

	sender_->send_rsp_msg(sequence, ret, cmd);
}




/***********************************************************************************************
** 函数名称: set_gyro_calibration_res
** 函数功能: 设置陀螺仪拼接结果
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::set_gyro_calibration_res(const char* msg, std::string cmd, int sequence)
{
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	json_obj obj(msg);
	auto param_obj = obj.get_obj(ACCESS_MSG_PARAM);
	if (!param_obj) {
		LOGERR("no key param");
		return;
	}

	auto quat = param_obj->get_double_array("imu_rotation");	/* 从"parameters"中提取IMU信息 */

	xml_config::set_gyro_rotation(quat);
}



/***********************************************************************************************
** 函数名称: get_gyro_calibration_res
** 函数功能: 获取陀螺仪拼接结果
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::get_gyro_calibration_res(const char* msg, std::string cmd, int sequence)
{
	int ret = INS_OK;
	std::vector<double> quat;
	xml_config::get_gyro_rotation(quat);
	if (quat.size() != 4) {
		LOGERR("no gyro imu rotation");
		sender_->send_rsp_msg(sequence, INS_ERR_NOT_EXIST, cmd);
	} else {
		json_obj obj;
		auto array = json_obj::new_array();
		for (uint32_t i = 0; i < quat.size(); i++) {
			array->array_add(quat[i]);	
		}
		obj.set_obj("imu_rotation", array.get());
		sender_->send_rsp_msg(sequence, INS_OK, cmd, &obj);
	}
}


/***********************************************************************************************
** 函数名称: query_battery_temp
** 函数功能: 查询电池温度
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::query_battery_temp(const char* msg, std::string cmd, int sequence)
{
	double temp = 0;
	ins_battery battery;
	//auto ret = battery.open();
	auto ret = battery.read_temp(temp);

	if (ret == INS_OK) {
		json_obj obj;
		obj.set_double("temp", temp);
		sender_->send_rsp_msg(sequence, ret, cmd, &obj);
	} else {
		sender_->send_rsp_msg(sequence, ret, cmd);
	}
}

void access_msg_center::get_offset(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	std::string offset_pano_4_3;
	std::string offset_pano_16_9;
	// std::string offset_3d_left;
	// std::string offset_3d_right;

	xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3, offset_pano_4_3);
	//xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9, offset_pano_16_9);
	// xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT, offset_3d_left);
	// xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT, offset_3d_right);
	if (offset_pano_4_3 == "") {	
		LOGERR("no pano 4:3 offset");
		ret = INS_ERR_CONFIG_FILE;
	}
	// if (offset_pano_16_9 == "")
	// {	
	// 	LOGERR("no pano 16:9 offset");
	// 	ret = INS_ERR_CONFIG_FILE;
	// }
	// if (offset_3d_left == "")
	// {	
	// 	LOGERR("no 3d left offset");
	// 	ret = INS_ERR_CONFIG_FILE;
	// }
	// if (offset_3d_right == "")
	// {	
	// 	LOGERR("no 3d right offset");
	// 	ret = INS_ERR_CONFIG_FILE;
	// }

	res_obj.set_string(ACCESS_MSG_OPT_OFFSET_PANO_4_3, offset_pano_4_3);
	res_obj.set_string(ACCESS_MSG_OPT_OFFSET_PANO_16_9, offset_pano_16_9);
	// res_obj.set_string(ACCESS_MSG_OPT_OFFSET_3D_LEFT, offset_3d_left);
	// res_obj.set_string(ACCESS_MSG_OPT_OFFSET_3D_RIGHT, offset_3d_right);

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);
}



void access_msg_center::get_image_param(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_IF_CAM_NOT_OPEN(INS_ERR_NOT_ALLOW_OP_IN_STATE);

	std::string property;
	ret = camera_->get_options(camera_->master_index(), "imgParam", property);
	BREAK_IF_NOT_OK(ret);

	json_obj obj(property.c_str());
	sender_->send_rsp_msg(sequence, ret, cmd, &obj);
	return;

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}



/***********************************************************************************************
** 函数名称: change_storage_path
** 函数功能: 存储路径发生改变
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::change_storage_path(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	int fix = 0; 
	xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_FIX_STORAGE, fix);
	if (fix == 1) {
		LOGINFO("config to fix storage, so don't change");
		break;
	}

	std::string path;
	ret = msg_parser_.storage_option(msg, path);
	BREAK_IF_NOT_OK(ret);

	/*
	 * 将存储路径写入到cam_config.xml -> storage字段中
	 */
	xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_STORAGE, path);

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}



/***********************************************************************************************
** 函数名称: query_storage
** 函数功能: 查询模组的容量
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::query_storage(const char* msg, std::string cmd, int sequence)
{
	bool b_need_close = false;
	DECLARE_AND_DO_WHILE_0_BEGIN

	if (camera_ == nullptr) {	/* 模组处于close状态 */
		b_need_close = true;
		OPEN_CAMERA_IF_ERR_BREAK(-1);	/* 打开模组 */
	}

	std::vector<std::string> v_value;
	ret = camera_->get_all_options(ACCESS_MSG_OPT_STORAGE_CAP, v_value); //获取模组存储容量
	BREAK_IF_NOT_OK(ret);
	res_obj.set_string("storagePath", msg_parser_.storage_path_);

	auto array = json_obj::new_array();
	for (auto it = v_value.begin(); it != v_value.end(); it++) {
		json_obj obj((*it).c_str());
		array->array_add(&obj);	
	}
	res_obj.set_obj("module", array.get());

	//LOGINFO("storage:%s", res_obj.to_string().c_str());

	DO_WHILE_0_END

	if (b_need_close) close_camera();	

	sender_->send_rsp_msg(sequence, ret, cmd, &res_obj);
}



void access_msg_center::calibration(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_CALIBRATION_ORIGIN_FINISH_, sequence);
	sender_->set_ind_msg_sequece(ACCESS_CMD_CALIBRATION_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);
	
	if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_PREVIEW) {
		sender_->send_ind_msg(ACCESS_CMD_CALIBRATION_FINISH_, INS_ERR_NOT_ALLOW_OP_IN_STATE);
	} else {
		std::string mode;
		int delay = 0;
		int ret = msg_parser_.calibration_option(msg, mode, delay);
		if (ret != 	INS_OK) {
			sender_->send_ind_msg(ACCESS_CMD_CALIBRATION_FINISH_, ret);
		} else {
			ret = do_calibration(mode, delay);
			if (ret != INS_OK) {
				sender_->send_ind_msg(ACCESS_CMD_CALIBRATION_FINISH_, ret);
			} else {
				state_ |= CAM_STATE_CALIBRATION;
			}
		}
	}
}



int32_t access_msg_center::do_calibration(std::string mode, int delay)
{
	DECLARE_AND_DO_WHILE_0_BEGIN	

	if (state_ & CAM_STATE_PREVIEW) {
		ret = do_camera_operation_stop(false);
		BREAK_IF_NOT_OK(ret);
	} else {
		OPEN_CAMERA_IF_ERR_BREAK(-1);
	}

	ins_picture_option opt;
	opt.origin.width = 4000;
	opt.origin.height = 3000;
	opt.origin.mime = INS_JPEG_MIME;
	opt.b_stiching = false;
	opt.b_stabilization = false;
	opt.delay = delay;
 
	img_mgr_ = std::make_shared<image_mgr>();
	ret = img_mgr_->start(camera_.get(), opt, true);
	if (ret != INS_OK) {
		do_camera_operation_stop(true); break;
	}

	DO_WHILE_0_END

	return ret;
}

void access_msg_center::start_qr_scan(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	if (state_ == CAM_STATE_IDLE) {
		OPEN_CAMERA_IF_ERR_BREAK(-1);
	} else if (state_ == CAM_STATE_PREVIEW) {
		ret = do_camera_operation_stop(false);
		BREAK_IF_NOT_OK(ret);
	} else {
		ret = INS_ERR_NOT_ALLOW_OP_IN_STATE; break;
	}

	qr_scanner_ = std::make_shared<qr_scanner>();
	ret = qr_scanner_->open(camera_);
	if (ret != INS_OK) {
		do_camera_operation_stop(true); break;
	}

	state_ |= CAM_STATE_QRCODE_SCAN;
	
	DO_WHILE_0_END

	sender_->set_ind_msg_sequece(ACCESS_CMD_QR_SCAN_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::stop_qr_scan(const char* msg, std::string cmd, int sequence)
{
	sender_->send_rsp_msg(sequence, INS_OK, cmd);
	if (!(state_ & CAM_STATE_QRCODE_SCAN)) return;
	do_stop_qr_scan();
	sender_->send_ind_msg(ACCESS_CMD_QR_SCAN_FINISH_, INS_OK);
}

void access_msg_center::internal_qr_scan_finish(const char* msg, std::string cmd, int sequence)
{
	if (!(state_ & CAM_STATE_QRCODE_SCAN)) return;
	do_stop_qr_scan();
	auto root_obj = std::make_shared<json_obj>(msg);
	std::string content;
	root_obj->get_string("content", content);

	char* pdst = new char[content.length()]();
	const char* psrc = content.c_str();
	for (unsigned int i = 0; i < content.length()/3; i++) {
		pdst[i] = strtol(psrc+i*3, nullptr, 16);
	}

	json_obj root(pdst);
	json_obj res_obj;
	if (root.is_obj()) {
		res_obj.set_string("content", pdst);
	} else {
		LOGERR("not valid json string");
		res_obj.set_string("content", content);
	}

	sender_->send_ind_msg(ACCESS_CMD_QR_SCAN_FINISH_, INS_OK, &res_obj);

	delete[] pdst;
}

void access_msg_center::do_stop_qr_scan()
{ 
	state_ &= ~CAM_STATE_QRCODE_SCAN;
	do_camera_operation_stop(true);
}

void access_msg_center::upgrade_fw(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_IDLE();

	std::string path;
	std::string version;
	ret = msg_parser_.upgrade_fw_option(msg, path, version);
	BREAK_IF_NOT_OK(ret);
	OPEN_CAMERA_IF_ERR_BREAK(-1);
	ret = camera_->upgrade(path, version);

	close_camera();
	
	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::calibration_awb(const char* msg, std::string cmd, int sequence)
{
	bool b_need_close_cam = false;

	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_IDLE();

	b_need_close_cam = true;
	OPEN_CAMERA_IF_ERR_BREAK(INS_CAM_MASTER_INDEX);

	std::string uuid;
	std::string sensor_id;
	ret = camera_->get_uuid(camera_->master_index(), uuid, sensor_id);
	if (uuid == "") {
		uuid = ins_uuid_gen();
		LOGINFO("gen uuid:%s", uuid.c_str());
		ret = camera_->set_uuid(camera_->master_index(), uuid);
		BREAK_IF_NOT_OK(ret);
	}

	int maxvalue[4] = {0};
	std::shared_ptr<insbuff> buff;
	ret = camera_->calibration_awb(camera_->master_index(), 0,0, 0, sensor_id, maxvalue, buff);
	BREAK_IF_NOT_OK(ret);

	ret = camera_->get_uuid(camera_->master_index(), uuid, sensor_id);
	if (uuid == "") {
		ret = INS_ERR; break;
	}

	system("mkdir -p /home/nvidia/data");
	std::string file_name = "/home/nvidia/data/awb.dat";
	std::ofstream fs(file_name, std::ios::binary|std::ios::trunc);
	if (fs.is_open()) {
		fs.write((char*)buff->data(), buff->size());
		fs.close();
	} else {
		LOGERR("file:%s open fail", file_name.c_str());
	}

	ret = upload_cal_file::upload_file(file_name, uuid, sensor_id, maxvalue);
	BREAK_IF_NOT_OK(ret);

	DO_WHILE_0_END

	if (b_need_close_cam) 
		close_camera();
	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::calibration_bpc(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_BPC_RESULT, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_PREVIEW) {
		sender_->send_ind_msg(ACCESS_CMD_BPC_RESULT, INS_ERR_NOT_ALLOW_OP_IN_STATE);
	} else {
		state_ |= CAM_STATE_BPC_CAL; 
		th_ = std::thread(&access_msg_center::do_bpc_calibration, this);
	}
}

void access_msg_center::do_bpc_calibration()
{
	int32_t ret = INS_OK;
	if (state_ & CAM_STATE_PREVIEW) {
		ret  = do_camera_operation_stop(false); //关预览
		if (ret == INS_OK) {
			ret = camera_->calibration_bpc_all();
			do_camera_operation_stop(true); //启预览
		}
	} else {
		ret = open_camera(-1);
		if (ret == INS_OK) {
			ret = camera_->calibration_bpc_all();
			close_camera();
		}
	}

	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_BPC_CAL_F);
	obj.set_int("code", ret);
	queue_msg(0, obj.to_string());
}

void access_msg_center::internal_bpc_calibration_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);
	state_ &= ~CAM_STATE_BPC_CAL;

	int ret;
	json_obj root_obj(msg);
	root_obj.get_int("code", ret);
	
	sender_->send_ind_msg(ACCESS_CMD_BPC_RESULT, ret);
}

//1.只有在preview或者idle状态下才可以进行blc矫正
//2.preview的时候进行blc先要停止预览
void access_msg_center::calibration_blc(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_BLC_RESULT, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	bool b_reset = false;
	msg_parser_.calibration_blc_option(msg, b_reset);

	if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_PREVIEW) {
		sender_->send_ind_msg(ACCESS_CMD_BLC_RESULT, INS_ERR_NOT_ALLOW_OP_IN_STATE);
	} else {
		state_ |= CAM_STATE_BLC_CAL; 
		th_ = std::thread(&access_msg_center::do_blc_calibration, this, b_reset);
	}
}

void access_msg_center::do_blc_calibration(bool b_reset)
{
	LOGINFO("blc calibration reset:%d", b_reset);

	int ret = INS_OK;
	if (state_ & CAM_STATE_PREVIEW) {
		if (!b_reset) {
			ret  = do_camera_operation_stop(false); //关预览
			if (ret == INS_OK) {
				ret = camera_->calibration_blc_all(b_reset);
				do_camera_operation_stop(true); //启预览
			}
		} else {
			//重置不需要关预览，顺便记录一下：重置时间很快
			ret = camera_->calibration_blc_all(b_reset);
		}
	} else {
		ret = open_camera(-1);
		if (ret == INS_OK) {
			ret = camera_->calibration_blc_all(b_reset);
			close_camera();
		}
	}

	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_BLC_CAL_F);
	obj.set_int("code", ret);
	queue_msg(0, obj.to_string());
}

void access_msg_center::internal_blc_calibration_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);
	state_ &= ~CAM_STATE_BLC_CAL;

	int ret;
	json_obj root_obj(msg);
	root_obj.get_int("code", ret);
	
	sender_->send_ind_msg(ACCESS_CMD_BLC_RESULT, ret);
}

void access_msg_center::update_gamma_curve(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	std::string s_data;
	ret = msg_parser_.update_gamma_curve_option(msg, s_data);
	BREAK_IF_NOT_OK(ret);

	BREAK_IF_CAM_NOT_OPEN(INS_ERR_NOT_ALLOW_OP_IN_STATE);

	if (s_data == "")  {	// 恢复gama
		ret = camera_->send_all_buff_data(nullptr, 0, AMBA_FRAME_GAMMA_CURVE, 5000);
	} else {
		auto buff = ins_base64::decode(s_data);
		if (buff->offset() > 512) buff->set_offset(512);
		ret = camera_->send_all_buff_data(buff->data(), buff->offset(), AMBA_FRAME_GAMMA_CURVE, 5000); 
	}

	// if (s_data == "") //恢复gama
	// {
	// 	if (camera_info::get_gamma_default())
	// 	{
	// 		LOGINFO("restore gamma and gamma is default");
	// 		ret = INS_OK;
	// 	}
	// 	else if (state_ == CAM_STATE_PREVIEW) //恢复gama，模组要关闭预览
	// 	{
	// 		ret = camera_->send_all_buff_data(nullptr, 0, AMBA_FRAME_GAMMA_CURVE, 5000);
	// 		if (ret == INS_OK) camera_info::set_gamma_default(true);
	// 		do_camera_operation_stop(true);
	// 	}
	// 	else //如果是录像等其他状态，不允许操作
	// 	{
	// 		ret = INS_ERR_NOT_ALLOW_OP_IN_STATE;
	// 	}
	// }
	// else
	// {
	// 	auto buff = ins_base64::decode(s_data);
	// 	if (buff->offset() > 512) buff->set_offset(512);
	// 	ret = camera_->send_all_buff_data(buff->data(), buff->offset(), AMBA_FRAME_GAMMA_CURVE, 5000); 
	// 	if (ret == INS_OK) camera_info::set_gamma_default(false);
	// }

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}



void access_msg_center::format_camera_moudle(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_IDLE();

	int pid;
	ret = msg_parser_.format_camera_moudle_option(msg, pid);
	BREAK_IF_NOT_OK(ret);

	OPEN_CAMERA_IF_ERR_BREAK(-1);

	LOGINFO("format flash pid:%d", pid);
	ret = camera_->format_flash(camera_->get_index(pid));

	close_camera();
	
	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}



void access_msg_center::get_module_log_file(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_IDLE();

	std::string file_name;
	ret = msg_parser_.module_log_file_option(msg, file_name);
	BREAK_IF_NOT_OK(ret);

	OPEN_CAMERA_IF_ERR_BREAK(-1);
	ret = camera_->get_log_file(file_name);
	close_camera();

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::power_off_req(const char* msg, std::string cmd, int sequence)
{
	// bool b_rec_finish_s = false;
	// bool b_live_finish_s = false;
	// if (state_ & CAM_STATE_STOP_RECORD) b_rec_finish_s = true;
	// if (state_ & CAM_STATE_STOP_LIVE) b_live_finish_s = true;

	state_ = CAM_STATE_IDLE;

	do_camera_operation_stop(true);

	// if (b_rec_finish_s) sender_->send_ind_msg(ACCESS_CMD_RECORD_FINISH_, INS_OK);
	// if (b_live_finish_s) sender_->send_ind_msg(ACCESS_CMD_LIVE_FINISH_, INS_OK);
	
	if (task_mgr_) task_mgr_ = nullptr;

	hw_util::switch_fan(true);

	sender_->send_rsp_msg(sequence, INS_OK, cmd);
}

void access_msg_center::storage_test_1(const char* msg, std::string cmd, int sequence)
{
	if (msg_parser_.storage_path_ == "") return;

	json_obj root_obj(msg);
	auto param_obj = root_obj.get_obj(ACCESS_MSG_PARAM);
	if (param_obj == nullptr) {
		LOGERR("no param key");
		return;
	}

	int32_t file_cnt = 6;
	int32_t block_size = 32;//k
	int64_t total_size = 4;//G
	param_obj->get_int("file_cnt", file_cnt);
	param_obj->get_int("block_size", block_size);
	param_obj->get_int64("total_size", total_size);

	storage_test(msg_parser_.storage_path_, file_cnt, block_size*1024, total_size*1024*1024*1024);
}

void access_msg_center::test_storage_speed(const char* msg, std::string cmd, int sequence)
{
	/* 发送响应,表示接收到了测速命令 */
	sender_->set_ind_msg_sequece(ACCESS_CMD_STORAGE_ST_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	std::string path;
	msg_parser_.storage_speed_test_option(msg, path);
	if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_PREVIEW) {	/* 非空闲或预览装,返回413错误 */
		sender_->send_ind_msg(ACCESS_CMD_STORAGE_ST_FINISH_, INS_ERR_NOT_ALLOW_OP_IN_STATE);
	} else {
		int32_t ret = INS_OK;
		if (state_ & CAM_STATE_PREVIEW) {		/* 预览状态下,测速会先停止预览 */
			ret = do_camera_operation_stop(false);
		} else {
			ret = open_camera(-1);
		}

		if (ret != INS_OK)  {
			sender_->send_ind_msg(ACCESS_CMD_STORAGE_ST_FINISH_, ret);
		} else {
			state_ |= CAM_STATE_STORAGE_ST;		/* 进入测速状态 */
			th_ = std::thread(&access_msg_center::do_test_storage_speed, this, path);
		}
	}
}

void access_msg_center::do_test_storage_speed(std::string path)
{
	LOGINFO("do storage speed test, path[%s]", path.c_str());

	std::map<int32_t, bool> map_res;	
	std::future<int> f_module = std::async(
			std::launch::async, 
			[this, &map_res]() -> int
			{
				return camera_->storage_speed_test(map_res);
			});

	std::future<int> f_local = std::async(
			std::launch::async, 
			[](std::string path) -> int 
			{
				if (path == "") return INS_OK; //路径为空表示大卡不用测速
				storage_speed_test sst;
				return sst.test(path);
			}, 
			path);

	auto ret_local = f_local.get();
	auto ret_module = f_module.get();
	if (ret_module != INS_OK) {
		for (int32_t i = 0; i < INS_CAM_NUM; i++) {
			map_res.insert(std::make_pair(i, false));
		}
	}

	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_STORAGE_ST_F);
	queue_msg(0, obj.to_string());

	json_obj param_obj;
	param_obj.set_bool("local", (ret_local == INS_OK));	/* 设置本地测速的状态(true/false) */
	
	auto array = json_obj::new_array();		/* 设置各模组的测速结果 */
	for (auto& it : map_res) {
		json_obj t_obj;
		t_obj.set_int("index", it.first);
		t_obj.set_bool("result", it.second);
		array->array_add(&t_obj);
	}
	param_obj.set_obj("module", array.get());

	//sender_->send_ind_msg_(ACCESS_CMD_STORAGE_ST_FINISH_, &param_obj);
	sender_->send_ind_msg(ACCESS_CMD_STORAGE_ST_FINISH_, INS_OK, &param_obj);
}


void access_msg_center::internal_storage_st_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);
	state_ &= ~CAM_STATE_STORAGE_ST;
	do_camera_operation_stop(true);
	//sender_->send_ind_msg(ACCESS_CMD_STORAGE_ST_FINISH_, ret);
}

void access_msg_center::gyro_calibration(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_GYRO_CAL_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_PREVIEW) {
		sender_->send_ind_msg(ACCESS_CMD_GYRO_CAL_FINISH_, INS_ERR_NOT_ALLOW_OP_IN_STATE);
	} else {
		hw_util::switch_fan(false);
		usleep(1000*1000); //风扇停止需要时间

		int32_t ret = INS_OK;
		if (state_ & CAM_STATE_PREVIEW) {
			ret = do_camera_operation_stop(false);
		} else {
			ret = open_camera(-1);
		}

		if (ret != INS_OK)  {
			hw_util::switch_fan(true);
			sender_->send_ind_msg(ACCESS_CMD_GYRO_CAL_FINISH_, ret);
		} else {
			state_ |= CAM_STATE_GYRO_CALIBRATION; 
			th_ = std::thread(&access_msg_center::do_gyro_calibration, this);
		}
	}
}

void access_msg_center::do_gyro_calibration()
{
	auto ret = camera_->gyro_calibration();

	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_GYRO_CALIBRATION_F);
	obj.set_int("code", ret);
	queue_msg(0, obj.to_string());
}

void access_msg_center::internal_gyro_calibration_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);
	state_ &= ~CAM_STATE_GYRO_CALIBRATION;

	hw_util::switch_fan(true);
	do_camera_operation_stop(true);

	int ret;
	json_obj root_obj(msg);
	root_obj.get_int("code", ret);
	
	sender_->send_ind_msg(ACCESS_CMD_GYRO_CAL_FINISH_, ret);
}

void access_msg_center::magmeter_calibration(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_MAGMETER_CAL_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_PREVIEW) {
		sender_->send_ind_msg(ACCESS_CMD_MAGMETER_CAL_FINISH_, INS_ERR_NOT_ALLOW_OP_IN_STATE);
	} else {
		hw_util::switch_fan(false);
		usleep(1000*1000); //风扇停止需要时间

		int32_t ret = INS_OK;
		if (state_ & CAM_STATE_PREVIEW) {
			ret = do_camera_operation_stop(false);
		} else {
			ret = open_camera(-1);
		}

		if (ret != INS_OK)  {
			hw_util::switch_fan(true);
			sender_->send_ind_msg(ACCESS_CMD_MAGMETER_CAL_FINISH_, ret);
		} else {
			state_ |= CAM_STATE_MAGMETER_CAL; 
			th_ = std::thread(&access_msg_center::do_magmeter_calibration, this);
		}
	}
}

void access_msg_center::do_magmeter_calibration()
{	
	auto ret = camera_->magmeter_calibration();

	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_MAGMETER_CALIBRATION_F);
	obj.set_int("code", ret);
	queue_msg(0, obj.to_string());
}

void access_msg_center::internal_magmeter_calibration_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);
	state_ &= ~CAM_STATE_MAGMETER_CAL;

	hw_util::switch_fan(true);
	do_camera_operation_stop(true);

	int ret;
	json_obj root_obj(msg);
	root_obj.get_int("code", ret);
	
	sender_->send_ind_msg(ACCESS_CMD_MAGMETER_CAL_FINISH_, ret);
}

void access_msg_center::start_magmeter_calibration(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_EXCPET_STATE(CAM_STATE_MODULE_POWON);

	if (camera_ == nullptr) {
		LOGERR("camera not open");
		ret = INS_ERR_CAMERA_NOT_OPEN;
		break;
	}

	hw_util::switch_fan(false);
	usleep(2000*1000); //风扇停止需要时间

	ret = camera_->start_magmeter_calibration();
	BREAK_IF_NOT_OK(ret);

	state_ |= CAM_STATE_MAGMETER_CAL; 

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::stop_magmeter_calibration(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_STATE(CAM_STATE_MAGMETER_CAL);

	state_ &= ~CAM_STATE_MAGMETER_CAL;

	ret = camera_->stop_magmeter_calibration();
	BREAK_IF_NOT_OK(ret);

	DO_WHILE_0_END

	hw_util::switch_fan(true);
	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::delete_file(const char* msg, std::string cmd, int sequence)
{
	sender_->set_ind_msg_sequece(ACCESS_CMD_DEL_FILE_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	std::string dir;
	json_obj obj(msg);
	auto param_obj = obj.get_obj(ACCESS_MSG_PARAM);
	if (param_obj) param_obj->get_string("dir", dir);

	int32_t ret = INS_OK;
	if (camera_ == nullptr) {
		sender_->send_ind_msg(ACCESS_CMD_DEL_FILE_FINISH_, INS_ERR_NOT_ALLOW_OP_IN_STATE);
	} else {
		state_ |= CAM_STATE_DELETE_FILE;
		th_ = std::thread(&access_msg_center::do_delete_file, this, dir);
	}
}

void access_msg_center::do_delete_file(std::string dir)
{	
	auto ret = camera_->delete_file(dir);

	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_DEL_FILE_F);
	obj.set_int("code", ret);
	queue_msg(0, obj.to_string());
}

void access_msg_center::internal_delete_file_finish(const char* msg, std::string cmd, int sequence)
{
	INS_THREAD_JOIN(th_);
	state_ &= ~CAM_STATE_DELETE_FILE;

	int ret;
	json_obj root_obj(msg);
	root_obj.get_int("code", ret);
	
	sender_->send_ind_msg(ACCESS_CMD_DEL_FILE_FINISH_, ret);
}


/***********************************************************************************************
** 函数名称: module_power_on
** 函数功能: 请求进入CAM_STATE_MODULE_POWON状态
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::module_power_on(const char* msg, std::string cmd, int sequence)
{
	int32_t ret;
	
	if (state_ == CAM_STATE_MODULE_POWON) {	
		ret = INS_OK;
	} else if (state_ != CAM_STATE_IDLE) {
		ret = INS_ERR_NOT_ALLOW_OP_IN_STATE;
	} else {
		ret = open_camera(-1);
	}
	sender_->send_rsp_msg(sequence, ret, cmd);
	
	if (ret == INS_OK) 
		state_ = CAM_STATE_MODULE_POWON;
}


/***********************************************************************************************
** 函数名称: module_power_off
** 函数功能: 请求进入CAM_STATE_MODULE_POWON状态
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::module_power_off(const char* msg, std::string cmd, int sequence)
{
	int32_t ret = INS_OK;
	if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_MODULE_POWON) {	/* 非IDLE和非CAM_STATE_MODULE_POWON状态返回413 */
		ret = INS_ERR_NOT_ALLOW_OP_IN_STATE;
	} else {
		state_ &= ~CAM_STATE_MODULE_POWON;
		close_camera();
	}
	sender_->send_rsp_msg(sequence, ret, cmd);
}



/***********************************************************************************************
** 函数名称: change_udisk_mode
** 函数功能: 请求进入/退出U盘模式
** 入口参数:
**		msg - 请求参数
**		cmd - 命令
**		sequence - 响应序列
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::change_udisk_mode(const char* msg, std::string cmd, int sequence)
{
	json_obj obj(msg);
	auto param_obj = obj.get_obj(ACCESS_MSG_PARAM);
	if (!param_obj) {
		sender_->send_rsp_msg(sequence, INS_ERR_INVALID_MSG_PARAM, cmd);
		return;
	}
	int32_t mode = 0; 		/* 0:退出u盘 1:进入u盘 */
	param_obj->get_int("mode", mode);
	LOGINFO("change udisk mode:%d", mode);

	if (mode) {		/* 请求进入U盘模式 */
		if (state_ == CAM_STATE_MODULE_POWON) {	/* 如果处于相册页中(相册页中App会设置CAM_STATE_MODULE_POWON状态) */
			state_ &= ~CAM_STATE_MODULE_POWON;
			close_camera();
		} else if (state_ != CAM_STATE_IDLE && state_ != CAM_STATE_UDISK_MODE) {	/* 非空闲状态并且非U盘状态: 返回413 */
			sender_->send_rsp_msg(sequence, INS_ERR_NOT_ALLOW_OP_IN_STATE, cmd);
			return;
		}
		state_ |= CAM_STATE_UDISK_MODE;
	} else {		/* 请求退出U盘模式 */
		state_ &= ~CAM_STATE_UDISK_MODE;
	}
	sender_->send_rsp_msg(sequence, INS_OK, cmd);
}



void access_msg_center::test_module_communication(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_IDLE();

	OPEN_CAMERA_IF_ERR_BREAK(-1);

	std::vector<std::string> v_version;
	ret = camera_->get_version(INS_CAM_ALL_INDEX, v_version);
	close_camera();

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::test_module_spi(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_IDLE();

	OPEN_CAMERA_IF_ERR_BREAK(-1);
	ret = camera_->test_spi(camera_->master_index());
	close_camera();

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::change_module_usb_mode(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	json_obj root(msg);
	auto obj = root.get_obj(ACCESS_MSG_PARAM);
	BREAK_IF_TRUE(!obj, "param obj is null");
	int32_t mode = 0;
	obj->get_int("mode", mode);
	LOGINFO("change module usb mode:%d", mode);

	if (mode) {	//storage mode
		BREAK_NOT_IN_IDLE();

		OPEN_CAMERA_IF_ERR_BREAK(-1);
	
		ret = camera_->change_all_usb_mode();

		close_camera();

		if (ret == INS_OK) {
			cam_manager::power_on_all();
			state_ = CAM_STATE_MODULE_STORAGE;
		}
	} else {	// no storage mode
		if (!(state_ & CAM_STATE_MODULE_STORAGE)) break;
		state_ &= ~CAM_STATE_MODULE_STORAGE;
		cam_manager::power_off_all();
	}

	DO_WHILE_0_END

	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::query_gps_status(const char* msg, std::string cmd, int sequence)
{
	auto data = gps_mgr::get()->get_gps();
	json_obj obj;
	obj.set_int("state", gps_mgr::get()->get_state());
	if (data == nullptr) {
		obj.set_int("fix_type", 0);
		json_obj s_obj;
		s_obj.set_int("sv_num", 0);
		obj.set_obj("sv_status", &s_obj);
	} else {
		obj.set_int("fix_type", data->fix_type);
		json_obj s_obj;
		s_obj.set_int("sv_num", data->sv_status.sv_num);
		std::stringstream ss << data->sv_status.sv_num;
		property_set("sys.sv_num", ss.str());

		auto array = json_obj::new_array();
		for (int32_t i = 0; i < data->sv_status.sv_num; i++) {
			json_obj t_obj;
			t_obj.set_int("prn", data->sv_status.sv_list[i].prn);
			t_obj.set_double("snr", data->sv_status.sv_list[i].snr);
			t_obj.set_double("elevation", data->sv_status.sv_list[i].elevation);
			t_obj.set_double("azimuth", data->sv_status.sv_list[i].azimuth);
			array->array_add(&t_obj);	
		}
		s_obj.set_obj("status", array.get());
		obj.set_obj("sv_status", &s_obj);
	}

	sender_->send_rsp_msg(sequence, INS_OK, cmd, &obj);
}

void access_msg_center::start_capture_audio(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	BREAK_NOT_IN_IDLE();

	audio_rec_ = std::make_shared<audio_record>();
	audio_rec_->start();

	state_ |= CAM_STATE_AUDIO_CAPTURE;

	DO_WHILE_0_END

	sender_->set_ind_msg_sequece(ACCESS_CMD_CAPTURE_AUDIO_FINISH_, sequence);
	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::stop_capture_audio(const char* msg, std::string cmd, int sequence)
{
	audio_rec_ = nullptr;
	state_ &= ~CAM_STATE_AUDIO_CAPTURE;
	sender_->send_rsp_msg(sequence, INS_OK, cmd);
}

void access_msg_center::internal_capture_audio_finish(const char* msg, std::string cmd, int sequence)
{
	audio_rec_ = nullptr;
	state_ &= ~CAM_STATE_AUDIO_CAPTURE;
	sender_->send_ind_msg(ACCESS_CMD_CAPTURE_AUDIO_FINISH_, INS_OK);	
}



/***********************************************************************************************
** 函数名称: system_time_change
** 函数功能: 系统时间发生改变
** 入口参数:
** 返 回 值: 成功返回INS_OK;失败返回错误码
*************************************************************************************************/
void access_msg_center::system_time_change(const char* msg, std::string cmd, int sequence)
{
	int64_t timestamp = 0;
	std::string source;
	msg_parser_.systime_change_option(msg, timestamp, source);

	/* 如果APP没有设置过时间,那么设置时间为GPS的时间 */
	if (source == "gps" && timestamp && state_ == CAM_STATE_IDLE && !camera_info::is_sync_time()) {
		
		time_t t_time = timestamp;
        struct tm *tm_time = localtime(&timestamp);
		std::stringstream ss;
		std::string back_tz = property_get("sys.timezone");

		
		ss << "date -s \"" 
		    << tm_time->tm_year + 1900 << "-"
			<< tm_time->tm_mon + 1 << "-"
			<< tm_time->tm_mday << " " 
			<< tm_time->tm_hour << ":"
			<< tm_time->tm_min << ":"
			<< tm_time->tm_sec << "\"";

        /* 
         * - 切换系统的时区为UTC时区 
         * - 设置本地时间
         * - 切换回系统原来的时区
         */
		LOGINFO("--> Switch current timezone to UTC first, backup timezone[%s]", back_tz);
        system("timedatectl set-timezone UTC");
		
		LOGINFO("%s", ss.str().c_str());
		system(ss.str().c_str());

		if (back_tz != "") {
			std::string set_tz = "timedatectl set-timezone " + back_tz;
			system(set_tz.c_str());
		}

		camera_info::sync_time();
	}
	
	if (source == "") {	/* 表示是APP设置时间  */
		camera_info::sync_time(); 						/* APP设置时间,要标记 */
		sender_->send_rsp_msg(sequence, INS_OK, cmd); 	/* 自己设置时间不用回复消息 */
	}
}




/***********************************************************************************************
** 函数名称: open_camera
** 函数功能: 打开模组
** 入口参数:
**		index - 打开的指定的模组(-1代表所有的模组)
** 返 回 值: 成功返回INS_OK;失败返回错误码
*************************************************************************************************/
int access_msg_center::open_camera(int index)
{
	/* 模组处于打开状态,直接返回(如果上次打开一个模组,这次需要打开所有模组??) */
	if (camera_ != nullptr) 
		return INS_OK;	

	/*
	 * 对模组进行下电/上电操作
	 */
	cam_manager::power_off_all();
	usleep(5*1000);
	cam_manager::power_on_all();

	/*
	 * 构造cam_manager并打开模组
	 */
	camera_ = std::make_shared<cam_manager>();

	int ret = INS_OK;
	if (index == INS_CAM_ALL_INDEX) {	/* 打开的所有camera */
		ret = camera_->open_all_cam();
	} else if (index == INS_CAM_MASTER_INDEX) {	/* 打开master */
		std::vector<unsigned int> v_index;
		v_index.push_back(camera_->master_index());
		ret = camera_->open_cam(v_index);
	}

	if (INS_OK != ret) {
		cam_manager::power_off_all();
		camera_ = nullptr;
		return ret;
	} else {
		return INS_OK;
	}
}



/***********************************************************************************************
** 函数名称: close_camera
** 函数功能: 关闭模组
** 入口参数:
** 返 回 值: 无
*************************************************************************************************/
void access_msg_center::close_camera()
{
	/* camera处于忙碌状态, 直接返回  */
	if (state_ & CAM_STATE_RECORD 
		|| state_ & CAM_STATE_LIVE 
		|| state_ & CAM_STATE_PREVIEW
		|| state_ & CAM_STATE_PIC_PROCESS
		|| state_ & CAM_STATE_PIC_SHOOT) {
		return;
	}

	camera_ = nullptr;
	cam_manager::power_off_all();
	camera_info::set_volume(INS_DEFAULT_AUDIO_GAIN); 	/* 恢复默认gain */
}


int access_msg_center::do_camera_operation_stop(bool restart_preview)
{
	std::lock_guard<std::mutex> lock(camera_operation_stop_mtx_);

	video_mgr_ 		= nullptr;
	img_mgr_ 		= nullptr;
	timelapse_mgr_ 	= nullptr;
	qr_scanner_ 	= nullptr;
	singlen_mgr_ 	= nullptr;

	sleep(1);

	auto ret = INS_OK;
	if (camera_) 
		ret = camera_->exception();
	
	if (ret != INS_OK)  {
		if (state_ & CAM_STATE_PREVIEW) {
			state_ &= ~CAM_STATE_PREVIEW;
			sender_->send_ind_msg(ACCESS_CMD_PREVIEW_FINISH_, ret);
		}
		close_camera();
		return ret;
	}
	
	if (restart_preview) {
		if (state_ & CAM_STATE_PREVIEW) {
			if (state_mgr_.preview_opt_.index == -1) {
				video_mgr_ = std::make_shared<video_mgr>();
				ret = video_mgr_->start(camera_.get(), state_mgr_.preview_opt_);	/* 重新启动预览 */
			} else {	/* 单镜头合焦预览 */
				singlen_mgr_ = std::make_shared<singlen_mgr>();
				ret = singlen_mgr_->start_focus(camera_.get(), state_mgr_.preview_opt_);
			}

			if (ret != INS_OK) {
				state_ &= ~CAM_STATE_PREVIEW;
				video_mgr_ = nullptr;
				singlen_mgr_ = nullptr;
				sender_->send_ind_msg(ACCESS_CMD_PREVIEW_FINISH_, ret);
				close_camera();
				return ret;
			}
		} else {
			close_camera();
		}
	}

	return INS_OK;
}



/*------------------------stitching box-----------------------------*/

#if 0
void access_msg_center::s_start_box(const char* msg, std::string cmd, int sequence)
{
	DECLARE_AND_DO_WHILE_0_BEGIN

	if (state_ != CAM_STATE_STITCHING_BOX && state_ != CAM_STATE_IDLE)
	{
		ret = INS_ERR_NOT_ALLOW_OP_IN_STATE;
		break;
	}

	task_mgr_ = std::make_shared<box_task_mgr>();
	ret = task_mgr_->open();
	BREAK_IF_NOT_OK(ret);
	// ret = task_mgr_->start_task_list(); //进入box就要开始列表
	// BREAK_IF_NOT_OK(ret);
	state_ |= CAM_STATE_STITCHING_BOX;

	DO_WHILE_0_END
	
	if (ret != INS_OK) task_mgr_ = nullptr;

	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::s_stop_box(const char* msg, std::string cmd, int sequence)
{
	task_mgr_ = nullptr;
	state_ &= ~CAM_STATE_STITCHING_BOX;
	sender_->send_rsp_msg(sequence, INS_OK, cmd);

	//system("pkill mediaserver");
	//system("pkill media");
}

void access_msg_center::s_query_task_list(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;

	std::string list;
	bool b_list_start;
	int ret = task_mgr_->query_task_list(list, b_list_start);
	if (ret != INS_OK)
	{
		sender_->send_rsp_msg(sequence, ret, cmd);
	}
	else
	{
		std::stringstream ss;
		ss << "{\"list_started\":" << std::boolalpha << b_list_start << ",\"list\":" << "[" << list << "]}";
		json_obj res_obj(ss.str().c_str());
		sender_->send_rsp_msg(sequence, INS_OK, cmd, &res_obj);
	}
}

void access_msg_center::s_start_task_list(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	int ret = task_mgr_->start_task_list();
	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::s_stop_task_list(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	int ret = task_mgr_->stop_task_list();
	sender_->send_rsp_msg(sequence, ret, cmd);
}

void access_msg_center::s_add_task(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	std::map<std::string,int> res = task_mgr_->add_task(msg);
	send_box_result(cmd, sequence, res);
}

void access_msg_center::s_delete_task(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	std::map<std::string,int> res = task_mgr_->delete_task(msg);
	send_box_result(cmd, sequence, res);
}

void access_msg_center::s_start_task(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	std::map<std::string,int> res = task_mgr_->start_task(msg);
	send_box_result(cmd, sequence, res);
}

void access_msg_center::s_stop_task(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	std::map<std::string,int> res = task_mgr_->stop_task(msg);
	send_box_result(cmd, sequence, res);
}

void access_msg_center::s_update_task(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	std::map<std::string,int> res = task_mgr_->update_task(msg);
	send_box_result(cmd, sequence, res);
}

void access_msg_center::s_query_task(const char* msg, std::string cmd, int sequence)
{
	RSP_ERR_NOT_IN_BOX_STATE;
	std::map<std::string,int> option;
	std::map<std::string,std::string> res = task_mgr_->query_task(msg, option);

	json_obj res_obj;
	auto array = json_obj::new_array();
	for (auto it = res.begin(); it != res.end(); it++) 
	{ 
		json_obj obj;
		obj.set_string("uuid", it->first);
		obj.set_string("state", it->second);

		if (it->second == "error")
		{
			json_obj err_obj;
			err_obj.set_int("code", option[it->first]);
			auto des = inserr_to_str(option[it->first]);
			err_obj.set_string("description", des);
			obj.set_obj("error", &err_obj);
		}
		else if (it->second == "running")
		{
			obj.set_int("progress", option[it->first]);
		}
		array->array_add(&obj);
	}
	
	res_obj.set_obj("detail", array.get());
	sender_->send_rsp_msg(sequence, INS_OK, cmd, &res_obj);
}

void access_msg_center::send_box_result(std::string cmd, int sequence, const std::map<std::string,int>& res)
{
	if (!res.empty())
	{
		json_obj res_obj;
		auto array = json_obj::new_array();
		for (auto it = res.begin(); it != res.end(); it++) 
		{ 
			json_obj obj;
			obj.set_string("uuid", it->first);
			obj.set_int("code", it->second);
			auto description = inserr_to_str(it->second);
			obj.set_string("description", description);
			array->array_add(&obj); 
		}
		res_obj.set_obj("detail", array.get());
		sender_->send_rsp_msg(sequence, INS_OK, cmd, &res_obj);
	}
	else
	{
		sender_->send_rsp_msg(sequence, INS_ERR_INVALID_MSG_FMT, cmd);
	}
}
#endif
