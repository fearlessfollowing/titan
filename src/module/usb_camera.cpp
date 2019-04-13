#include "stdio.h"
#include <sstream>
#include "usb_camera.h"
#include <fcntl.h>
#include <math.h>
#include "inslog.h"
#include "common.h"
#include "usb_msg.h"
#include "xml_config.h"
#include "usb_device.h"
#include "access_msg_center.h"
#include "cam_manager.h"
#include "ins_util.h"
#include "camera_info.h"
#include "system_properties.h"

#define SEND_DATA_TIMEOUT 	1000
#define SEND_CMD_TIMEOUT 	5000
#define RECV_CMD_TIMEOUT 	20000 	/* 停止录像存卡需要时间 */
#define RECV_VID_TIMEOUT 	5000

#define MIN_READ_DATA_LEN	(2*1024*1024)


std::atomic_llong usb_camera::base_ref_pts_(INS_PTS_NO_VALUE);



/**********************************************************************************************
 * 函数名称: close
 * 功能描述: 停止与模组间的传输 
 * 参   数: 
 * 返 回 值: 
 *********************************************************************************************/
void usb_camera::close()
{
	quit_ = true;						/* 读线程标记为退出状态 */
	usb_device::get()->cancle_transfer(pid_);
	INS_THREAD_JOIN(th_cmd_read_);		/* 停止读命令线程 */
}



/**********************************************************************************************
 * 函数名称: set_camera_time
 * 功能秒数: 设置模组的系统时间
 * 参   数: 
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::set_camera_time()
{
	int32_t ret = INS_ERR;
	int32_t loop_cnt = 30;
	time_t t1;
	struct tm *tm_local;
	
	while (--loop_cnt > 0) {
		struct timeval tm_start;
		gettimeofday(&tm_start, nullptr);
		time(&t1);
		tm_local = localtime(&t1);
		t1 = mktime(tm_local);

		json_obj obj;
		//obj.set_int("tv_sec", tm_start.tv_sec);

		obj.set_int("tv_sec", t1);		
		obj.set_int("tv_usec", tm_start.tv_usec);
		ret = send_cmd(USB_CMD_SET_SYSTEM_TIME, obj.to_string());
		BREAK_IF_NOT_OK(ret);

		ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
		BREAK_IF_NOT_OK(ret);

		int32_t ttl = tm_module_end_.tv_sec*1000*1000 + tm_module_end_.tv_usec - tm_start.tv_sec*1000*1000 - tm_start.tv_usec;
		LOGINFO("pid:%x cmd(USB_CMD_SET_SYSTEM_TIME), ttl:%d", pid_, ttl);
		if (ttl > 5*1000) 
			continue;
		else 
			return INS_OK;
	}
	return ret;
}



/**********************************************************************************************
 * 函数名称: get_camera_time
 * 功能秒数: 获取模组的系统时间与本地系统时间的差值
 * 参   数: 
 * 		delta_time - 差值
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::get_camera_time(int32_t& delta_time)
{
	int32_t ret = INS_ERR;
	int32_t loop_cnt = 30;
	
	while (--loop_cnt > 0) {
		struct timeval tm_start;
		gettimeofday(&tm_start, nullptr);

		ret = send_cmd(USB_CMD_GET_SYSTEM_TIME, "");
		BREAK_IF_NOT_OK(ret);

		ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
		BREAK_IF_NOT_OK(ret);

		int32_t ttl = tm_module_end_.tv_sec*1000*1000 + tm_module_end_.tv_usec - tm_start.tv_sec*1000*1000 - tm_start.tv_usec;
		//LOGINFO("pid:%x get time ttl:%d", pid_, ttl);
		if (ttl > 5*1000) continue;
		
		int32_t sec, usec;
		json_obj root_obj(cmd_result_.c_str());
		root_obj.get_int("tv_sec", sec);
		root_obj.get_int("tv_usec", usec);
		
		delta_time = tm_module_end_.tv_sec * 1000 * 1000 + tm_module_end_.tv_usec - sec * 1000 * 1000 - usec;

		LOGINFO("pid:%d nv - amba delta time:%d ttl:%d", pid_, delta_time, ttl);
		return INS_OK;
	}

	LOGINFO("pid:%x get time fail", pid_);
	return ret;
}



uint32_t usb_camera::get_sequence() 
{ 
	return sequence_cur_; 
}



/**********************************************************************************************
 * 函数名称: set_delta_time
 * 功能秒数: 设置对时参数
 * 参   数: 
 * 		sequence - 下一次进行对时的帧序号
 **		delta_time - 该帧下,模组的时间戳
 * 返 回 值: 
 *********************************************************************************************/
void usb_camera::set_delta_time(uint32_t sequence, int32_t delta_time) 
{ 
	sequence_delta_time_ = sequence;
	delta_time_new_ = delta_time;
}



/**********************************************************************************************
 * 函数名称: start_video_rec
 * 功能秒数: 启动视频录像或预览
 * 参   数: 
 * 		queue - 接收所有模组数据的queue
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::start_video_rec(const std::shared_ptr<cam_video_buff_i>& queue)
{
	auto ret = send_cmd(USB_CMD_START_VIDEO_RECORD);
	RETURN_IF_NOT_OK(ret);

	clear_rec_context();
	video_buff_ = queue;

	return INS_OK;
}



/**********************************************************************************************
 * 函数名称: stop_video_rec
 * 功能秒数: 停止视频录像或预览
 * 参   数: 
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::stop_video_rec()
{
	auto ret = send_cmd(USB_CMD_STOP_VIDEO_RECORD);
	is_data_thread_quit_ = true;
	return ret;
}



/**********************************************************************************************
 * 函数名称: start_still_capture
 * 功能秒数: 通知模组启动拍照
 * 参   数: 
 * 		param - 拍照参数
 *		img_repo - 存储照片数据的repo
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::start_still_capture(const cam_photo_param& param, 
											   std::shared_ptr<cam_img_repo> img_repo)
{
	pic_queue_.clear();

	pic_type_ = param.type;
	raw_seq_ = pic_seq_ = 1; /* 非timelapse都发1 */

	if (param.type == INS_PIC_TYPE_HDR || param.type == INS_PIC_TYPE_BRACKET) {
		pic_cnt_ = param.count;
	} else if (param.type == INS_PIC_TYPE_BURST) {
		pic_cnt_ = param.count;
	} else if (param.type == INS_PIC_TYPE_TIMELAPSE) {
		pic_cnt_ = 1;
		pic_seq_ = param.sequence;
	} else {
		pic_cnt_ = 1;
	}

	json_obj root;
	root.set_int("sequence", pic_seq_);

	if (param.mime == INS_RAW_JPEG_MIME && param.b_usb_jpeg && param.b_usb_raw) {
		pic_cnt_ *= 2;	/* 同时存jpeg raw */
	}

	/* 拍完这些照片的总超时时间 */
	total_pic_timeout_ = pic_cnt_ * single_pic_timeout_;

	/* 发送拍照命令 */
	auto ret = send_cmd(USB_CMD_STILL_CAPTURE, root.to_string());
	RETURN_IF_NOT_OK(ret);

	img_repo_ = img_repo;	/* 将存储image的repo传递给8个采集模组数据的线程 */
	
	/* needn't start read thread if b_usb_stream = false */
	if (param.b_usb_jpeg || param.b_usb_raw)	/* 需要模组传jpeg或raw数据,启动数据读线程 */
		start_data_read_task();

	return INS_OK;
}



/**********************************************************************************************
 * 函数名称: set_video_param
 * 功能秒数: 设置录像参数
 * 参   数: 
 *		param - 第一路流的视频参数
 *		sec_param - 第二路流的视频参数
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::set_video_param(const cam_video_param& param, const cam_video_param* sec_param)
{
	std::stringstream url;
	if (param.file_url != "") {
		url << param.file_url << "/origin_" << pid_ << ".mp4";
	}

	json_obj obj;
	obj.set_int("width", param.width);
	obj.set_int("height", param.height);
	obj.set_string("mime", param.mime);
	obj.set_int("bitdepth", param.bitdepth);
	obj.set_int("framerate", param.framerate);
	obj.set_int("log_mode", param.logmode);
	obj.set_bool("hdr", param.hdr);
	obj.set_int("bitrate", param.bitrate/1000);
	obj.set_bool("file_stream", param.b_file_stream);
	obj.set_string("file_url", url.str());
	obj.set_bool("usb_stream", param.b_usb_stream);
	
	if (param.b_usb_stream) 
		fps_ = ins_util::to_real_fps(param.framerate).to_double();

	if (sec_param) {
		std::stringstream sec_url;
		if (sec_param->file_url != "") {
			sec_url << sec_param->file_url << "/origin_" << pid_ << "_lrv.mp4";
		}

		json_obj sec_obj;
		sec_obj.set_int("width", sec_param->width);
		sec_obj.set_int("height", sec_param->height);
		sec_obj.set_string("mime", "h264");
		sec_obj.set_int("bitdepth", 8);
		sec_obj.set_int("framerate", sec_param->framerate);
		sec_obj.set_int("log_mode", sec_param->logmode);
		sec_obj.set_int("bitrate", sec_param->bitrate/1000);
		sec_obj.set_bool("file_stream", sec_param->b_file_stream);
		sec_obj.set_string("file_url", sec_url.str());
		sec_obj.set_bool("usb_stream", sec_param->b_usb_stream);

		obj.set_obj("sec_stream", &sec_obj);

		if (sec_param->b_usb_stream) 
            fps_ = ins_util::to_real_fps(sec_param->framerate).to_double();
	}

	rec_seq_ = param.rec_seq;

	return send_cmd(USB_CMD_SET_VIDEO_PARAM, obj.to_string());
}



/**********************************************************************************************
 * 函数名称: get_video_param
 * 功能秒数: 获取录像参数
 * 参   数: 
 *		param - 第一路流的视频参数
 *		sec_param - 第二路流的视频参数
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::get_video_param(cam_video_param& param, std::shared_ptr<cam_video_param>& sec_param)
{
	auto ret = send_cmd(USB_CMD_GET_VIDEO_PARAM);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	json_obj root(cmd_result_.c_str());
	root.get_string("mime", param.mime);
	root.get_int("bitdepth", param.bitdepth);
	root.get_int("width", param.width);
	root.get_int("height", param.height);
	root.get_int("framerate", param.framerate);
	root.get_int("bitrate", param.bitrate);
	root.get_int("log_mode", param.logmode);
	root.get_boolean("hdr", param.hdr);
	root.get_boolean("file_stream", param.b_file_stream);
	root.get_boolean("usb_stream", param.b_usb_stream);
	root.get_string("file_url", param.file_url);
	root.get_double("rolling_shutter_time", param.rolling_shutter_time); //us
	root.get_int("gyro_delay_time", param.gyro_delay_time); 
	root.get_int("gyro_orientation", param.gyro_orientation); 
	root.get_int("crop_flag", param.crop_flag); 
	camera_info::set_gyro_orientation(param.gyro_orientation);

	param.bitrate *= 1000;

	if (param.b_usb_stream) 
        fps_ = ins_util::to_real_fps(param.framerate).to_double();

	auto sec_obj = root.get_obj("sec_stream");
	if (sec_obj) {
		sec_param = std::make_shared<cam_video_param>();
		sec_obj->get_string("mime", sec_param->mime);
		sec_obj->get_int("bitdepth", sec_param->bitdepth);
		sec_obj->get_int("width", sec_param->width);
		sec_obj->get_int("height", sec_param->height);
		sec_obj->get_int("framerate", sec_param->framerate);
		sec_obj->get_int("bitrate", sec_param->bitrate);
		sec_obj->get_int("log_mode", sec_param->logmode);
		sec_obj->get_boolean("hdr", sec_param->hdr);
		sec_obj->get_boolean("file_stream", sec_param->b_file_stream);
		sec_obj->get_boolean("usb_stream", sec_param->b_usb_stream);
		sec_obj->get_string("file_url", sec_param->file_url);
		sec_param->bitrate *= 1000;
		if (sec_param->b_usb_stream) 
            fps_ = ins_util::to_real_fps(sec_param->framerate).to_double();
	}	

	return INS_OK;
}



/**********************************************************************************************
 * 函数名称: set_photo_param
 * 功能秒数: 设置拍照参数
 * 参   数: 
 *		param - 第一路流的视频参数
 *		sec_param - 第二路流的视频参数
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
int32_t usb_camera::set_photo_param(const cam_photo_param& param)
{
	std::stringstream url;
	std::stringstream raw_url;
	if (param.file_url != "") {
		url << param.file_url << "/origin_" << pid_ << ".jpg";
		raw_url << param.file_url << "/origin_" << pid_ << ".dng";
	}

	json_obj obj;
	obj.set_string("type", param.type);
	obj.set_int("width", param.width);
	obj.set_int("height", param.height);

	if (param.type == INS_PIC_TYPE_HDR || param.type == INS_PIC_TYPE_BRACKET) {
		obj.set_int("num", param.count);
		obj.set_int("min_ev", param.min_ev);
		obj.set_int("max_ev", param.max_ev);
	} else if (param.type == INS_PIC_TYPE_BURST) {
		obj.set_int("num", param.count);
	}

	if (param.mime == INS_RAW_JPEG_MIME) {
		obj.set_bool("raw", true);
		if (param.b_file_jpeg) 
			obj.set_string("jpeg_url", url.str()); 
		if (param.b_file_raw) 
			obj.set_string("raw_url", raw_url.str());
		obj.set_bool("usb_jpeg", param.b_usb_jpeg);
		obj.set_bool("usb_raw", param.b_usb_raw);
	} else if (param.mime == INS_RAW_MIME) {
		obj.set_bool("raw", true);
		if (param.b_file_raw) 
			obj.set_string("raw_url", raw_url.str());
		obj.set_bool("usb_raw", param.b_usb_raw);
	} else {
		obj.set_bool("raw", false);
		if (param.b_file_jpeg) 
			obj.set_string("jpeg_url", url.str());
		obj.set_bool("usb_jpeg", param.b_usb_jpeg);
	}
	return send_cmd(USB_CMD_SET_PHOTO_PARAM, obj.to_string());
}


int32_t usb_camera::get_vig_min_value(int32_t& value)
{
	auto ret = send_cmd(USB_CMD_VIG_MIN_VALUE_GET);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	json_obj obj(cmd_result_.c_str());
	if (obj.get_int("value", value)) {
		return INS_OK;
	} else {
		return INS_ERR;
	}
}

int32_t usb_camera::set_vig_min_value(int32_t value)
{
	json_obj obj;
	obj.set_int("value", value);
	return send_cmd(USB_CMD_VIG_MIN_VALUE_SET, obj.to_string());
}

int32_t usb_camera::set_capture_mode(int32_t mode)
{
	json_obj obj;
	obj.set_int("mode", mode);
	return send_cmd(USB_CMD_SET_PHOTO_PARAM, obj.to_string());
}


int32_t usb_camera::delete_file(std::string dir)
{
	json_obj dir_array(dir.c_str());
	json_obj obj;
	obj.set_obj("dir", &dir_array);
	auto dir_size = obj.to_string().size();

	json_obj dirsize_obj;
	dirsize_obj.set_int("dir_size", dir_size);
	auto ret = send_cmd(USB_CMD_DELETE_FILE, dirsize_obj.to_string());
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(1000); //1s
	RETURN_IF_NOT_OK(ret);

	auto buff = std::make_shared<insbuff>(dir_size+sizeof(amba_frame_info));
	amba_frame_info* frame = (amba_frame_info*)(buff->data());
	frame->syncword = -1;
	frame->type = AMBA_FRAME_FILE_DIRS;
	frame->sequence = 1;
	frame->size = dir_size;
	memcpy(buff->data()+sizeof(amba_frame_info), obj.to_string().c_str(), dir_size);

	LOGINFO("send delete file:%s", obj.to_string().c_str());
	ret = send_data_by_ep_cmd(buff->data(), buff->size());
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(120*1000, USB_CMD_SEND_DATA_RESULT_IND);		// 2min, 删除文件多的时候可能需要很长时间
	return ret;
}


int32_t usb_camera::set_options(std::string property,int32_t value)
{
	if (property == "aaa_mode" 
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
		return set_image_property(property, value);
	} else {
		//0:PAL 1:NTSC
		return set_param(property, value); 
	}
}


//不需要6个模组同步的消息
int32_t usb_camera::get_options(std::string property, std::string& value)
{
	int32_t ret;
	json_obj obj;
	obj.set_string("property", property);

	if (property == ACCESS_MSG_OPT_FLICKER || property == ACCESS_MSG_OPT_BLC_STATE || property == ACCESS_MSG_OPT_STORAGE_CAP) {
		ret = send_cmd(USB_CMD_GET_CAMERA_PARAM, obj.to_string());
		RETURN_IF_NOT_OK(ret);
	} else if (property == ACCESS_MSG_OPT_IMGPARAM) {
		ret = send_cmd(USB_CMD_GET_IMAGE_PROPERTY, "");
		RETURN_IF_NOT_OK(ret);
	} else  {
		LOGERR("get invalid option:%s", property.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	if (property == ACCESS_MSG_OPT_FLICKER || property == ACCESS_MSG_OPT_BLC_STATE) {
		json_obj obj(cmd_result_.c_str());
		obj.get_string(property.c_str(), value);
	
		//LOGINFO("pid:%d %s:%s", pid_, property.c_str(), value.c_str());
	} else if (property == ACCESS_MSG_OPT_STORAGE_CAP) {
		json_obj obj(cmd_result_.c_str());
		obj.set_int("index", pid_);
		value = obj.to_string();
		//LOGINFO("pid:%d %s:%s", pid_, property.c_str(), value.c_str());
	} else {	
		value = cmd_result_;
		//LOGINFO("pid:%d %s:%s", pid_, property.c_str(), value.c_str());
	}

	return INS_OK;
}

int32_t usb_camera::set_image_property(std::string property,int32_t value)
{
	json_obj obj;
	obj.set_string("property", property);
	obj.set_int("value", value);

	auto ret = send_cmd(USB_CMD_SET_IMAGE_PROPERTY, obj.to_string());
	RETURN_IF_NOT_OK(ret);

	if ((property == "aaa_mode" && !value)) {	/* 0:手动 1：自动 2：独立 3：快门优先 4：iso优先 */
		single_pic_timeout_ = RECV_PIC_TIMEOUT;
		LOGINFO("pid:%d set aaa mode:%d", pid_, value);
	} else if (property == "long_shutter") {
		single_pic_timeout_ = value * 1000 + RECV_PIC_TIMEOUT;
		LOGINFO("pid:%d set long shutter:%d ms", pid_, single_pic_timeout_);
	}

	return INS_OK;
}

int32_t usb_camera::set_param(std::string property, int32_t value)
{
	json_obj obj;
	obj.set_string("property", property.c_str());
	obj.set_int("value", value);

	return send_cmd(USB_CMD_SET_CAMERA_PARAM, obj.to_string());
}



/**********************************************************************************************
 * 函数名称: get_version
 * 功能秒数: 获取模组的版本号
 * 参   数: 
 *		version - 存储模组的版本号信息
 * 返 回 值: 成功返回INS_OK; 失败返回错误码(见inserr.h)
 *********************************************************************************************/
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

int32_t usb_camera::reboot()
{
	return send_cmd(USB_CMD_REBOOT, "");
}

int32_t usb_camera::change_usb_mode()
{
	return send_cmd(USB_CMD_CHANGE_USB_MODE, "");
}

int32_t usb_camera::format_flash()
{
	return send_cmd(USB_CMD_FORMAT_FLASH, "");
}

int32_t usb_camera::test_spi()
{
	return send_cmd(USB_CMD_TEST_SPI, ""); 
}

int32_t usb_camera::gyro_calibration()
{
	int32_t ret = send_cmd(USB_CMD_GYRO_CALIBRATION_REQ, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(10*1000, USB_CMD_GYRO_CALIBRATION_IND); //10s
	RETURN_IF_NOT_OK(ret);

	double x = 0, y = 0, z = 1;
	json_obj obj(cmd_result_.c_str());
	auto acc_obj = obj.get_obj("acc");
	if (acc_obj) {
		acc_obj->get_double("x", x);
		acc_obj->get_double("y", y);
		acc_obj->get_double("z", z);
		xml_config::set_accel_offset(x, y, z);
	} else {
		LOGERR("no aac obj in result");
	}

	return INS_OK;
}

int32_t usb_camera::start_magmeter_calibration()
{
	mag_cal_quit_ = false;
	f_magmeter_cal_ = std::async(std::launch::async, &usb_camera::do_magmeter_calibration, this);
	return INS_OK;
}

int32_t usb_camera::stop_magmeter_calibration()
{
	mag_cal_quit_ = true;
	if (f_magmeter_cal_.valid()) {
		return f_magmeter_cal_.get();
	} else {
		return INS_ERR;
	}
}

int32_t usb_camera::do_magmeter_calibration()
{
	ins::magneticCalibrate cal;
	ins::MagCalibConfigData out_config;

	int32_t ret = INS_ERR;
	int32_t res = INS_ERR;

	while (!mag_cal_quit_) {
		//读取数据
		ret = send_cmd(USB_CMD_MAGMETER_CALIBRATION_REQ, "");
		RETURN_IF_NOT_OK(ret);

		ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
		RETURN_IF_NOT_OK(ret);

		ret = read_cmd_rsp(30*1000, USB_CMD_MAGMETER_CALIBRATION_IND);
		RETURN_IF_NOT_OK(ret);

		start_data_read_task();
		ret = stop_data_read_task(true);
		if (ret != INS_OK && ret != INS_ERR_OVER) return ret;

		//校准
		std::vector<Eigen::Vector3d> in_data;
		uint8_t* p = buff_->data();
		uint32_t offset = 0;

		while (offset + 3*sizeof(double) <= buff_->size()) {
			Eigen::Vector3d t;
			memcpy(&t[0], p+offset, sizeof(double)*3);
			in_data.push_back(t);
			offset += 3*sizeof(double);
		}

		if (cal.feedMagneticData(in_data)) {
			ret = cal.calibrateOnce(out_config);

			if (ret != ins::magneticCalibrate::errorCode::Succeed) {
				res = INS_ERR;
				LOGINFO("magnetic calibration fail:%d", ret);
			} else {
				res = INS_OK;
				LOGINFO("magnetic calibration OK");
			}
		} else {
			LOGINFO("magmeter still data");
		}
	}

	if (res == INS_OK && ret == INS_OK) {
		printf("r:%lf %lf %lf   %lf %lf %lf   %lf %lf %lf  c:%lf %lf %lf\n", 
			out_config.R_(0,0), 
			out_config.R_(0,1), 
			out_config.R_(0,2),
			out_config.R_(1,0), 
			out_config.R_(1,1), 
			out_config.R_(1,2),
			out_config.R_(2,0), 
			out_config.R_(2,1), 
			out_config.R_(2,2),
			out_config.c_(0), 
			out_config.c_(1), 
			out_config.c_(2));

		json_obj obj;	
		auto r_array = json_obj::new_array();
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				r_array->array_add(out_config.R_(i,j));	
			}
		}
		obj.set_obj("r", r_array.get());

		auto c_array = json_obj::new_array();
		for (int i = 0; i < 3; i++) {
			c_array->array_add(out_config.c_(i));	
		}
		obj.set_obj("c", c_array.get());

		printf("json res:%s\n", obj.to_string().c_str());

		ret = send_cmd(USB_CMD_MAGMETER_CALIBRATION_RES, obj.to_string());
	}

	RETURN_IF_NOT_OK(ret);
	RETURN_IF_NOT_OK(res);
 
	return INS_OK;
}


int32_t usb_camera::magmeter_calibration()
{
	ins::magneticCalibrate cal;
	ins::MagCalibConfigData out_config;

	int32_t ret = INS_OK;
	int still_fail_cnt = 2;
	int cal_fail_cnt = 2;
	
	while (still_fail_cnt > 0 && cal_fail_cnt > 0) {
		//读取数据
		ret = send_cmd(USB_CMD_MAGMETER_CALIBRATION_REQ, "");
		RETURN_IF_NOT_OK(ret);

		ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
		RETURN_IF_NOT_OK(ret);

		ret = read_cmd_rsp(30*1000, USB_CMD_MAGMETER_CALIBRATION_IND);
		RETURN_IF_NOT_OK(ret);

		start_data_read_task();
		ret = stop_data_read_task(true);
		if (ret != INS_OK && ret != INS_ERR_OVER) return ret;

		//校准
		std::vector<Eigen::Vector3d> in_data;
		uint8_t* p = buff_->data();
		uint32_t offset = 0;
		while (offset + 3*sizeof(double) <= buff_->size()) {
			Eigen::Vector3d t;
			memcpy(&t[0], p+offset, sizeof(double)*3);
			in_data.push_back(t);
			offset += 3*sizeof(double);
		}

		if (cal.feedMagneticData(in_data)) {
			ret = cal.calibrateOnce(out_config);
			if (ret != ins::magneticCalibrate::errorCode::Succeed) {
				LOGINFO("magnetic calibration fail:%d", ret);
				ret = INS_ERR;
				cal_fail_cnt--;
			} else {
				ret = INS_OK;
				break;
			}
		} else {
			LOGERR("invalid data");
			still_fail_cnt--;
			ret = INS_ERR_COMPASS_STILL;
		}
	}

	if (ret == INS_OK) {
		printf("r:%lf %lf %lf   %lf %lf %lf   %lf %lf %lf  c:%lf %lf %lf\n", 
			out_config.R_(0,0), 
			out_config.R_(0,1), 
			out_config.R_(0,2),
			out_config.R_(1,0), 
			out_config.R_(1,1), 
			out_config.R_(1,2),
			out_config.R_(2,0), 
			out_config.R_(2,1), 
			out_config.R_(2,2),
			out_config.c_(0), 
			out_config.c_(1), 
			out_config.c_(2));

		json_obj obj;	
		auto r_array = json_obj::new_array();
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				r_array->array_add(out_config.R_(i,j));	
			}
		}
		obj.set_obj("r", r_array.get());

		auto c_array = json_obj::new_array();
		for (int i = 0; i < 3; i++) {
			c_array->array_add(out_config.c_(i));	
		}
		obj.set_obj("c", c_array.get());

		printf("json res:%s\n", obj.to_string().c_str());

		ret = send_cmd(USB_CMD_MAGMETER_CALIBRATION_RES, obj.to_string());
	}
 
	return ret;
}

int32_t usb_camera::storage_speed_test()
{
	int32_t ret = send_cmd(USB_CMD_STORATE_SPEED_TEST_REQ, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(5*60*1000, USB_CMD_STORATE_SPEED_TEST_IND); //5min 2018.9.27 15:32 范健聪确认超时5分钟
	RETURN_IF_NOT_OK(ret);

	LOGINFO("pid:%d storage_speed_test over", pid_);

	return INS_OK;
}

int32_t usb_camera::get_log_file(std::string file_name)
{
	// std::stringstream ss;
	// ss << "{\"filename\":\"" <<  file_name << "\"}";
	json_obj obj;
	obj.set_string("filename", file_name);

	auto ret = send_cmd(USB_CMD_GET_LOG_FILE, obj.to_string()); 
	RETURN_IF_NOT_OK(ret);

	std::stringstream sss;
	sss << "/data/" << pid_ << ".txt"; 
	log_file_fp_ = fopen(sss.str().c_str(), "w");
	if (log_file_fp_ == nullptr) {
		LOGERR("file:%s open fail", file_name.c_str());
	}
	start_data_read_task();
	return INS_OK;
}

int32_t usb_camera::set_uuid(std::string uuid)
{
	//std::stringstream ss;
	//ss << "{" << "\"value\":\"" << uuid << "\"}";
	json_obj obj;
	obj.set_string("value", uuid);

	int32_t ret = send_cmd(USB_CMD_SET_UUID, obj.to_string());
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t usb_camera::get_uuid(std::string& uuid, std::string& sensorId)
{
	int32_t ret = send_cmd(USB_CMD_GET_UUID, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	std::string result = get_result();

	json_obj root_obj(result.c_str());
	root_obj.get_string("uuid", uuid);
	root_obj.get_string("sensorId", sensorId);

	if (uuid == "") {
		LOGINFO("get uuid null");
		return INS_ERR;
	} else {
		LOGINFO("get uuid:%s sensorId:%s", uuid.c_str(), sensorId.c_str());
		return INS_OK;
	}
}

int32_t usb_camera::calibration_awb_req(int32_t x, int32_t y, int32_t r, std::string& sensor_id, int32_t* maxvalue, std::shared_ptr<insbuff>& buff)
{
	// std::stringstream ss;
	// ss << "{" << "\"x\":" << x << "," << "\"y\":" << y << "," << "\"r\":" << r << "}";
	json_obj obj;
	obj.set_int("x", x);
	obj.set_int("y", y);
	obj.set_int("r", r);

	int32_t ret = send_cmd(USB_CMD_CALIBRATION_AWB_REQ, obj.to_string());
	RETURN_IF_NOT_OK(ret);

	//respone immediately
	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	//then amba calibration, indiation to tx when over
	ret = read_cmd_rsp(40000, USB_CMD_CALIBRATION_AWB_IND);
	RETURN_IF_NOT_OK(ret);

	parse_sersor_info(cmd_result_, sensor_id, maxvalue);

	start_data_read_task();
	stop_data_read_task(true);

	if (buff_ == nullptr) return INS_ERR_CAMERA_READ_DATA;

	buff = buff_;

	return INS_OK;
}


int32_t usb_camera::calibration_bpc_req()
{
	int32_t ret = send_cmd(USB_CMD_CALIBRATION_BPC_REQ, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(2*60*1000, USB_CMD_CALIBRATION_BPC_IND); //2min
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int32_t usb_camera::calibration_blc_req()
{
	int32_t ret = send_cmd(USB_CMD_CALIBRATION_BLC_REQ, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(4*60*1000, USB_CMD_CALIBRATION_BLC_IND);	// 4min
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

std::string usb_camera::get_result()
{
	return cmd_result_;
}

// 调用发送命消息后，不管怎样都要调用wait_cmd_over作为结束
std::future<int32_t> usb_camera::wait_cmd_over()
{
	return std::async(std::launch::async, &usb_camera::do_wait_cmd_over, this);
}



int32_t usb_camera::do_wait_cmd_over()
{
	/* 表示发送命令失败或者没有发送命令，不用去等待响应 */
	if (send_cmd_ == -1) 
		return INS_OK;

	int32_t timeout;
	if (send_cmd_ == USB_CMD_STILL_CAPTURE) {
		timeout = total_pic_timeout_;
	} else {
		timeout = RECV_CMD_TIMEOUT;
	}

	auto ret = read_cmd_rsp(timeout);

	if (send_cmd_ == USB_CMD_STILL_CAPTURE) {
		if (ret == INS_OK) {
			/* 模组可能在数据还没有传完的时候就回响应了,所以这里阻塞到数据读完 */
			ret = stop_data_read_task(true); /* 响应成功，有可能读数据出错 */
		} else {
			stop_data_read_task(false); /* 马上停止读线程 */
		}
		img_repo_ = nullptr;
	} else if (send_cmd_ == USB_CMD_STOP_VIDEO_RECORD) {
		auto r = stop_data_read_task();
		if (ret == INS_OK) ret = r; /* 卡速不足 */
		video_buff_ = nullptr;
		b_record_ = false;
	} else if (send_cmd_ == USB_CMD_START_VIDEO_RECORD) { /* 收到启动录像成功响应后再启动读数据线程 */
		if (ret == INS_OK) {
			b_record_ = true;   /* record 不能用send_cmd_来标示，因为录像过程中对时会改变send_cmd_ */
			start_data_read_task();
		}
	} else if (send_cmd_ == USB_CMD_GET_LOG_FILE) {
		stop_data_read_task();
		if (log_file_fp_) {
			fclose(log_file_fp_);
			log_file_fp_ = nullptr;
		}
	}

	return ret;
}



void usb_camera::read_cmd_task()
{
	while (!quit_) {
		read_cmd(0); // 无限长
	}
	LOGINFO("pid:%d cmd read task exit", pid_);
}



int32_t usb_camera::read_cmd_rsp(int32_t timeout, int32_t cmd)
{
	uint32_t rsp_cmd = (cmd != -1) ? cmd : send_cmd_;
	int32_t loop_cnt = timeout / 30;

	//LOGINFO("pid:%d read rsp timeout:%d loop:%d", pid_, timeout, loop_cnt);

	while (!quit_ && loop_cnt-- > 0 && exception_ == INS_OK) {
		mtx_cmd_rsp_.lock();
		auto it = cmd_rsp_map_.find(rsp_cmd);
		if (it == cmd_rsp_map_.end()) {
			mtx_cmd_rsp_.unlock();
			usleep(30*1000);
			continue;
		} else {
			auto ret = it->second;
			cmd_rsp_map_.erase(it);
			mtx_cmd_rsp_.unlock();
			return ret;
		}
	}

	RETURN_IF_NOT_OK(exception_);

	LOGERR("pid:%d read cmd:%s rsp timeout", pid_, get_cmd_str(rsp_cmd));
	if (is_exception_cmd(rsp_cmd)) 
		exception_ = INS_ERR_CAMERA_READ_CMD;

	return INS_ERR_CAMERA_READ_CMD;
}


bool usb_camera::is_exception_cmd(int32_t cmd)
{
	if (cmd ==  USB_CMD_START_VIDEO_RECORD           
		|| cmd == USB_CMD_STOP_VIDEO_RECORD                  
		|| cmd == USB_CMD_STILL_CAPTURE                
		|| cmd == USB_CMD_GET_VIDEO_PARAM              
		|| cmd == USB_CMD_SET_VIDEO_PARAM              
		|| cmd == USB_CMD_GET_PHOTO_PARAM              
		|| cmd == USB_CMD_SET_PHOTO_PARAM
		|| cmd == USB_CMD_SET_SYSTEM_TIME) {
		return true;
	} else {
		return false;
	}
}



/**********************************************************************************************
 * 函数名称: read_cmd
 * 功能秒数: 从模组的命令端口读取数据
 * 参   数: 
 * 		timeout - 超时时间(0:无限等待)
 * 返 回 值: 成功返回INS_OK; 失败见错误码
 *********************************************************************************************/
int32_t usb_camera::read_cmd(int32_t timeout)
{
	// LOGINFO("pid:%d read begin", pid_);
	RETURN_IF_NOT_OK(exception_);

	/* 1.从命令端点读取数据 */
	char buff[USB_MAX_CMD_SIZE];	// 1024
	int32_t ret = usb_device::get()->read_cmd(pid_, buff, USB_MAX_CMD_SIZE, timeout);
	if (ret != LIBUSB_SUCCESS) {
		if (ret != LIBUSB_ERROR_CANCLE) {
			LOGERR("pid:%d usb read cmd fail", pid_);
		}
		return INS_ERR_CAMERA_READ_CMD;
	}

	/* 2.解析数据并做相应处理 */
	int32_t rsp_cmd = 0;
	int32_t rsp_result = 0;
	json_obj root_obj(buff);
	
	root_obj.get_int("type", rsp_cmd);
	root_obj.get_int("code", rsp_result);
	
	if (rsp_result >= INS_OK) 
		rsp_result = INS_OK;

	if (rsp_cmd != USB_CMD_SET_SYSTEM_TIME && rsp_cmd != USB_CMD_GET_SYSTEM_TIME) {	
		LOGINFO("pid:%d recv cmd:%x respone:%s code:%d", pid_, rsp_cmd, buff, rsp_result); 
	}

	cmd_result_ = "";
	root_obj.get_string("data", cmd_result_);

	if (rsp_result == INS_OK) {		/* 成功处理的某个命令后者模组发来通知指示 */
		if (USB_CMD_STORAGE_STATE_IND == rsp_cmd) {				/* 模组上的存储设备状态变化指示 */
			std::string data;
			root_obj.get_string("data", data);
			send_storage_state(data);
			return INS_OK; 			/* 不用设置cmd_rsp_map_，因为这个不是请求消息 */
		} else if (USB_CMD_VIDEO_FRAGMENT == rsp_cmd) {			/* 启动新的视频段指示 */
			if (pid_ != INS_CAM_NUM) 
				return INS_OK; 		/* 只处理master */
			auto obj_data = root_obj.get_obj("data");
			int32_t sequence = -1;
			obj_data->get_int("sequence", sequence);
			if (sequence > 0) 
				send_video_fragment_msg(sequence);
			return INS_OK; 			/* 不用设置cmd_rsp_map_，因为这个不是请求消息 */
		} else if (USB_CMD_VIG_MIN_VALUE_CHANGE == rsp_cmd) {	/* 视频分段最小值改变指示 */
			send_vig_min_value_change_msg();
		} else if (rsp_cmd == USB_CMD_SET_PHOTO_PARAM) {		/* USB_CMD_SET_PHOTO_PARAM的响应 */
			auto data_obj = root_obj.get_obj("data");
			if (data_obj) {
				int32_t gyro_orientation = 0;
				data_obj->get_int("gyro_orientation", gyro_orientation);
				camera_info::set_gyro_orientation(gyro_orientation);	/* 设置陀螺仪的方向 */
			}
		} else if (rsp_cmd == USB_CMD_STILL_CAPTURE) {			/* 拍照命令的响应(USB_CMD_STILL_CAPTURE) */
			std::string state;
			root_obj.get_string("what", state);
			if (state == "doing") {
				if (pid_ == INS_CAM_NUM && pic_type_ != INS_PIC_TYPE_TIMELAPSE) {
					send_pic_origin_over();						/* 发送拍照完成 */
				}
				return INS_OK; 									/* 这里要return,不能设置cmd_rsp_值 */
			} else if (state == "done") {
				if (pid_ == INS_CAM_NUM && pic_type_ == INS_PIC_TYPE_TIMELAPSE) {	/* 如果是拍timelapse */
					send_timelapse_pic_take(pic_seq_);	/* 自动加 */
				}					
			}
		} else if (rsp_cmd == USB_CMD_GET_SYSTEM_TIME || rsp_cmd == USB_CMD_SET_SYSTEM_TIME) {	/* 设置/获取模组时间 */
			gettimeofday(&tm_module_end_, nullptr);				/* 获取/设置系统时间接收响应的时间 */
		} else if (rsp_cmd == USB_CMD_START_VIDEO_RECORD) {		/* 启动视频录像 */
			auto data_obj = root_obj.get_obj("data");
			if (data_obj) 
				data_obj->get_int64("start_time", start_pts_);
		} else {
			root_obj.get_string("data", cmd_result_);
		}
	} else {
		//LOGERR("pid:%d cmd:%x rsp errcode:%d", pid_, send_cmd_, rsp_result);
	}

	mtx_cmd_rsp_.lock();
	cmd_rsp_map_.insert(std::make_pair(rsp_cmd, rsp_result));
	mtx_cmd_rsp_.unlock();

	return INS_OK;
}


const char* usb_camera::get_cmd_str(unsigned int uCmd)
{
    switch (uCmd) {
        CONVNUMTOSTR(USB_CMD_START_VIDEO_RECORD);
        CONVNUMTOSTR(USB_CMD_STOP_VIDEO_RECORD);
        CONVNUMTOSTR(USB_CMD_VIDEO_FRAGMENT);
        CONVNUMTOSTR(USB_CMD_STILL_CAPTURE);
        CONVNUMTOSTR(USB_CMD_GET_VIDEO_PARAM);
        
        CONVNUMTOSTR(USB_CMD_SET_VIDEO_PARAM);
        CONVNUMTOSTR(USB_CMD_GET_PHOTO_PARAM);
        CONVNUMTOSTR(USB_CMD_SET_PHOTO_PARAM);
        CONVNUMTOSTR(USB_CMD_GET_IMAGE_PROPERTY);
        CONVNUMTOSTR(USB_CMD_SET_IMAGE_PROPERTY);

        CONVNUMTOSTR(USB_CMD_GET_SYSTEM_VERSION);
        CONVNUMTOSTR(USB_CMD_UPDATE_SYSTEM_START);
        CONVNUMTOSTR(USB_CMD_UPDATE_SYSTEM_COMPLETE);
        CONVNUMTOSTR(USB_CMD_REBOOT);
        CONVNUMTOSTR(USB_CMD_SET_SYSTEM_TIME);
        
        CONVNUMTOSTR(USB_CMD_GET_SYSTEM_TIME);
        CONVNUMTOSTR(USB_CMD_RECEIVE_ERROR_CMD);
        CONVNUMTOSTR(USB_CMD_ENCRYPT_FAIL);
        CONVNUMTOSTR(USB_CMD_REQ_RETRANSMIT);
        CONVNUMTOSTR(USB_CMD_SET_CAMERA_PARAM);

        CONVNUMTOSTR(USB_CMD_GET_CAMERA_PARAM);
        CONVNUMTOSTR(USB_CMD_FORMAT_FLASH);
        CONVNUMTOSTR(USB_CMD_STORATE_SPEED_TEST_REQ);
        CONVNUMTOSTR(USB_CMD_STORATE_SPEED_TEST_IND);
        CONVNUMTOSTR(USB_CMD_STORAGE_STATE_IND);

        CONVNUMTOSTR(USB_CMD_DELETE_FILE);
        CONVNUMTOSTR(USB_CMD_SET_UUID);
        CONVNUMTOSTR(USB_CMD_GET_UUID);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_AWB_REQ);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_AWB_IND);

        CONVNUMTOSTR(USB_CMD_CALIBRATION_BP_REQ);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_BP_IND);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_BPC_REQ);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_BPC_IND);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_CS_REQ);

        CONVNUMTOSTR(USB_CMD_CALIBRATION_CS_IND);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_BLC_REQ);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_BLC_IND);
        CONVNUMTOSTR(USB_CMD_CALIBRATION_BLC_RESET);
        CONVNUMTOSTR(USB_CMD_SEND_DATA_RESULT_IND);

        CONVNUMTOSTR(USB_CMD_TEST_SPI);
        CONVNUMTOSTR(USB_CMD_TEST_CONNECTION);
        CONVNUMTOSTR(USB_CMD_GET_LOG_FILE);
        CONVNUMTOSTR(USB_CMD_CHANGE_USB_MODE);
        CONVNUMTOSTR(USB_CMD_GYRO_CALIBRATION_REQ);

        CONVNUMTOSTR(USB_CMD_GYRO_CALIBRATION_IND);
        CONVNUMTOSTR(USB_CMD_MAGMETER_CALIBRATION_REQ);
        CONVNUMTOSTR(USB_CMD_MAGMETER_CALIBRATION_IND);
        CONVNUMTOSTR(USB_CMD_MAGMETER_CALIBRATION_RES);
        CONVNUMTOSTR(USB_CMD_VIG_MIN_VALUE_CHANGE);

        CONVNUMTOSTR(USB_CMD_VIG_MIN_VALUE_GET);
        CONVNUMTOSTR(USB_CMD_VIG_MIN_VALUE_SET);
    }
}



/**********************************************************************************************
 * 函数名称: send_cmd
 * 功能秒数: 给模组发送命令
 * 参   数: 
 * 		cmd - 命令值
 * 		content - 附加的参数
 * 返 回 值: 成功返回INS_OK; 失败见错误码
 *********************************************************************************************/
int32_t usb_camera::send_cmd(uint32_t cmd, std::string content)
{	
	char buff[USB_MAX_CMD_SIZE] = {0};

	if (content.length() > USB_MAX_CMD_SIZE - 100) {
		LOGERR("content:%s too long", content.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	if (content.length()) {
		snprintf(buff, USB_MAX_CMD_SIZE, "{%s:%u,%s:%s}", USB_MSG_KEY_CMD, cmd, USB_MSG_KEY_DATA, content.c_str());
	} else {
		snprintf(buff, USB_MAX_CMD_SIZE, "{%s:%u}", USB_MSG_KEY_CMD, cmd);
	}

	if (cmd != USB_CMD_SET_SYSTEM_TIME && cmd != USB_CMD_GET_SYSTEM_TIME) {
		LOGINFO("pid:%d send cmd:%s %s", pid_, get_cmd_str(cmd), buff);
	}


	send_cmd_ = cmd;
	cmd_result_ = "";

	mtx_cmd_rsp_.lock();
	cmd_rsp_map_.erase(send_cmd_);	/* 擦除命令响应map中当前发送命令的项(如果存在) */
	mtx_cmd_rsp_.unlock();

	RETURN_IF_NOT_OK(exception_); 	/* 等先初始化上面的值再返回 */

	int32_t ret = usb_device::get()->write_cmd(pid_, buff, strlen(buff)+1, SEND_CMD_TIMEOUT);
	if (ret != LIBUSB_SUCCESS) {
		if (is_exception_cmd(send_cmd_)) 
			exception_ = INS_ERR_CAMERA_WRITE_CMD;
		
		send_cmd_ = -1;
		return INS_ERR_CAMERA_WRITE_CMD;
	} else {
		return INS_OK;
	}
}


void usb_camera::start_data_read_task()
{
	if (!is_data_thread_quit_) 
		return;
	
	is_data_thread_quit_ = false;

	/* 启动读视频数据的异步线程 */
	f_data_read_ = std::async(std::launch::async, &usb_camera::read_data_task, this);
}


int32_t usb_camera::stop_data_read_task(bool b_wait)
{
	if (!b_wait) 
		is_data_thread_quit_ = true;

	int32_t ret = INS_OK; 
	if (f_data_read_.valid()) 
		ret = f_data_read_.get();
	is_data_thread_quit_ = true;
	return ret;
}

int32_t usb_camera::read_data_task()
{
	LOGINFO("pid %x read data task start", pid_);
	int32_t ret = INS_OK;

	while (!is_data_thread_quit_) {
		ret = read_data();
		if (ret == INS_OK || ret == INS_ERR_RETRY) {
			ret = INS_OK;
			continue;
		} else if (ret == INS_ERR_OVER) {	/* 表示数据已经读完 */
			ret = INS_OK;
			break;
		} else {
			/* 只有录像的时候才发消息停止录像 record 不能用send_cmd_来标示，
			 * 因为录像过程中对时会改变send_cmd_ 
			 */
			if (b_record_) 
				send_rec_over_msg(ret);
			break;
		}
	}

	LOGINFO("pid %x read data task over", pid_);
	return ret;
}



int32_t usb_camera::req_retransmit(bool read_pre_data)
{
	std::lock_guard<std::mutex> lock(cam_manager::mtx_);

	std::stringstream ss;
	ss << "{" << "\"sequence\":" << frame_seq_+1 << "}";
	int32_t ret = send_cmd(USB_CMD_REQ_RETRANSMIT, ss.str());
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(1000); // 1s
	if (ret == INS_OK) {
		b_retransmit_ = true;
		if (read_pre_data) {
			auto buff = std::make_shared<page_buffer>(5*1024*1024);
			ret = usb_device::get()->read_data(pid_, buff->data(), buff->size(), RECV_VID_TIMEOUT);
			if (ret != LIBUSB_SUCCESS && ret != LIBUSB_ERROR_NOT_COMPLETE) {
				return INS_ERR_CAMERA_READ_DATA;
			} else {
				return INS_OK;
			}
		}
	} else {
		return ret;
	}
	// else if (ret == INS_ERR_M_RTRANSMIT_FAIL) 
	// {
	// 	ret = INS_OK;
	// }
}


/**********************************************************************************************
 * 函数名称: read_data_head
 * 功能秒数: 读取数据头
 * 参   数: 
 * 		head - 存储帧头缓冲区指针
 * 返 回 值: 成功返回INS_OK; 失败见错误码
 *********************************************************************************************/
int32_t usb_camera::read_data_head(amba_frame_info* head)
{	
	int32_t timeout;
	int32_t loop_cnt = 10;
	int32_t ret = INS_ERR;

	/* 接收数据头的超时事件(一次的超时时间) */
	if (send_cmd_ == USB_CMD_STILL_CAPTURE) {
		timeout = single_pic_timeout_;
	} else {
		timeout = RECV_VID_TIMEOUT;
	}

	int32_t i = 0;
	
	while (i++ < loop_cnt) {	/* 循环10次 */
		if (is_data_thread_quit_) {	/* is_data_thread_quit_ - 读数据线程是否需要退出标志 */
			LOGINFO("pid:%d read head over", pid_);
			return INS_ERR_OVER;
		}

		/* 接收数据(接收头部) */
		ret = usb_device::get()->read_data(pid_, (uint8_t*)head, sizeof(amba_frame_info), timeout);
		if (ret == LIBUSB_ERROR_NULL_PACKET || ret == LIBUSB_ERROR_NOT_COMPLETE) {
			ret = req_retransmit(true);
			RETURN_IF_NOT_OK(ret);		// 重新继续开始读头
			i = 0;
			continue;
		} else if (ret == LIBUSB_ERROR_TIMEOUT) {
			continue;
		} else if (ret != LIBUSB_SUCCESS) {
			break;
		} else  {
			if (head->reserve1 < INS_OK) {	// <0:代表出错
				LOGERR("pid:%d read data err code:%d", pid_, head->reserve1);
				return head->reserve1;
			} else if (head->size <= 0) {
				LOGERR("pid:%x read invalid head size:%d", pid_, head->size);
				return INS_ERR_CAMERA_READ_DATA;
			} else {
				return INS_OK;
			}
		}
	}

	LOGERR("pid:%x read head fail retry count:%d", pid_, i);
	exception_ = INS_ERR_CAMERA_READ_DATA;
	return exception_;
}



/* 1.非录像：非正常退出标记exception_，正常退出不用任何额外处理 */
/* 2.录像： 非正常退出标记exception_，然后发送录像结束消息，正常退出不用任何额外处理 */


/**********************************************************************************************
 * 函数名称: read_data
 * 功能秒数: 读取数据
 * 参   数: 
 * 返 回 值: 成功返回INS_OK; 失败见错误码
 *********************************************************************************************/
int32_t usb_camera::read_data()
{
	amba_frame_info head;

#if 0		
	auto head_buff = std::make_shared<page_buffer>(4096);
	int32_t ret = read_data_head(head_buff->data());
	RETURN_IF_NOT_OK(ret);

	memset(&head, 0, sizeof(head));
	memcpy(&head, head_buff->data(), sizeof(amba_frame_info));
#else
	/* 1.读取帧头信息: amba_frame_info */
	int32_t ret = read_data_head(&head);
	RETURN_IF_NOT_OK(ret);
#endif


	/* 2.针对不同的数据类型做不同的处理 */


	if (head.type == AMBA_FRAME_IDR || head.type == AMBA_FRAME_I || head.type == AMBA_FRAME_B || head.type == AMBA_FRAME_P) {

		if (head.reserve1 >= head.size) {
			LOGERR("pid:%d reserve1:%d > size:%d", pid_, head.reserve1, head.size);
			return INS_ERR_OVER;
		}

		/* 分配buffer来存放该帧数据 */
		auto buff = std::make_shared<page_buffer>(head.size);
		uint32_t offset = 0;
		uint32_t size = head.size;

		/* 从USB中读出该帧数据 */
		while (offset < size) {
			uint32_t size_cur = std::min((unsigned)MIN_READ_DATA_LEN, size - offset);
			int32_t ret = usb_device::get()->read_data(pid_, buff->data()+offset, size_cur, RECV_VID_TIMEOUT);

			/* LOGINFO("pid:%d total size:%d offset:%d read size:%d", pid_, head.size, offset, size_cur); */
			if (ret == LIBUSB_ERROR_NULL_PACKET || ret == LIBUSB_ERROR_NOT_COMPLETE) {
				/* 读数据读到头,请求重传后要先将上次的数据读完 */
				auto r = req_retransmit(ret == LIBUSB_ERROR_NOT_COMPLETE);
				RETURN_IF_NOT_OK(r);
				return INS_ERR_RETRY;
			} else if (ret != LIBUSB_SUCCESS) {
				exception_ = INS_ERR_CAMERA_READ_DATA;
				return INS_ERR_CAMERA_READ_DATA; 
			}
			offset += size_cur;
		}
		
		if (b_retransmit_ && frame_seq_ + 1 != head.sequence) {
			LOGINFO("pid:%d req retransmit seq:%d discard rev seq:%d", pid_, frame_seq_+1, head.sequence);
		} else {	
			if (b_retransmit_) {
				LOGINFO("pid:%d retransmit rec seq:%d", pid_, head.sequence);
			}
			
			b_retransmit_ = false;
			frame_seq_ = head.sequence;	/* 记录当前的帧序列号 */

			/* 将该帧视频数据入队列 */
			queue_video(buff, head.type, head.timestamp, head.sequence, head.reserve1);
		}
		return INS_OK;
	} else if (head.type == AMBA_FRAME_PIC) {	/* 读照片数据(timelapse) */


#if 0
		uint32_t size = head.size;							/* 从头部获取照片数据的长度 */
		auto buff = std::make_shared<page_buffer>(size);	/* 分配缓冲区来存储来自模组的照片数据 */
		uint32_t offset = 0;
		
		while (offset < head.size) {						/* 读取完整张照片数据 */
			uint32_t size = std::min((unsigned)MIN_READ_DATA_LEN, head.size - offset);
			int32_t ret = usb_device::get()->read_data(pid_, buff->data()+offset, size, RECV_VID_TIMEOUT);
			if (ret != LIBUSB_SUCCESS)  {
				exception_ = INS_ERR_CAMERA_READ_DATA;
				return INS_ERR_CAMERA_READ_DATA; 
			}
			offset += size;
		}

		/* 数据读取成功,将照片数据存入repo中 */
		enque_pic(buff, head.timestamp, head.sequence, head.reserve1);
#else 
		uint32_t size = head.size;							/* 从头部获取照片数据的长度 */
		auto buff = std::make_shared<page_buffer>(size);	/* 分配缓冲区来存储来自模组的照片数据 */
		uint32_t offset = 0;

		uint32_t total_size = (head.size + 1023 ) / 1024) * 1024;

		while (offset <  total_size) {						/* 读取完整张照片数据 */
			uint32_t size = std::min((unsigned)MIN_READ_DATA_LEN, total_size - offset);
			
			int32_t ret = usb_device::get()->read_data(pid_, buff->data() + offset, size, RECV_VID_TIMEOUT);
			if (ret != LIBUSB_SUCCESS)	{
				exception_ = INS_ERR_CAMERA_READ_DATA;
				return INS_ERR_CAMERA_READ_DATA; 
			}
			offset += size;
		}

		memset((buff->data() + size), 0x00, total_size - size);

		/* 数据读取成功,将照片数据存入repo中 */
		enque_pic(buff, head.timestamp, head.sequence, head.reserve1);

#endif

		if (--pic_cnt_ <= 0) {
			return INS_ERR_OVER;
		} else {
			return INS_OK;
		}
	} else if (head.type == AMBA_FRAME_CB_AWB) { /* AWB数据帧 */
		buff_ = std::make_shared<insbuff>(head.size);
		int32_t ret = usb_device::get()->read_data(pid_, buff_->data(), head.size, RECV_VID_TIMEOUT);
		if (ret != LIBUSB_SUCCESS) 
			return INS_ERR_CAMERA_READ_DATA;
		return INS_ERR_OVER;
	} else if (head.type == AMBA_FRAME_CB_BRIGHT) {		/*  */
		buff_ = std::make_shared<insbuff>(head.size);
		int32_t ret = usb_device::get()->read_data(pid_, buff_->data(), head.size, RECV_VID_TIMEOUT);
		if (ret != LIBUSB_SUCCESS) 
			return INS_ERR_CAMERA_READ_DATA;
		return INS_ERR_OVER;
	} else if (head.type == AMBA_FRAME_PIC_RAW) {		/* PIC RAW数据帧 */
		auto buff = std::make_shared<insbuff>(head.size);
		int32_t ret = usb_device::get()->read_data(pid_, buff->data(), head.size, RECV_VID_TIMEOUT);
		if (ret != LIBUSB_SUCCESS) {
			exception_ = INS_ERR_CAMERA_READ_DATA;
			return INS_ERR_CAMERA_READ_DATA; 
		}
		
		queue_pic_raw(buff, head.sequence, (head.reserve1 == AMBA_FLAG_END), head.timestamp);
		if (head.reserve1 == AMBA_FLAG_END) {
			if (--pic_cnt_ <= 0) {
				return INS_ERR_OVER;
			} else {
				return INS_OK;
			}
		} else {
			return INS_OK;
		}
	} else if (head.type == AMBA_FRAME_LOG) {		/* LOG日志数据帧 */
		auto buff = std::make_shared<insbuff>(head.size);
		int32_t ret = usb_device::get()->read_data(pid_, buff->data(), head.size, RECV_VID_TIMEOUT);
		if (ret != LIBUSB_SUCCESS) 
			return INS_ERR_CAMERA_READ_DATA; 
		if (log_file_fp_) 
			fwrite(buff->data(), 1, buff->size(), log_file_fp_);
		if (head.reserve1 == AMBA_FLAG_END) {
			LOGINFO("pid:%d read log file over", pid_);
			return INS_ERR_OVER;
		} else {
			LOGINFO("pid:%d read log file seq:%d size:%d", pid_, head.sequence, head.size);
			return INS_OK;
		}
	} else if (head.type == AMBA_FRAME_MAGMETER) {		/* 磁力计数据帧 */
		LOGINFO("pid:%d read head magmeter size:%d", pid_, head.size);
		buff_ = std::make_shared<insbuff>(head.size);
		int32_t ret = usb_device::get()->read_data(pid_, buff_->data(), head.size, RECV_VID_TIMEOUT);
		if (ret != LIBUSB_SUCCESS) 
			return INS_ERR_CAMERA_READ_DATA; 
		LOGINFO("pid:%d read magmeter success", pid_);
		return INS_ERR_OVER;
	} else {
		LOGERR("pid:%x camera frame invalid type %d", pid_, head.type);
		return INS_OK;
	}
}



void usb_camera::queue_pic_raw(const std::shared_ptr<insbuff>& buff, int32_t sequece, bool b_end, int64_t timestamp)
{
	raw_buff_.push_back(buff);

	if (!b_end) return;

	int32_t total_size = 0;
	for (auto it = raw_buff_.begin(); it != raw_buff_.end(); it++) {
		total_size += (*it)->size();
	} 

	std::lock_guard<std::mutex> lock(mtx_pic_);
	auto frame = pool_->pop();

	//auto frame = std::make_shared<ins_frame>();
	frame->page_buf = std::make_shared<page_buffer>(total_size); 
	frame->pts = timestamp;
	frame->dts = frame->pts;
	frame->metadata.pts = frame->pts;
	frame->metadata.raw = true;
	frame->pid = pid_;
	frame->sequence = raw_seq_++;

	int32_t offset = 0;
	for (auto it = raw_buff_.begin(); it != raw_buff_.end(); it++) {
		memcpy(frame->page_buf->data()+offset, (*it)->data(), (*it)->size());
		offset += (*it)->size();
	}

	//LOGINFO("pid:%d raw picture read over total size:%d seq:%d", pid_, total_size, frame->sequence);
	
	img_repo_->queue_frame(index_, frame);

	raw_buff_.clear();
}



void usb_camera::enque_pic(const std::shared_ptr<page_buffer>& buff, int64_t timestamp, uint32_t sequence, uint32_t extra_size)
{
	std::lock_guard<std::mutex> lock(mtx_pic_);

	auto frame = pool_->pop();
	
	if (extra_size > 0 && extra_size < buff->offset()) {
		uint8_t* extra_data = buff->data() + buff->offset() - extra_size;
		parse_extra_data(extra_data, extra_size, sequence, &frame->metadata);
		buff->set_offset(buff->offset()-extra_size);
	}

	memset(buff->data() + buff->offset(), 0x00, buff->size() - buff->offset());
	frame->page_buf 	= buff; 
	frame->pts 			= timestamp;
	frame->dts 			= frame->pts;
	frame->sequence 	= sequence;
	frame->metadata.pts = frame->pts;
	frame->metadata.raw = false;
	frame->pid			= pid_;

	#if 0
	LOGINFO("pid:%d jpeg size:%d seq:%d", pid_, frame->page_buf->offset(), frame->sequence);
	#endif
	
	img_repo_->queue_frame(index_, frame);
} 




/**********************************************************************************************
 * 函数名称: queue_video
 * 功能秒数: 将读取的视频帧丢入的数据队列中
 * 参   数: 
 *		buff - 视频帧数据指针
 *		frametype - 帧类型
 *		timestamp - 帧的时间戳
 *		sequence - 帧的序列号
 *		extra_size - 附加数据大小
 * 返 回 值: 无
 *********************************************************************************************/
void usb_camera::queue_video(const std::shared_ptr<page_buffer>& buff, 
									uint8_t frametype, 
									int64_t timestamp, 
									uint32_t sequence, 
									uint32_t extra_size)
{	
	/* 时间戳处理 */
	pre_process_timestamp(sequence, timestamp);

	if (spsItem == nullptr || ppsItem == nullptr) {	/* 首先必须获取SPS/PPS */
		if (frametype != AMBA_FRAME_IDR) {
			return;
		}

		if (INS_OK != parse_sps_pps(buff->data(), buff->size())) {	/* 解析SPS/PPS */
			return;
		}

		LOGINFO("pid:%d first frame sequence:%d timestamp: %lld", pid_, sequence, timestamp-delta_pts_);

		if (video_buff_) {	/* 设置SPS/PPS */
			video_buff_->set_sps_pps(index_, spsItem, ppsItem);
		}

		last_seq_ = sequence;
		wait_idr_ = false;
	} else {
		if (sequence != last_seq_ + 1) {
			wait_idr_ = true;
			LOGERR("pid:%x cur sequence:%d last sequence:%d time:%lld", pid_, sequence, last_seq_, timestamp - delta_pts_);
		}
		last_seq_ = sequence;

		if (wait_idr_ && frametype != AMBA_FRAME_IDR) {
			return;
		}
		wait_idr_ = false;
	}

	if (index_ == 0) print_fps_info();

	/*
	 * 视频帧后带额外数据,线解析额外数据(比如: 陀螺仪,曝光时间等)
	 */
	if (extra_size > 0 && extra_size < buff->offset()) {
		uint8_t* extra_data = buff->data()+buff->offset()-extra_size;
		parse_extra_data(extra_data, extra_size, sequence);
		buff->set_offset(buff->offset()-extra_size);
	}

	memset(buff->data() + buff->offset(), 0x00, buff->size() - buff->offset());

	/* 从ins_frame池中获取frame,构造并丢入到模组对应的Queue中 */
	auto frame 				= pool_->pop();
	
	frame->page_buf 		= buff; 
	frame->pts 				= timestamp - delta_pts_;
	frame->dts 				= frame->pts;
	frame->duration 		= 1000000/fps_;
	frame->media_type 		= INS_MEDIA_VIDEO;
	frame->is_key_frame 	= (frametype == AMBA_FRAME_IDR) ? true : false;
	frame->pid 				= pid_;
	frame->b_fragment 		= false;
	frame->sequence 		= sequence;

	//printf("pid:%d sequence:%d pts %ld\n", pid_, sequence, frame->pts);

	if (video_buff_) 
		video_buff_->queue_frame(index_, frame);
}



int32_t usb_camera::send_data_by_ep_cmd(uint8_t* data, uint32_t size) //命令通道发送数据:老通道,以前没有用到数据通道
{
	RETURN_IF_NOT_OK(exception_);

	int32_t ret =  usb_device::get()->write_data(pid_, data, size, SEND_CMD_TIMEOUT);
	if (ret != LIBUSB_SUCCESS) {
		LOGERR("pid:%d usb write data size:%d fail", pid_, size);
		//exception_ = INS_ERR_CAMERA_WRITE_DATA; //目前用到的地方:1.升级数据 2.gamma 3.awb 4.删除文件 -->不用标记为异常
		return exception_;
	} else {
		return INS_OK;
	}
}

int32_t usb_camera::send_data(uint8_t* data, uint32_t size) //数据通道发送数据:新加通道,用于数据回传
{
	RETURN_IF_NOT_OK(exception_);

	int32_t ret =  usb_device::get()->write_data2(pid_, data, size, SEND_DATA_TIMEOUT);
	if (ret != LIBUSB_SUCCESS) {
		LOGERR("pid:%d usb write data size:%d fail", pid_, size);
		//exception_ = INS_ERR_CAMERA_WRITE_DATA; //用于回传数据,暂时不用标记为异常
		return exception_;
	} else {
		return INS_OK;
	}
}

int32_t usb_camera::send_buff_data(const uint8_t* data, uint32_t size, int32_t type, int32_t timeout)
{
	uint8_t* buff = new uint8_t[size+sizeof(amba_frame_info)]();
	amba_frame_info* frame = (amba_frame_info*)(buff);
	frame->syncword = -1;
	frame->type = type;
	frame->sequence = 1;
	frame->timestamp = 2;
	frame->size = size;
	memcpy(buff+sizeof(amba_frame_info), data, size);

	int32_t ret = send_data_by_ep_cmd(buff, size+sizeof(amba_frame_info));
	INS_DELETE_ARRAY(buff);

	if (ret != INS_OK) {
		LOGINFO("pid:%d send data fail, size:%d type:%d", pid_, size, type);
	} else {
		LOGINFO("pid:%d send data success, size:%d type:%d", pid_, size, type);
		ret = read_cmd_rsp(timeout, USB_CMD_SEND_DATA_RESULT_IND);
		RETURN_IF_NOT_OK(ret);
	}

	return ret;
}


/* 六个摄像头必须同时升级 */
int32_t usb_camera::upgrade(std::string file_name, const std::string& md5)
{
	int32_t ret = send_cmd(USB_CMD_UPDATE_SYSTEM_START, "");
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	ret = send_fw(file_name, md5);

	std::string result;
	if (ret == INS_OK) {
		result = "{\"state\":\"done\"}";
	} else {
		result = "{\"state\":\"error\"}";
	}

	ret = send_cmd(USB_CMD_UPDATE_SYSTEM_COMPLETE, result);
	RETURN_IF_NOT_OK(ret);

	ret = read_cmd_rsp(RECV_CMD_TIMEOUT);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}



int32_t usb_camera::send_fw(std::string file_name, const std::string& md5)
{
	LOGINFO("pid:%d send upgrade data begin", pid_);

	FILE* fp = fopen(file_name.c_str(), "rb");
	if (fp == nullptr) {
		LOGERR("fw file:%s open fail", file_name.c_str());
		return INS_ERR_FILE_OPEN;
	}

	int32_t ret = INS_OK;
	uint32_t size = 51200;
	uint8_t* buff = new uint8_t[size+sizeof(amba_frame_info)]();
	uint8_t* data = buff + sizeof(amba_frame_info);
	amba_frame_info* frame = (amba_frame_info*)(buff);
	frame->syncword = -1;
	frame->type = AMBA_FRAME_UP_DATA;
	frame->sequence = 1;
	
	while (!feof(fp)) {	
		int32_t len = fread(data, 1, size, fp);
		if (len < 0)  {
			LOGERR("pid:%d fw file read errror", pid_);
			ret = INS_ERR_FILE_IO;
			break;
		}
		if (len == 0) break;

		frame->size = len;
		ret = send_data_by_ep_cmd(buff, len+sizeof(amba_frame_info));
		if (ret != INS_OK) 
			break;
		frame->sequence++;
	}

	if (ret == INS_OK) {
		LOGINFO("pid:%d send fw data over", pid_);

		frame->size = md5.length();
		frame->type = AMBA_FRAME_UP_MD5;
		memcpy(data, md5.c_str(),md5.length());
		ret = send_data_by_ep_cmd(buff, md5.length()+sizeof(amba_frame_info));
		if (ret == INS_OK) {
			LOGINFO("pid:%d send fw md5 over", pid_);
		} else {
			LOGINFO("pid:%d send fw md5 error", pid_);
		}
	} else {
		LOGINFO("pid:%d send fw data error", pid_);
	}

	fclose(fp);
	INS_DELETE_ARRAY(buff);

	return ret;
}



/**********************************************************************************************
 * 函数名称: pre_process_timestamp
 * 功能秒数: 时间戳处理
 * 参   数: 
 *		sequence - 帧序列号
 *		timestamp - 该帧对应的时间戳(模组)
 * 返 回 值: 
 *********************************************************************************************/
void usb_camera::pre_process_timestamp(uint32_t sequence, int64_t timestamp)
{
	if (base_ref_pts_ == INS_PTS_NO_VALUE) {	/* 参考时间戳没有设置(以数据最先到达的那个模组的时间戳为基准) */
		if (sequence == 0) {	/* 如果是第一帧数据 */
			base_ref_pts_ = timestamp;
			send_first_frame_ts(rec_seq_, timestamp);
			LOGINFO("pid:%d set base ref time:%lld seq:%d", pid_, timestamp, sequence);
		} else {
			LOGERR("pid:%x base ref not set, but seq:%d != 1 pts:%lld", pid_, sequence, timestamp);
			return;
		}
	}

	/* 各个模组第一帧时间戳的时间差没有设置 */
	if (delta_pts_ == INS_PTS_NO_VALUE) {	/* 各模组的第一帧相对时间戳没有设置 */
		if (sequence == 0) {
			delta_pts_ = timestamp - base_ref_pts_;
			if (video_buff_) 
				video_buff_->set_first_frame_ts(pid_, delta_pts_);	/* 设置各模组第一帧的相对时间戳 */
			LOGINFO("pid:%d pts:%lld delta pts:%lld, seq:%d", pid_, timestamp, delta_pts_, sequence);
		} else {
			LOGERR("pid:%x delta pts not set, but seq:%d != 1 pts:%lld", pid_, sequence, timestamp);
			return;
		}
	}

	/* 设置下次对应的序列号 并且该序列号已经到来 */
	if (sequence_delta_time_ != (uint32_t)-1 && sequence >= sequence_delta_time_) {
		sequence_delta_time_ = -1;
		delta_time_cur_ = delta_time_new_;	/* 更新本地保存的模组当前时间戳 */
		//LOGINFO("pid:%d change delta time to:%d at sequence:%d", pid_, delta_time_cur_, sequence);
	}
	sequence_cur_ = sequence; 	/* 记录当前的帧序列号 */
}



void usb_camera::parse_nal_pos(const uint8_t* data, uint32_t data_size, uint8_t nal_type, int32_t& start_pos, int32_t& size)
{
	start_pos = -1;
	size = 0;
	
	for (uint32_t i = 0; i < data_size - 4; i++) {
		int32_t offset = 0;
		if (data[i] == 0 && data[i+1] == 0 && data[i+2] == 0 && data[i+3] == 1) {
			offset = 4; 
		} else if (data[i] == 0 && data[i+1] == 0 && data[i+2] == 1) {
			offset = 3; 
		} else {
			continue;
		}

		if (start_pos != -1) {
			size = i - start_pos;
			break;
		} else {
			if (nal_type == (data[i+offset] & 0x1f))  {
				start_pos = i + offset;
			}
		}
		i += offset;
	}
}


int32_t usb_camera::parse_sps_pps(const uint8_t* data, uint32_t data_size)
{
	int32_t sps_start_pos;
	int32_t sps_size;
	int32_t pps_start_pos;
	int32_t pps_size;

	parse_nal_pos(data, data_size, 7, sps_start_pos, sps_size);
	parse_nal_pos(data, data_size, 8, pps_start_pos, pps_size);
	if (sps_size == 0 || pps_size == 0) {
		return INS_ERR;
	}

	spsItem = std::make_shared<insbuff>(sps_size+4);
	ppsItem = std::make_shared<insbuff>(pps_size+4);

	(spsItem->data())[0] = 0;
	(spsItem->data())[1] = 0;
	(spsItem->data())[2] = 0;
	(spsItem->data())[3] = 1;
	memcpy(spsItem->data()+4, data +sps_start_pos, sps_size);
	(ppsItem->data())[0] = 0;
	(ppsItem->data())[1] = 0;
	(ppsItem->data())[2] = 0;
	(ppsItem->data())[3] = 1;
	memcpy(ppsItem->data()+4, data +pps_start_pos, pps_size);

	//LOGINFO("pid:%x sps size:%d pps size:%d", pid_, sps_size, pps_size);

	return INS_OK;
}



int32_t usb_camera::process_magmeter_data(std::shared_ptr<insbuff> buff, std::string& res, ins::magneticCalibrate* cal)
{
	std::vector<Eigen::Vector3d> in_data;
	ins::MagCalibConfigData out_config;

	FILE* fp = fopen("/home/nvidia/mgm.dat", "ab");
	fwrite(buff->data(), 1, buff->size(), fp);
	fclose(fp);

	uint8_t* p = buff->data();
	uint32_t offset = 0;
	while (offset + 3*sizeof(double) <= buff->size()) {
		// double* m = (double*)(p+offset);
		// Eigen::Vector3d t(m[0], m[1], m[2]);
		Eigen::Vector3d t;
		memcpy(&t[0], p+offset, sizeof(double)*3);
		in_data.push_back(t);
		offset += 3*sizeof(double);
	}

	printf("mag data size:%u cnt:%lu rel cnt:%lu\n", buff->size(), buff->size()/sizeof(double)/3, in_data.size());

	//ins::magneticCalibrate(in_data, out_config);
	//ins::magneticCalibrateSphere(in_data, out_config);

	cal->feedMagneticData(in_data);
	auto cal_ret = cal->calibrateOnce(out_config);

	printf("r:%lf %lf %lf   %lf %lf %lf   %lf %lf %lf\n", 
		out_config.R_(0,0), 
		out_config.R_(0,1), 
		out_config.R_(0,2),
		out_config.R_(1,0), 
		out_config.R_(1,1), 
		out_config.R_(1,2),
		out_config.R_(2,0), 
		out_config.R_(2,1), 
		out_config.R_(2,2));

	printf("c:%lf %lf %lf\n", out_config.c_(0), out_config.c_(1), out_config.c_(2));

	if (cal_ret != ins::magneticCalibrate::errorCode::Succeed) {
		LOGINFO("magnetic calibration fail:%d", cal_ret);
		return INS_ERR;
	}

	json_obj obj;	
	auto r_array = json_obj::new_array();
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			r_array->array_add(out_config.R_(i,j));	
		}
	}
	obj.set_obj("r", r_array.get());

	auto c_array = json_obj::new_array();
	for (int i = 0; i < 3; i++) {
		c_array->array_add(out_config.c_(i));	
	}
	obj.set_obj("c", c_array.get());

	res = obj.to_string();

	printf("json res:%s\n", res.c_str());

	return INS_OK;
}


void usb_camera::parse_sersor_info(const std::string& info, std::string& sensor_id, int32_t* maxvalue)
{
	json_obj root_obj(info.c_str());
	root_obj.get_string("sensorId", sensor_id);
	root_obj.get_int("MaxValue00", maxvalue[0]);
	root_obj.get_int("MaxValue01", maxvalue[1]);
	root_obj.get_int("MaxValue10", maxvalue[2]);
	root_obj.get_int("MaxValue11", maxvalue[3]);

	LOGINFO("pid:%d sensor id:%s maxvalue:%d %d %d %d", pid_, sensor_id.c_str(), maxvalue[0], maxvalue[1], maxvalue[2], maxvalue[3]);
}



void usb_camera::parse_extra_data(const uint8_t* data, uint32_t size, uint32_t seq, jpeg_metadata* metadata)
{	
	amba_video_extra* head;
	int32_t offset = 0;

	while (offset + sizeof(amba_video_extra) <= size) {
		
		head = (amba_video_extra*)(data + offset);
		offset += sizeof(amba_video_extra);

		switch (head->type) {
			case AMBA_EXTRA_EXPOSURE: {
				
				uint32_t exp_size = sizeof(int64_t) + sizeof(double);
				if (offset + exp_size > size) {
					LOGERR("exposure data exceed extra size:%d", size);
					return;
				}

				assert(head->count == 1);
				int64_t ts = *((int64_t*)(data+offset)) - delta_pts_;
				offset += sizeof(int64_t);
    			double exposure = *((double*)(data+offset));
				offset += sizeof(double);
    			int64_t exposure_ns = exposure*1000000000; //ns
				if (video_buff_) 
					video_buff_->queue_expouse(pid_, seq, ts, exposure_ns);
				//printf("---------pid:%d ts:%ld exposure:%ld\n", pid_, ts, exposure_ns);
				break;
			}

			case AMBA_EXTRA_GYRO: {
				#ifdef GYRO_EXT
				LOGERR("gyro not support");
				#else

				uint32_t gyro_size = head->count*sizeof(amba_gyro_data);
				if (offset + gyro_size > size) {
					LOGERR("gyro total size(cnt:%d) > extra size:%d", head->count, size);
					return;
				}
				//amba_gyro_data* pkt = (amba_gyro_data*)(data+offset);
				if (video_buff_) 
					video_buff_->queue_gyro(data+offset, gyro_size, delta_pts_);
				
				if (img_repo_) 
					img_repo_->queue_gyro(data+offset, gyro_size);
				
				offset += gyro_size;
				//printf("--------gyro ts:%ld\n", pkt->pts);
				#endif
				break;
			}

			case AMBA_EXTRA_GYRO_EXT: {
				#ifdef GYRO_EXT
				uint32_t gyro_size = head->count*sizeof(amba_gyro_data);
				if (offset + gyro_size > size) {
					LOGERR("gyro total size(cnt:%d) > extra size:%d", head->count, size);
					return;
				}
				amba_gyro_data* pkt = (amba_gyro_data*)(data+offset);
				if (video_buff_) 
					video_buff_->queue_gyro(data+offset, gyro_size, delta_pts_);
				if (img_repo_) 
					img_repo_->queue_gyro(data+offset, gyro_size);
				offset += gyro_size;
				printf("--------gyro ts:%ld cnt:%d\n", pkt->pts, head->count);
				#else
				LOGERR("gyro ext not support");
				#endif
				break;
			}	

			case AMBA_EXTRA_EXIF: {
				pasre_exif_info(data + offset, head->count, metadata);
				offset += head->count;
				break;
			}

			
			case AMBA_EXTRA_TEMP: { 
				//int8_t temp = *(data + offset);
				int16_t temp = *((int16_t*)(data + offset));
				int8_t h2_temp = 0, sensor_temp = 0;
				sensor_temp = (temp & 0xff);
				h2_temp = (temp >> 8) & 0xff;

				//offset += head->count;
				offset += sizeof(int16_t);

				if (temp_cnt_++ % (int32_t)fps_ == 0) {
					auto s_temp = std::to_string(temp);
					std::string name = "module.temp";
					name += std::to_string(pid_);

					LOGINFO("sensor_tem: %d C, h2_temp:%d C\n", sensor_temp, h2_temp);
					
					//LOGINFO("set %s = %s", name.c_str(), s_temp.c_str());
					property_set(name, s_temp);
				}
				//LOGINFO("pid:%d temp:%d", pid_, temp);
				break;
			}
			default:
				break;
		}
	}
}	



void usb_camera::pasre_exif_info(const uint8_t* data, uint32_t size, jpeg_metadata* metadata)
{
	assert(metadata != nullptr);

	//LOGINFO("-------pid:%d exif size:%d data:%x %x %x %x", pid_, size, data[0], data[1], data[2], data[size-1])
	
	metadata->b_exif = true;
	json_obj root_obj((char*)data);
	root_obj.get_int("ExpNum", metadata->exif.exp_num);
	root_obj.get_int("ExpDen", metadata->exif.exp_den);
	root_obj.get_int("FNumberNum", metadata->exif.f_num);
	root_obj.get_int("FNumberDen", metadata->exif.f_den);
	root_obj.get_int("EVNum", metadata->exif.ev_num);
	root_obj.get_int("EVDen", metadata->exif.ev_den);
	root_obj.get_int("ISO", metadata->exif.iso);
	root_obj.get_int("Meter", metadata->exif.meter);
	root_obj.get_int("AWB", metadata->exif.awb);
	//root_obj.get_int("ExposureMode", metadata->exif.exp_mode);
	root_obj.get_int("LightSource", metadata->exif.lightsource);
	root_obj.get_int("Flash", metadata->exif.flash);
	root_obj.get_int("ShutterSpeedNum", metadata->exif.shutter_speed_num);
	root_obj.get_int("ShutterSpeedDen", metadata->exif.shutter_speed_den);
	root_obj.get_int("FocalLengthNum", metadata->exif.focal_length_num);
	root_obj.get_int("FocalLengthDen", metadata->exif.focal_length_den);
	root_obj.get_int("Contrast", metadata->exif.contrast);
	root_obj.get_int("Sharpness", metadata->exif.sharpness);
	root_obj.get_int("Saturation", metadata->exif.saturation);
	root_obj.get_int("Brightness", metadata->exif.brightness_num);
	metadata->exif.brightness_den = 1;
	root_obj.get_int("ExposureProgram", metadata->exif.exposure_program);

	// LOGINFO("pid:%d exp:%d/%d fnumber:%d/%d ev:%d/%d iso:%d meter:%d awb:%d lightsource:%d flash:%d shutter speed:%d/%d "
	// 		"focalLenth:%d/%d contrast:%d sharpnees:%d saturation:%d brightness:%d exposure_program:%d", 
	// 	pid_, 
	// 	metadata->exif.exp_num, 
	// 	metadata->exif.exp_den,
	// 	metadata->exif.f_num,
	// 	metadata->exif.f_den,
	// 	metadata->exif.ev_num,
	// 	metadata->exif.ev_den,
	// 	metadata->exif.iso,
	// 	metadata->exif.meter,
	// 	metadata->exif.awb,
	// 	metadata->exif.lightsource,
	// 	metadata->exif.flash, 
	// 	metadata->exif.shutter_speed_num, 
	// 	metadata->exif.shutter_speed_den, 
	// 	metadata->exif.focal_length_num, 
	// 	metadata->exif.focal_length_den, 
	// 	metadata->exif.contrast, 
	// 	metadata->exif.sharpness,
	// 	metadata->exif.saturation, 
	// 	metadata->exif.brightness_num, 
	// 	metadata->exif.exposure_program);
}


void usb_camera::clear_rec_context()
{
	//LOGINFO("pid:%x clear sps pps", pid_);
	ppsItem = nullptr;
	spsItem = nullptr;
	delta_pts_ = INS_PTS_NO_VALUE;
	sequence_delta_time_ = -1;
	delta_time_cur_ = 0;
	delta_time_new_ = 0;
	sequence_cur_ = 0;
}


void usb_camera::send_rec_over_msg(int32_t errcode) const
{	
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
	obj.set_int("code", errcode);
	access_msg_center::queue_msg(0, obj.to_string());
}



/**********************************************************************************************
 * 函数名称: send_pic_origin_over
 * 功能秒数: 发送模组拍照完成消息(进程内部处理消息)
 * 参   数: 
 * 返 回 值: 
 *********************************************************************************************/
void usb_camera::send_pic_origin_over() const
{
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_PIC_ORIGIN_F);
	access_msg_center::queue_msg(0, obj.to_string());
}


/**********************************************************************************************
 * 函数名称: send_timelapse_pic_take
 * 功能描述: 发送一组timelapse拍摄完成消息 
 * 参   数: 
 * 		sequence - 序列值(当前拍的组数)
 * 返 回 值: 无
 *********************************************************************************************/
void usb_camera::send_timelapse_pic_take(uint32_t sequence) const
{	
	json_obj param_obj;
	param_obj.set_int("sequence", sequence);
	access_msg_center::send_msg(ACCESS_CMD_TIMELAPSE_PIC_TAKE_, &param_obj);
} 


/**********************************************************************************************
 * 函数名称: send_storage_state
 * 功能描述: 发送模组存储设备状态变化消息	 
 * 参   数: 
 * 		state - 状态消息
 * 返 回 值: 无
 *********************************************************************************************/
void usb_camera::send_storage_state(std::string state) const
{
	json_obj obj;
	json_obj module_obj(state.c_str());
	module_obj.set_int("index", pid_);
	obj.set_obj("module", &module_obj);
	
	LOGINFO("pid:%d stotrage state:%s msg:%s", pid_, state.c_str(), obj.to_string().c_str());
	
	access_msg_center::send_msg(ACCESS_CMD_STORAGE_STATE_, &obj);
}


/**********************************************************************************************
 * 函数名称: send_first_frame_ts
 * 功能秒数: 发送第一帧数据的时间戳
 * 参   数: 
 * 		rec_seq - 序列值
 *		ts - 时间戳
 * 返 回 值: 无
 *********************************************************************************************/
void usb_camera::send_first_frame_ts(int32_t rec_seq, int64_t ts) const
{
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_FIRST_FRAME_TS);
	obj.set_int64("timestamp", ts);
	obj.set_int("rec_seq", rec_seq);
	access_msg_center::queue_msg(0, obj.to_string());
}


/**********************************************************************************************
 * 函数名称: send_video_fragment_msg
 * 功能秒数: 发送视频分段指示消息(进程内部处理消息)
 * 参   数: 
 * 		frament_index - 段索引
 * 返 回 值: 成功返回INS_OK; 失败见错误码
 *********************************************************************************************/
void usb_camera::send_video_fragment_msg(int32_t frament_index) const
{
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_VIDEO_FRAGMENT);
	obj.set_int("sequence", frament_index);
	access_msg_center::queue_msg(0, obj.to_string());
}


/**********************************************************************************************
 * 函数名称: send_vig_min_value_change_msg
 * 功能秒数: 发送视频分段最小值改变消息(进程内部处理消息)
 * 参   数: 
 * 返 回 值:
 *********************************************************************************************/
void usb_camera::send_vig_min_value_change_msg() const
{
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_VIG_MIN_CHANGE);
	access_msg_center::queue_msg(0, obj.to_string());
}


void usb_camera::print_fps_info()
{
	if (cnt == -1) {
		gettimeofday(&start_time, nullptr);
	}

	if (cnt++ > 120) {
		gettimeofday(&end_time, nullptr);
		double fps = 1000000.0*cnt/(double)(end_time.tv_sec*1000000 + end_time.tv_usec - start_time.tv_sec*1000000 - start_time.tv_usec);
		start_time = end_time;
		cnt = 0;
		LOGINFO("pid:%d fps: %lf", pid_, fps);
	}
}


