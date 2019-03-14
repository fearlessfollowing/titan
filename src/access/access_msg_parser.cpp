
#include "access_msg_parser.h"
#include "xml_config.h"
#include "inslog.h"
#include <sys/stat.h>
#include <sstream> 
#include "common.h"
#include "cam_manager.h"
#include "ins_util.h"
#include "hw_util.h"
#include "cam_manager.h"
#include "prj_file_mgr.h"
#include <sys/time.h>
#include "ins_battery.h"
#include <system_properties.h>

#define CPU_GPU_START_TEMP_THRESHOLD 	70
#define BATTERY_START_TEMP_THRESHOLD 	60

struct ins_resolution {
	int w;		/* 宽 */
	int h;		/* 高 */
	int fps;	/* 帧率 */
	bool rt;	/* 是否实时拼接 */
};

#define ORIGIN_FILE_PREFIX	"origin"

#define PARSE_PARAM_OBJ(msg) \
auto root_obj = std::make_shared<json_obj>(msg); \
auto param_obj = root_obj->get_obj(ACCESS_MSG_PARAM); \
if (param_obj == nullptr) \
{ \
	LOGINFO("key:%s not fond", ACCESS_MSG_PARAM); \
	return INS_ERR_INVALID_MSG_FMT; \
}

int access_msg_parser::check_disk_space(unsigned int size, std::string path)
{ 
	if (path != "") { 
		auto pos = path.find("/", 1);
		if (pos != std::string::npos) {
			path.erase(pos, path.length()-pos);
			LOGINFO("------path:%s", path.c_str());
		}

		auto space = ins_util::disk_available_size(path.c_str());
		if (space < (int)size) { 
			LOGINFO("storage space not enough:%d", space);
			return INS_ERR_NO_STORAGE_SPACE; 
		} 
	} else { 
		if (storage_path_ == "") { 
			return INS_ERR_NO_STORAGE_DEVICE; 
		} 

		auto space = ins_util::disk_available_size(storage_path_.c_str());
		if (space < (int)size) { 
			LOGINFO("storage space not enough:%d", space);
			return INS_ERR_NO_STORAGE_SPACE; 
		} 
	} 

	return INS_OK;
}


/*
 * 配置参数路径: /home/nvidia/insta360/etc/cam_config.xml
 */
void access_msg_parser::setup()
{
	/* 存储设备路径 <storage>/home/nvidia</storage>      */
	xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_STORAGE, storage_path_);
	
	//preview_url_ = "/sdcard/playlist.m3u8";
	//preview_url_ = "rtmp://127.0.0.1/live/preview";
	//live_url_ = "rtmp://127.0.0.1/live/live";

	int value = 0;
	if (!xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_FANLESS, value)) {
		b_fanless_ = (value == 0) ? false : true;
	}

	if (!xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_PANO_AUDIO, value)) {
		b_pano_audio_ = (value == 0) ? false : true;
	}

	if (!xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_AUDIO_TO_STITCH, value)) {
		b_audio_to_stitch_ = (value == 0) ? false : true;
	}

	if (!xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_RT_STAB, value)) {
		b_stab_rt_ = (value == 0) ? false : true;
	}

	if (!xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_LOGO, value)) {
		b_logo_on_ = (value == 0) ? false : true;
	}

	LOGINFO("storage path:%s fanless:%d pano audio:%d audiotostitch:%d stab_rt:%d logo:%d", 
			storage_path_.c_str(), 
			b_fanless_, 
			b_pano_audio_, 
			b_audio_to_stitch_, 
			b_stab_rt_, 
			b_logo_on_);
}


/***********************************************************************************************
** 函数名称: parse_cmd_name
** 函数功能: 提取消息的"name"字段
** 入口参数:
**		msg - 消息对象
** 返 回 值: 成功返回消息的name; 否则返回null
*************************************************************************************************/
std::string access_msg_parser::parse_cmd_name(const char* msg)
{
	auto root_obj = std::make_shared<json_obj>(msg);

	std::string cmd;
	if (!root_obj->get_string(ACCESS_MSG_NAME, cmd)) {
		return "null";
	} else {
		return cmd;
	}
}


int access_msg_parser::preview_option(const char* msg, ins_video_option& opt)
{
	PARSE_PARAM_OBJ(msg);

	opt.type = INS_PREVIEW;
	int ret = parse_video_option(param_obj, opt);
	if (INS_OK != ret) 
		return ret;

	// opt.stiching.width = INS_PREVIEW_WIDTH;
	// opt.stiching.height = INS_PREVIEW_HEIGHT;
	// opt.stiching.bitrate = INS_PREVIEW_BITRATE;
	opt.origin.storage_mode = INS_STORAGE_MODE_NONE;
	opt.origin.live_prefix = "";
	opt.stiching.url_second = "";
	opt.stiching.hdmi_display = true;		/* 预览都是同时显示到hdmi */

	if (!b_audio_to_stitch_) {				/* 拼接不需要音频 */
		opt.b_audio = false;
	} else {
		opt.audio.type = INS_AUDIO_N_C;		/* 原始流不需要音频,拼接流需要普通音频 */
	}

	if (opt.index != INS_CAM_ALL_INDEX) {	/* 单镜头合焦HDMI预览 */
		opt.b_stiching = false;
	} else {
		if (!opt.b_stiching) {
			LOGERR("start preview but no stiching");
			return INS_ERR_INVALID_MSG_PARAM;
		}
		
		if (opt.stiching.format == "hls") {
			LOGINFO("preview in hls");
			opt.stiching.url = HLS_PREVIEW_URL;
			ret = hls_dir_prepare(HLS_PREVIEW_STREAM_DIR);
			RETURN_IF_NOT_OK(ret);
		} else {
			opt.stiching.url = RTMP_PREVIEW_URL;	/* RTMP的预览地址: "rtmp://127.0.0.1/live/preview" */
		}
	}
	return INS_OK;
}


int32_t access_msg_parser::get_audio_type(bool stitch)
{
	if (!b_audio_to_stitch_) {
		return INS_AUDIO_Y_N;
	} else if (!b_pano_audio_ || !stitch) {
		return INS_AUDIO_Y_C;
	} else {
		return INS_AUDIO_Y_S;
	}
}

int access_msg_parser::record_option(const char* msg, ins_video_option& opt)
{
	PARSE_PARAM_OBJ(msg);	/* 获取"name"字段来区分是录像还是直播存片 */

	auto command = root_obj->get_obj(ACCESS_MSG_NAME);

	opt.name = command;

	opt.type = INS_RECORD;
	int ret = parse_video_option(param_obj, opt);
	RETURN_IF_NOT_OK(ret);


	opt.auto_connect.enable = false;
	opt.stiching.url_second = "";

	if (b_fanless_ && opt.b_audio && !opt.b_stiching) {
		auto cpu_temp = hw_util::get_temp(SYS_PROPERTY_CPU_TEMP);
		auto gpu_temp = hw_util::get_temp(SYS_PROPERTY_GPU_TEMP);
		auto battery_temp = hw_util::get_temp(SYS_PROPERTY_BATTERY_TEMP);

		int start_temp = INS_FANLESS_START_TEMP;
		xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_START_TEMP, start_temp);
		LOGINFO("config min temp:%d", start_temp);

		if (cpu_temp > start_temp || gpu_temp > start_temp || battery_temp > BATTERY_START_TEMP_THRESHOLD) {	
			LOGERR("fanless mode temperature high cpu:%d gpu:%d battery:%lf", cpu_temp, gpu_temp, battery_temp);
			return INS_ERR_TEMPERATURE_HIGH;
		}
		opt.duration = 15*60; //无风扇模式只录制15分钟
		opt.audio.fanless = true;
	}
	
	// else {
	// 	if (cpu_temp > CPU_GPU_START_TEMP_THRESHOLD 
	// 		|| gpu_temp > CPU_GPU_START_TEMP_THRESHOLD 
	// 		|| battery_temp > BATTERY_START_TEMP_THRESHOLD) {	
	// 		LOGERR("temperature high cpu:%d gpu:%d battery:%lf", cpu_temp, gpu_temp, battery_temp);
	// 		return INS_ERR_TEMPERATURE_HIGH;
	// 	}
	// }

	opt.audio.type = get_audio_type(opt.b_stiching);

	/*
	 * - 需要区别录像还是直播存片
	 * - 
	 */
	std::string start_str = opt.timelapse.enable? "PIC" : "VID";

#if 0
	std::string file_prefix = gen_file_prefix(start_str);
#else 

#endif

	if (opt.origin.storage_mode == INS_STORAGE_MODE_NV 
		|| opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV 
		|| opt.b_stiching) {
		ret = check_disk_space(INS_MIN_SPACE_MB, opt.path);
		RETURN_IF_NOT_OK(ret);
		ret = create_file_dir(file_prefix, opt.path);
		RETURN_IF_NOT_OK(ret);
		opt.prj_path = opt.path;

		if (opt.b_stiching) 
			opt.stiching.url = opt.path + "/" + ins_util::create_name_by_mode(opt.stiching.mode) + ".mp4";
	}

	if (opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV 
		|| opt.origin.storage_mode == INS_STORAGE_MODE_AB) {
		opt.origin.module_url = file_prefix;
		if (opt.prj_path == "") 
			opt.prj_path = INS_TMEP_PRJ_PATH;
	}

	if (opt.prj_path != "") {
		if (opt.timelapse.enable) {
			ins_picture_option pic_opt;
			pic_opt.origin = opt.origin;
			pic_opt.b_stiching = opt.b_stiching;
			pic_opt.stiching = opt.stiching;
			pic_opt.timelapse = opt.timelapse;
			ret = prj_file_mgr::create_pic_prj(pic_opt, opt.prj_path);
		} else {
			ret = prj_file_mgr::create_vid_prj(opt, opt.prj_path);
		}
		RETURN_IF_NOT_OK(ret);
	}

	return INS_OK;
}

int access_msg_parser::live_option(const char* msg, ins_video_option& opt)
{
	PARSE_PARAM_OBJ(msg);

	opt.type = INS_LIVE;
	int ret = parse_video_option(param_obj, opt);
	RETURN_IF_NOT_OK(ret);

	auto cpu_temp = hw_util::get_temp(SYS_PROPERTY_CPU_TEMP);
	auto gpu_temp = hw_util::get_temp(SYS_PROPERTY_GPU_TEMP);

	if (cpu_temp > CPU_GPU_START_TEMP_THRESHOLD || gpu_temp > CPU_GPU_START_TEMP_THRESHOLD) {	
		LOGERR("temperature high cpu:%d gpu:%d", cpu_temp, gpu_temp);
		return INS_ERR_TEMPERATURE_HIGH;
	}

	opt.audio.type = get_audio_type(opt.b_stiching);

	if (opt.b_stiching)	{
		if (opt.stiching.url == "" && !opt.stiching.hdmi_display) {
			opt.stiching.url = RTMP_LIVE_URL; //if no live url use default url
			// LOGERR("no live url && not hdmi diaplay");
			// return INS_ERR_INVALID_MSG_PARAM;
		}

		if (opt.stiching.format == "hls") {
			LOGINFO("live in hls");
			opt.stiching.url = HLS_LIVE_URL;
			ret = hls_dir_prepare(HLS_LIVE_STREAM_DIR);
			RETURN_IF_NOT_OK(ret);
		}

		opt.origin.live_prefix = ""; //推原始流的时候不支持同时推拼接流
	} else {
		if (opt.origin.live_prefix == "") {
			LOGERR("start live but no stiching");
			return INS_ERR_INVALID_MSG_PARAM;
		} else {
			opt.origin.storage_mode == INS_STORAGE_MODE_NONE;
			opt.b_stabilization = false;//如果只推原始流的话，就不防抖了
		}
	}
	
	std::string file_prefix = gen_file_prefix("VID");
	if (opt.origin.storage_mode == INS_STORAGE_MODE_NV 
		|| opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV 
		|| opt.stiching.url_second != "") {
		ret = check_disk_space(INS_MIN_SPACE_MB, opt.path);
		RETURN_IF_NOT_OK(ret);
		// if (ret != INS_OK) //live go on even though no disk space
		// {
		// 	opt.stiching.url_second = "";
		// 	return INS_OK;
		// }
		ret = create_file_dir(file_prefix, opt.path);
		RETURN_IF_NOT_OK(ret);
		opt.prj_path = opt.path;

		if (opt.stiching.url_second != "") opt.stiching.url_second = opt.path + "/" + ins_util::create_name_by_mode(opt.stiching.mode) + ".mp4";
	}

	if (opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV 
		|| opt.origin.storage_mode == INS_STORAGE_MODE_AB) {
		opt.origin.module_url = file_prefix;
		if (opt.prj_path == "") 
			opt.prj_path = INS_TMEP_PRJ_PATH;
	}

	if (opt.prj_path != "") {
		ret = prj_file_mgr::create_vid_prj(opt, opt.prj_path);
		RETURN_IF_NOT_OK(ret);
	}

	return INS_OK;
}

int access_msg_parser::hls_dir_prepare(std::string dir)
{
	if (access(dir.c_str(), 0)) {
		if (mkdir(dir.c_str(), 0755)) {
			LOGERR("mkdir:%s fail", dir.c_str());
			return INS_ERR_FILE_OPEN;
		}
	} else {	
		std::string cmd = std::string("rm -rf ") + dir + "/*";
		system(cmd.c_str());
	}

	return INS_OK;
}

int access_msg_parser::take_pic_option(const char* msg, ins_picture_option& opt)
{
	PARSE_PARAM_OBJ(msg);

	//opt.type = INS_TAKE_PIC;
	int ret = parse_picture_option(param_obj, opt);
	RETURN_IF_NOT_OK(ret);

	std::string file_prefix = gen_pic_file_prefix("PIC", opt);	// gen_file_prefix -> gen_pic_file_prefix
	LOGINFO("file_prefix = %s", file_prefix.c_str());
	if (opt.origin.storage_mode == INS_STORAGE_MODE_NV 
		|| opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV 
		|| opt.b_stiching) {
		ret = check_disk_space(INS_MIN_SPACE_MB, opt.path);
		RETURN_IF_NOT_OK(ret);

		ret = create_file_dir(file_prefix, opt.path);
		RETURN_IF_NOT_OK(ret);
		opt.prj_path = opt.path;

		if (opt.b_stiching) opt.stiching.url = opt.path;
	}

	if (opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV 
		|| opt.origin.storage_mode == INS_STORAGE_MODE_AB) {
		opt.origin.module_url = file_prefix;
		if (opt.prj_path == "") opt.prj_path = INS_TMEP_PRJ_PATH;
	}

	if (opt.index != INS_CAM_ALL_INDEX)  {	//单模组拍照
		opt.b_stiching = false;
		opt.b_stabilization = false;
		opt.b_thumbnail = false;
	} else if (opt.prj_path != "") {
		prj_file_mgr::create_pic_prj(opt, opt.prj_path);	/* 创建工程文件 */
		RETURN_IF_NOT_OK(ret);
	}

	return INS_OK;
}

int access_msg_parser::singlen_preview_option(const char* msg, ins_video_option& opt)
{
	PARSE_PARAM_OBJ(msg);

	int ret = parse_video_option(param_obj, opt);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int access_msg_parser::calibration_blc_option(const char* msg, bool& b_reset)
{
	PARSE_PARAM_OBJ(msg);

	param_obj->get_boolean("reset", b_reset);

	return INS_OK;
}

int access_msg_parser::internal_pic_over(const char* msg, int& code, std::string& path)
{
	auto root_obj = std::make_shared<json_obj>(msg);
	root_obj->get_int("code", code);
	root_obj->get_string("path", path);
	return INS_OK;
}

int access_msg_parser::power_mode_option(const char* msg, std::string& mode)
{
	PARSE_PARAM_OBJ(msg);

	param_obj->get_string("mode", mode);

	if (mode != "on" && mode != "off") {
		return INS_ERR_INVALID_MSG_PARAM;
	} else {
		return INS_OK;
	}
}

int access_msg_parser::offset_option(const char* msg, ins_offset_option& opt)
{
	PARSE_PARAM_OBJ(msg);

	param_obj->get_string(ACCESS_MSG_OPT_OFFSET_PANO_4_3, opt.pano_4_3);
	param_obj->get_string(ACCESS_MSG_OPT_OFFSET_PANO_16_9, opt.pano_16_9);
	param_obj->get_string(ACCESS_MSG_OPT_OFFSET_3D_LEFT, opt.left_3d);
	param_obj->get_string(ACCESS_MSG_OPT_OFFSET_3D_RIGHT, opt.right_3d);
	param_obj->get_boolean("factory_setting", opt.factory_setting);
	param_obj->get_boolean("using_factory_offset", opt.using_factory_offset);
	
	return INS_OK;
}

int access_msg_parser::option_get_option(const char* msg, std::string& property)
{
	PARSE_PARAM_OBJ(msg);
	
	if (!param_obj->get_string(ACCESS_MSG_OPT_PROPERTY, property)) {
		LOGERR("key property not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}

	return INS_OK;
}

static int parser_option(std::shared_ptr<json_obj> obj, std::map<std::string, int>& opt1, std::map<std::string, std::string>& opt2)
{
	std::string property;
	int value1 = 1;
	std::string value2;

	if (!obj->get_string(ACCESS_MSG_OPT_PROPERTY, property)) {
		LOGERR("key%s not fond", ACCESS_MSG_OPT_PROPERTY);
		return INS_ERR_INVALID_MSG_FMT;
	}

	// if (property == ACCESS_MSG_OPT_LOGMODE)
	// {
	// 	auto value_obj = obj->get_obj(ACCESS_MSG_OPT_VALUE);
	// 	if (value_obj == nullptr) return INS_ERR_INVALID_MSG_FMT;
	// 	value_obj->get_int(ACCESS_MSG_OPT_LOGMODE_MODE, value1);
	// 	value_obj->get_string(ACCESS_MSG_OPT_LOGMODE_EFFECT, value2);
	// 	opt1.insert(std::make_pair(property, value1));
	// 	opt2.insert(std::make_pair(property, value2));
	// }
	
	// depthMap是一个字符串，存在value2中，value1不用
	if (property == ACCESS_MSG_OPT_DEPTH_MAP || property == ACCESS_MSG_OPT_IQTYPE) {
		if (!obj->get_string(ACCESS_MSG_OPT_VALUE, value2)) {
			LOGERR("key:%s not fond", ACCESS_MSG_OPT_VALUE);
			return INS_ERR_INVALID_MSG_FMT;
		}
		opt1.insert(std::make_pair(property, value1)); 
		opt2.insert(std::make_pair(property, value2));
	} else {
		if (!obj->get_int(ACCESS_MSG_OPT_VALUE, value1)) {
			LOGERR("key:%s not fond", ACCESS_MSG_OPT_VALUE);
			return INS_ERR_INVALID_MSG_FMT;
		}
		opt1.insert(std::make_pair(property, value1));
	}

	LOGINFO("property:%s value:%d %s", property.c_str(), value1, value2.c_str());

	return INS_OK;
}

int access_msg_parser::option_option(const char* msg, std::map<std::string, int>& opt1, std::map<std::string, std::string>& opt2)
{
	PARSE_PARAM_OBJ(msg);
	
	if (param_obj->is_array()) {
		auto v_property = root_obj->get_string_array(ACCESS_MSG_PARAM);
		for (unsigned i = 0; i < v_property.size(); i++) {
			auto obj = std::make_shared<json_obj>(v_property[i].c_str());
			int ret = parser_option(obj, opt1, opt2);
			RETURN_IF_NOT_OK(ret);
		}
	} else {
		int ret = parser_option(param_obj, opt1, opt2);
		RETURN_IF_NOT_OK(ret);
	}

	return INS_OK;
}


// int access_msg_parser::set_ntscpal_option(const char* msg, std::string& mode, int& index)
// {
// 	PARSE_PARAM_OBJ(msg);

// 	int ret = parase_index(param_obj, index, INS_CAM_ALL_INDEX);
// 	RETURN_IF_NOT_OK(ret);

// 	param_obj->get_string("mode", mode);
// 	if (mode != "NTSC" && mode != "PAL")
// 	{
// 		LOGERR("invalid mode:%s", mode.c_str());
// 		return INS_ERR_INVALID_MSG_PARAM;
// 	}
// 	else
// 	{
// 		return INS_OK;
// 	}
// }

// int access_msg_parser::get_ntscpal_option(const char* msg, int& index)
// {
// 	PARSE_PARAM_OBJ(msg);

// 	return parase_index(param_obj, index, cam_manager::MASTER_INDEX);
// }

int access_msg_parser::format_camera_moudle_option(const char* msg, int& index)
{
	PARSE_PARAM_OBJ(msg);

	return parase_index(param_obj, index, INS_CAM_ALL_INDEX);
}

int access_msg_parser::module_log_file_option(const char* msg, std::string& file_name)
{
	PARSE_PARAM_OBJ(msg);

	param_obj->get_string("file",file_name);

	if (file_name == "") return INS_ERR_INVALID_MSG_PARAM;

	return INS_OK;
}

int access_msg_parser::hdmi_option(const char* msg, ins_video_option& opt)
{
	PARSE_PARAM_OBJ(msg);

	auto origin_obj = param_obj->get_obj(ACCESS_MSG_OPT_ORIGIN);
	if (origin_obj == nullptr) {
		LOGERR("key:origin not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}

	int ret = parse_origin_option(origin_obj, opt.origin);
	RETURN_IF_NOT_OK(ret);

	ret = parse_mode(param_obj, opt.stiching.mode);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
	//return check_hdmi_option(opt);
}

int access_msg_parser::update_gamma_curve_option(const char* msg, std::string& data)
{
	auto root_obj = std::make_shared<json_obj>(msg); 
	auto param_obj = root_obj->get_obj(ACCESS_MSG_PARAM);

	if (param_obj) param_obj->get_string(ACCESS_MSG_OPT_DATA, data);

	return INS_OK;
}

int access_msg_parser::systime_change_option(const char* msg, int64_t& timestamp, std::string& source)
{
	PARSE_PARAM_OBJ(msg);
	param_obj->get_int64("timestamp", timestamp);
	param_obj->get_string("source", source);
	return INS_OK;
}

int access_msg_parser::storage_option(const char* msg, std::string& path)
{
	PARSE_PARAM_OBJ(msg);

	if (!param_obj->get_string(ACCESS_MSG_OPT_PATH, path)) {
		LOGERR("key:storagePath not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}

	//if (path == "") return INS_ERR_INVALID_MSG_PARAM;

	storage_path_ = path;

	return INS_OK;
}

int access_msg_parser::storage_speed_test_option(const char* msg, std::string& path)
{
	PARSE_PARAM_OBJ(msg);
	
	param_obj->get_string(ACCESS_MSG_OPT_PATH, path);

	return INS_OK;
}

// int access_msg_parser::fw_version_option(const char* msg, int& index)
// {
// 	PARSE_PARAM_OBJ(msg);

// 	return parase_index(param_obj, index, cam_manager::MASTER_INDEX);
// }

int access_msg_parser::calibration_option(const char* msg, std::string& mode, int& delay)
{
	//PARSE_PARAM_OBJ(msg);

	auto root_obj = std::make_shared<json_obj>(msg); 
	auto param_obj = root_obj->get_obj(ACCESS_MSG_PARAM); 
	if (!param_obj) return INS_OK; 

	if (param_obj->get_string("mode", mode)) {
		if (mode != "pano" && mode != "3d") return INS_ERR_INVALID_MSG_PARAM;
	}

	param_obj->get_int("delay", delay);
	if (delay < 0) delay = 0;

	return INS_OK;
}

//no index, must upgrade all at same time
int access_msg_parser::upgrade_fw_option(const char* msg, std::string& path, std::string& version)
{
	PARSE_PARAM_OBJ(msg);

	if (!param_obj->get_string(ACCESS_MSG_OPT_PATH, path)) {
		LOGERR("key:path not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}
	
	if (path == "") return INS_ERR_INVALID_MSG_PARAM;


	if (!param_obj->get_string("version", version)) {
		LOGERR("key:version not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}
	if (version == "") return INS_ERR_INVALID_MSG_PARAM;

	LOGINFO("fw path:%s version:%s", path.c_str(), version.c_str());

	return INS_OK;
}



int access_msg_parser::parase_index(const std::shared_ptr<json_obj> obj, int& index, int defaut_index) const
{
	if (obj->get_int("index", index)) {
		if ((index > INS_CAM_NUM || index <= 0) && index != INS_CAM_ALL_INDEX) {
			LOGERR("invalid index:%d", index);
			return INS_ERR_INVALID_MSG_PARAM;
		}
	} else {
		index = defaut_index;
	}

	return INS_OK;
}

int access_msg_parser::parse_video_option(std::shared_ptr<json_obj> param_obj, ins_video_option& option) const 
{
	//for single len record
	auto ret = parase_index(param_obj, option.index, INS_CAM_ALL_INDEX); 
	RETURN_IF_NOT_OK(ret);

	param_obj->get_boolean("fileOverride", option.b_override);
	param_obj->get_boolean("saveFile", option.b_to_file);
	param_obj->get_string("storagePath", option.path);

	if (!param_obj->get_boolean("stabilization", option.b_stabilization)) {
		option.b_stabilization = b_stab_rt_;
	}

	// auto result = xml_config::get_string(INS_CONFIG_OPTION, INS_CONFIG_RECORD_TO_FILE);
	// if (result == "off") {
	//	option.b_to_file = false;
	// 	LOGINFO("config not save to file");
	// }

	auto origin_obj = param_obj->get_obj(ACCESS_MSG_OPT_ORIGIN);
	if (origin_obj == nullptr) {	/* "origin"字段不存在,返回410错误 */
		LOGERR("key:origin not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}

	if (!param_obj->get_int("duration", option.duration)) {
		option.duration = -1;
	}

	auto timelapse_obj = param_obj->get_obj("timelapse");
	if (timelapse_obj) 
		parse_timelapse_option(timelapse_obj, option.timelapse);

	parse_origin_option(origin_obj, option.origin/*, option.timelapse.enable*/);
	if (option.path != "") {	// 如果指定了存储路径肯定是存储在nvidia的
		option.origin.storage_mode = INS_STORAGE_MODE_NV;
	}
	
	if (option.type == INS_PREVIEW) {	// 容错客户端发预览的时候带saveOrigin参数
		option.origin.storage_mode = INS_STORAGE_MODE_NONE;
	}

	auto stiching_obj = param_obj->get_obj(ACCESS_MSG_OPT_STICHING);
	if (stiching_obj == nullptr) {
		option.b_stiching = false;
		LOGINFO("key:stiching not fond");
	} else {
		option.b_stiching = true;
		int ret = parse_stich_option(stiching_obj, option.stiching);
		RETURN_IF_NOT_OK(ret);
		if (option.stiching.mode != INS_MODE_PANORAMA) { /* 3d不使用陀螺仪 */
			option.b_stabilization = false;
		}
	}

	auto s_stab = property_get("sys.stab_on");
	if (s_stab == "false" && option.b_stabilization) {
		option.b_stabilization = false;
		LOGINFO("------config stab off");
	}

	auto s_audio = property_get("sys.use_audio");
	if (s_audio == "true" && !option.timelapse.enable) {
		auto audio_obj = param_obj->get_obj(ACCESS_MSG_OPT_AUDIO);
		if (audio_obj == nullptr) {
			option.b_audio = false;
			LOGINFO("key:audio not fond");
		} else {
			option.b_audio = true;
			int ret = parse_audio_option(audio_obj, option.audio);
			RETURN_IF_NOT_OK(ret);
		}
	} else {
		LOGINFO("------config no audio");
		option.b_audio = false;
	}

	auto auto_connect_obj = param_obj->get_obj("autoConnect"); //just for living
	if (auto_connect_obj) {
		auto_connect_obj->get_boolean("enable", option.auto_connect.enable);
		auto_connect_obj->get_int("interval", option.auto_connect.interval);
		auto_connect_obj->get_int("count", option.auto_connect.count);
	}

	if (b_logo_on_ /*&& option.b_stiching*/) { /* 非实时拼接也会用辅码流拼接预览流也要logo */
		std::string logo_file = storage_path_ + "/" + INS_LOGO_FILE;
		if (!access(logo_file.c_str(), 0)) 
			option.logo_file = logo_file;
	}

	ret = check_video_option(option);
	RETURN_IF_NOT_OK(ret);


	//实时拼接且存储同时存原片的时候，主码流固定分辨率，拼接用辅码流
	// if (option.origin.storage_mode != INS_STORAGE_MODE_NONE && option.b_stiching) {
	// 	if (option.stiching.framerate != 30) {
	// 		LOGERR("stitching framerate:%d not 30", option.stiching.framerate);
	// 		return INS_ERR_INVALID_MSG_PARAM;
	// 	}
	
	// 	if (option.stiching.mode == INS_MODE_PANORAMA) {
	// 		option.origin.width = 3840;
	// 		option.origin.height = 2160;
	// 	} else {
	// 		option.origin.width = 3200;
	// 		option.origin.height = 2400;
	// 	}
	// 	option.origin.framerate = 30;
	// 	option.origin.bitrate = 60000;
	// }

	return INS_OK;
}


int access_msg_parser::parse_picture_option(std::shared_ptr<json_obj> param_obj, ins_picture_option& option) const 
{
	auto ret = parase_index(param_obj, option.index, INS_CAM_ALL_INDEX); 
	RETURN_IF_NOT_OK(ret);

	param_obj->get_string("storagePath", option.path);
	param_obj->get_boolean("thumbnail", option.b_thumbnail);

	if (!param_obj->get_boolean("stabilization", option.b_stabilization)) {
		option.b_stabilization = b_stab_rt_;
	}

	auto origin_obj = param_obj->get_obj(ACCESS_MSG_OPT_ORIGIN);
	if (origin_obj == nullptr) {
		LOGERR("key:origin not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}

	int delay = 0;
	param_obj->get_int(ACCESS_MSG_OPT_DELAY, delay);
	if (delay > 0) option.delay = delay;

	parse_origin_option(origin_obj, option.origin, true);
	if (option.path != "") {	//如果指定了存储路径肯定是存储在nvidia的
		option.origin.storage_mode = INS_STORAGE_MODE_NV;
	}

	auto stiching_obj = param_obj->get_obj(ACCESS_MSG_OPT_STICHING);
	if (stiching_obj == nullptr) {
		option.b_stiching = false;
		//LOGINFO("key:stiching not fond");
	} else {
		if (option.origin.mime == INS_RAW_MIME) {
			LOGERR("don't support stiching when taking raw");
			return INS_ERR_INVALID_MSG_PARAM;
		}

		option.b_stiching = true;
		int ret = parse_stich_option(stiching_obj, option.stiching);
		RETURN_IF_NOT_OK(ret);
		if (option.stiching.mode != INS_MODE_PANORAMA) {	// 3d不使用陀螺仪
			option.b_stabilization = false;
		}
	}

	auto s_stab = property_get("sys.stab_on");
	if (s_stab == "false" && option.b_stabilization) {
		option.b_stabilization = false;
		LOGINFO("------config stab off");
	}

	auto burst_obj = param_obj->get_obj(ACCESS_MSG_OPT_BURST);
	if (burst_obj != nullptr) {
		burst_obj->get_boolean("enable", option.burst.enable);
		burst_obj->get_int("count", option.burst.count);
	}

	auto hdr_obj = param_obj->get_obj(ACCESS_MSG_OPT_HDR);
	if (hdr_obj != nullptr) {
		hdr_obj->get_boolean("enable", option.hdr.enable);
		hdr_obj->get_int("count", option.hdr.count);
		hdr_obj->get_int("min_ev", option.hdr.min_ev);
		hdr_obj->get_int("max_ev", option.hdr.max_ev);
	}

	auto bracket_obj = param_obj->get_obj(ACCESS_MSG_OPT_BRACKET);
	if (bracket_obj != nullptr) {
		bracket_obj->get_boolean("enable", option.bracket.enable);
		bracket_obj->get_int("count", option.bracket.count);
		bracket_obj->get_int("min_ev", option.bracket.min_ev);
		bracket_obj->get_int("max_ev", option.bracket.max_ev);
	}

	if (option.origin.storage_mode == INS_STORAGE_MODE_DEFAULT) {
		//busrt/hdr默认分开存raw/jpg
		if (option.bracket.enable || option.hdr.enable || option.burst.enable) {
			option.origin.storage_mode = INS_STORAGE_MODE_AB_NV;
		} else {
			option.origin.storage_mode = INS_STORAGE_MODE_NV;
		}
	}

	if (b_logo_on_) {
		std::string logo_file = storage_path_ + "/" + INS_LOGO_FILE;
		if (!access(logo_file.c_str(), 0)) option.logo_file = logo_file;
	}

	return check_picture_option(option);
}

int access_msg_parser::parse_video_file_option(std::shared_ptr<json_obj> param_obj, ins_video_file_option& option) const 
{
	for (int i = 0; i < 6; i++) {
		std::stringstream ss;
		ss << "file" << i +1;
		std::string url;
		if (!param_obj->get_string(ss.str().c_str(), url)) {
			LOGERR("key:%s not found", ss.str().c_str());
			return INS_ERR_INVALID_MSG_FMT;
		}
		option.url.push_back(url);
	}

	auto stiching_obj = param_obj->get_obj(ACCESS_MSG_OPT_STICHING);
	if (stiching_obj == nullptr)
	{
		LOGINFO("key:stiching not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}

	int ret = parse_stich_option(stiching_obj, option.stiching);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
	//return check_video_fcompose_option(option);
}

int access_msg_parser::parse_picture_file_option(std::shared_ptr<json_obj> param_obj, ins_picture_file_option& option) const 
{
	for (int i = 0; i < 6; i++)
	{
		std::stringstream ss;
		ss << "file" << i +1;
		std::string url;
		if (!param_obj->get_string(ss.str().c_str(), url))
		{
			LOGERR("key:%s not found", ss.str().c_str());
			return INS_ERR_INVALID_MSG_FMT;
		}
		option.url.push_back(url);
	}

	auto stiching_obj = param_obj->get_obj(ACCESS_MSG_OPT_STICHING);
	if (stiching_obj == nullptr)
	{
		LOGINFO("key:stiching not fond");
		return INS_ERR_INVALID_MSG_FMT;
	}

	int ret = parse_stich_option(stiching_obj, option.stiching);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
	//return check_pic_fcompose_option(option);
}

int access_msg_parser::parse_timelapse_option(std::shared_ptr<json_obj> obj, ins_timelapse_option& option) const
{
	obj->get_boolean("enable", option.enable);
	obj->get_int("interval", option.interval);
	return INS_OK;
}

int access_msg_parser::parse_origin_option(std::shared_ptr<json_obj> origin_obj, ins_origin_option& option, bool b_pic) const
{
	bool save_origin = false;
	origin_obj->get_boolean(ACCESS_MSG_OPT_SAVE_ORIGIN, save_origin);
	if (save_origin) {
		int32_t loc_mode = b_pic?INS_STORAGE_MODE_DEFAULT:INS_STORAGE_MODE_AB_NV; //默认
		origin_obj->get_int(ACCESS_MSG_OPT_STORAGE_LOC, loc_mode);
		option.storage_mode = loc_mode;
	} else {
		//不支持同时原始流存文件和原始流推流
		origin_obj->get_string("liveUrl", option.live_prefix);
	}

	origin_obj->get_string(ACCESS_MSG_OPT_MIME, option.mime);
	if (option.mime == ACCESS_MSG_OPT_MIME_H264) {
		option.mime = INS_H264_MIME;
	} else if (option.mime == ACCESS_MSG_OPT_MIME_H265) {
		option.mime = INS_H265_MIME;
	} else if (option.mime == ACCESS_MSG_OPT_MIME_JPEG) {
		option.mime = INS_JPEG_MIME;
	}
	else if (option.mime == ACCESS_MSG_OPT_MIME_RAW)
	{
		option.mime = INS_RAW_MIME;
	}
	else if (option.mime == ACCESS_MSG_OPT_MIME_RAW_JPEG)
	{
		option.mime = INS_RAW_JPEG_MIME;
	}
	else
	{
		LOGERR("origin mime:%s unsupport", option.mime.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	origin_obj->get_int(ACCESS_MSG_OPT_FORMAT_WIDTH, option.width);
	origin_obj->get_int(ACCESS_MSG_OPT_FORMAT_HEIGHT, option.height);
	origin_obj->get_int(ACCESS_MSG_OPT_FORMAT_FRAMERATE, option.framerate);
	origin_obj->get_int(ACCESS_MSG_OPT_FORMAT_BITRATE, option.bitrate);
	origin_obj->get_int(ACCESS_MSG_OPT_LOGMODE, option.logmode);
	origin_obj->get_boolean(ACCESS_MSG_OPT_HDR, option.hdr);
	origin_obj->get_int(ACCESS_MSG_OPT_BITDEPTH, option.bitdepth);

	return INS_OK;
}

int access_msg_parser::parse_stich_option(std::shared_ptr<json_obj> stiching_obj, ins_stiching_option& option) const
{
	stiching_obj->get_string(ACCESS_MSG_OPT_MIME, option.mime);
	if (option.mime == ACCESS_MSG_OPT_MIME_H264)
	{
		option.mime = INS_H264_MIME;
	}
	else if (option.mime == ACCESS_MSG_OPT_MIME_H265)
	{
		option.mime = INS_H265_MIME;
	}
	else if (option.mime == ACCESS_MSG_OPT_MIME_JPEG)
	{
		option.mime = INS_JPEG_MIME;
	}
	else
	{
		LOGERR("invalid mime type:%s", option.mime.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}
	stiching_obj->get_int(ACCESS_MSG_OPT_FORMAT_WIDTH, option.width);
	stiching_obj->get_int(ACCESS_MSG_OPT_FORMAT_HEIGHT, option.height);
	stiching_obj->get_int(ACCESS_MSG_OPT_FORMAT_FRAMERATE, option.framerate);
	stiching_obj->get_int(ACCESS_MSG_OPT_FORMAT_BITRATE, option.bitrate);

	std::string map_type;
	stiching_obj->get_string(ACCESS_MSG_OPT_MAP, map_type);
	if (map_type == "cube")
	{
		option.map_type = INS_MAP_CUBE;
	}

	std::string algorithm;
	stiching_obj->get_string(ACCESS_MSG_OPT_ALGORITHM, algorithm);
	if (algorithm == "opticalFlow")
	{
		LOGINFO("opticalflow");
		option.algorithm = INS_ALGORITHM_OPTICALFLOW;
	}
	else
	{
		option.algorithm = INS_ALGORITHM_NORMAL;
	}

	stiching_obj->get_boolean(ACCESS_MSG_OPT_LIVEONHDMI, option.hdmi_display);

	int ret = parse_mode(stiching_obj, option.mode);
	RETURN_IF_NOT_OK(ret);

	//stiching_obj->get_string(ACCESS_MSG_OPT_LENS_SET, option.lens_set);

	std::string live_url;
	stiching_obj->get_string(ACCESS_MSG_OPT_LIVE_URL, live_url);
	if (live_url != "")
	{
		option.url = live_url;
	}

	bool file_save = false; //just for living
	stiching_obj->get_boolean("fileSave", file_save);
	if (file_save) option.url_second = "fileSave"; //just mark here

	stiching_obj->get_string("format", option.format);

	return INS_OK;
}

int access_msg_parser::parse_mode(std::shared_ptr<json_obj> obj, int& mode) const
{
	std::string ss;
	obj->get_string(ACCESS_MSG_OPT_VRMODE, ss);
	if (ss == ACCESS_MSG_OPT_VRMODE_3D)
	{
		mode = INS_MODE_3D_TOP_LEFT;
	}
	else if (ss == ACCESS_MSG_OPT_VRMODE_3D_TOP_LEFT)
	{
		mode = INS_MODE_3D_TOP_LEFT;
	}
	else if (ss == ACCESS_MSG_OPT_VRMODE_3D_TOP_RIGHT)
	{
		mode = INS_MODE_3D_TOP_RIGHT;
	}
	else if (ss == ACCESS_MSG_OPT_VRMODE_PANO)
	{
		mode = INS_MODE_PANORAMA;
	}
	else
	{
		LOGERR("invalid mode:%s", ss.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	return INS_OK;
}

int access_msg_parser::parse_audio_option(std::shared_ptr<json_obj> audio_obj, ins_audio_option& option) const
{
	if (audio_obj->get_string(ACCESS_MSG_OPT_MIME, option.mime)) {
		if (option.mime == ACCESS_MSG_OPT_MIME_AAC) {
			option.mime = INS_AAC_MIME;
		} else {
			LOGERR("invalid audio mime type:%s", option.mime.c_str());
			return INS_ERR_INVALID_MSG_PARAM;
		}
	}else {	//default
		option.mime = INS_AAC_MIME;
	}

	if (!audio_obj->get_int(ACCESS_MSG_OPT_FORMAT_SAMPLERATE, option.samplerate)) {
		LOGERR("no sample rate in json msg");
		return INS_ERR_INVALID_MSG_FMT;
	}

	if (!audio_obj->get_int(ACCESS_MSG_OPT_FORMAT_BITRATE, option.bitrate)) {
		LOGERR("no bitrate in json msg");
		return INS_ERR_INVALID_MSG_FMT;
	}

	if (!audio_obj->get_string(ACCESS_MSG_OPT_FORMAT_SAMPLE_FORMAT, option.samplefmt)) {
		LOGERR("no sample format in json msg");
		return INS_ERR_INVALID_MSG_FMT;
	}

	// if (!audio_obj->get_string(ACCESS_MSG_OPT_FORMAT_CHANNEL_LAYOUT, option.ch_layout))
	// {
	// 	LOGERR("no channel layout in json msg");
	// 	return INS_ERR_INVALID_MSG_FMT;
	// }

	//just support mono now
	option.ch_layout = ACCESS_MSG_OPT_CHLAYOUT_MONO;

	return INS_OK;
}

std::string access_msg_parser::gen_file_prefix(std::string type)
{
	struct timeval time;
	gettimeofday(&time, nullptr); 
	struct tm* now = localtime(&time.tv_sec);
	char date_str[256] = {0};

	snprintf(date_str, 255, "%04d%02d%02d_%02d%02d%02d", 
			now->tm_year + 1900,
			now->tm_mon + 1,
			now->tm_mday,
			now->tm_hour,
			now->tm_min,
			now->tm_sec);

	std::string url = type + "_" + std::string(date_str);

	return url;
}

std::string access_msg_parser::gen_video_file_prefix(ins_video_option& opt)
{
	struct timeval time;
	gettimeofday(&time, nullptr); 
	struct tm* now = localtime(&time.tv_sec);
	char date_str[256] = {0};

	snprintf(date_str, 255, "%04d%02d%02d_%02d%02d%02d", 
			now->tm_year + 1900,
			now->tm_mon + 1,
			now->tm_mday,
			now->tm_hour,
			now->tm_min,
			now->tm_sec);

	/*
	 * 录像还是直播存片
	 */
	std::string type = "VID";
	if (opt.name == ACCESS_CMD_START_RECORD) {
		if (opt.timelapse.enable) {	/* Raw和非Raw */
			type = "PIC_TLP";
		} else {	
			/* 1.如果是街景  VID_GSV*/

			/* 2.如果是10bit */

			/* 3.如果是60fps */

			/* 4.如果支持LOG模式 */

			/* 5.如果支持HDR模式 */
		}
	} else if (opt.name == ACCESS_CMD_START_LIVE) {
		type = "VID_LIV";
	} else {
		LOGERR("invalid command name: %s", opt.name.c_str());
	}

	std::string url = type + "_" + std::string(date_str);

	return url;
}


/*
 * PIC_XXXX_XXXX_XXX
 * PIC_RAW_XXXX_XXXX_XXX
 * PIC_AEB[3,5,7,9]_XXXX_XXXX
 * PIC_BST_XXXX_XXXX
 */
std::string access_msg_parser::gen_pic_file_prefix(std::string type, ins_picture_option& opt)
{
	struct timeval time;
	gettimeofday(&time, nullptr); 
	struct tm* now = localtime(&time.tv_sec);
	char date_str[256] = {0};

	std::string prefix = type;
	
	if (opt.burst.enable) {	/* Burst模式 - Raw存Amba */
		prefix += "_BST";
	} else if (opt.bracket.enable) {	/* bracket模式 - Raw存Amba */
		if (opt.bracket.count == 3) {
			prefix += "_AEB3";
		} else if (opt.bracket.count == 5) {
			prefix += "_AEB5";
		} else if (opt.bracket.count == 7) {
			prefix += "_AEB7";
		} else if (opt.bracket.count == 9) {
			prefix += "_AEB9";
		} else {
			LOGERR("invalid aeb count given: %d", opt.bracket.count);
		}
	} else {	/* 普通模式 */
		LOGINFO("normal mode: mime: %s", opt.origin.mime.c_str());
		if (opt.origin.mime == "raw+jpeg") {
			prefix += "_RAW";
		}
	}

	snprintf(date_str, 255, "%04d%02d%02d_%02d%02d%02d", 
			now->tm_year + 1900,
			now->tm_mon + 1,
			now->tm_mday,
			now->tm_hour,
			now->tm_min,
			now->tm_sec);

	std::string url = prefix + "_" + std::string(date_str);

	return url;
}



int access_msg_parser::create_file_dir(std::string prefix, std::string& path)
{	
	bool b_custom_path = true;
	
	if (path == "") {
		b_custom_path = false;
		path = storage_path_ + "/" + prefix;
	}

	for (int i = 0; ; i++) {
		//路径不存在，创建路径
		if (access(path.c_str(), 0)) {
			if (mkdir(path.c_str(), 0755)) {
				LOGERR("mkdir:%s fail", path.c_str());
				path = "";
				return INS_ERR_FILE_OPEN;
			} else {
				LOGINFO("create dir:%s success", path.c_str());
				return INS_OK;
			}
		} else if (b_custom_path) {	// 如果是指定的路径，就覆盖写
			return INS_OK;	
		} else {	/* 如果自动生成的路径，不能覆盖 */
			LOGINFO("dir:%s exist, change dirname", path.c_str());
			std::stringstream ss;
			ss << storage_path_ << "/" << prefix << "_" << i;
			path = ss.str();
		}
	}
}

int access_msg_parser::check_video_option(const ins_video_option& option) const
{	
	int ret = INS_OK;

	if (option.b_stiching) {	/* 使能了实时拼接 */
		/*
	 	 * 原始流的帧率与拼接流的帧率不一致(拼接流为非jpeg), 错误
	 	 */
		if (option.origin.framerate != option.stiching.framerate && option.stiching.format != "jpeg") {
			LOGERR("origin fps:%d != stiching fps:%d", option.origin.framerate, option.stiching.framerate);
			return INS_ERR_INVALID_MSG_PARAM;
		}

		ret = check_stich_option(option.stiching);
		RETURN_IF_NOT_OK(ret);

		if (option.origin.storage_mode == INS_STORAGE_MODE_NV 
			|| option.origin.storage_mode == INS_STORAGE_MODE_NONE) {
			int ret = check_origin_video_option(option.origin, true, option.timelapse.enable);
			RETURN_IF_NOT_OK(ret);
		} else {
			if (option.origin.width > 3840 || option.origin.height > 2400 
				|| option.stiching.framerate > 60) {
				LOGERR("storage in module & stiching,w:%d h:%d fps:%d not support", 
					option.origin.width, option.origin.height, option.stiching.framerate);
				return INS_ERR_INVALID_MSG_PARAM;
			}

			//如果拼接且存原始流在amba,那么实际拼接的就是辅码流不是主码流，所以这里不用检测
			int ret = check_origin_video_option(option.origin, false, option.timelapse.enable);
			RETURN_IF_NOT_OK(ret);
		}
	} else {
		ret = check_origin_video_option(option.origin, false, option.timelapse.enable);
		RETURN_IF_NOT_OK(ret);
	}

	if (option.b_audio) {	/* 使能音频时, 检查音频参数是否合法 */
		ret = check_audio_option(option.audio);
		RETURN_IF_NOT_OK(ret);
	}

	if (option.timelapse.enable) {	/* 使能timelapse时,检查timelapse的间隔是否大于2s */
		if (option.timelapse.interval < 2000) {
			LOGERR("timelapse interval:%d < 2000ms", option.timelapse.interval);
			return INS_ERR_INVALID_MSG_PARAM;
		}
	}

	return INS_OK;
}


int access_msg_parser::check_stich_option(const ins_stiching_option& option) const
{
	/*
 	 * 实时拼接参数限制: 宽高不能大于4096,小于64, 帧率不能大于60
 	 */
	if (option.width > 4096 
		|| option.height > 4096 
		|| option.width < 64 
		|| option.height < 64
		|| option.framerate > 60) {
		LOGERR("stich option w:%d h:%d fps:%d not support", option.width, option.height, option.framerate);
		return INS_ERR_INVALID_MSG_PARAM;
	}

	// if (option.width*option.height > 4096*2048) {
	// 	LOGERR("stich option w:%d h:%d fps:%d not support", option.width, option.height, option.framerate);
	// 	return INS_ERR_INVALID_MSG_PARAM;
	// }

	return INS_OK;
}

int access_msg_parser::check_origin_video_option(const ins_origin_option& option, bool b_stiching, bool b_timelapse) const
{
	static const ins_resolution origin_fmts[] =  {
		{960, 720, 25, true},
		{960, 720, 30, true},
		{1920, 1080, 25, true},
		{1920, 1080, 30, true},
		{1920, 1080, 50, true},
		{1920, 1080, 60, true},
		{1920, 1080, 100, false},
		{1920, 1080, 120, false},

		{1280, 960, 25, true},
		{1280, 960, 30, true},

		{1920, 1440, 25, true},
		{1920, 1440, 30, true},
		{1920, 1440, 50, true},
		{1920, 1440, 60, true},
		{1920, 1440, 120, false},

		{2560, 1440, 25, true},
		{2560, 1440, 30, true},
		{2560, 1440, 60, true},

		{2560, 1920, 25, true},
		{2560, 1920, 30, true},

		{3200, 2400, 15, false},
		{3200, 2400, 25, false},
		{3200, 2400, 30, false},
		{3200, 2400, 50, false},
		{3200, 2400, 60, false},

		{3840, 1920, 60, false},

		{3840, 2160, 5, false},
		{3840, 2160, 25, false},		
		{3840, 2160, 30, false},
		{3840, 2160, 50, false},
		{3840, 2160, 60, false},

		{3840, 2880, 25, false},
		{3840, 2880, 30, false},
	};

	if (b_timelapse) {
		// if (option.width != 4000 || option.height != 3000)
		// {
		// 	LOGERR("origin w:%d h:%d not support for timelapse", option.width, option.height);
		// 	return INS_ERR_INVALID_MSG_PARAM;
		// }
		// else
		{
			return INS_OK;
		}
	}

	bool b_resolution_support = false;
	for (unsigned int i = 0; i < sizeof(origin_fmts)/sizeof(ins_resolution); i++) {
		if (option.width == origin_fmts[i].w 
			&& option.height == origin_fmts[i].h 
			&& option.framerate == origin_fmts[i].fps 
			&& (!b_stiching || origin_fmts[i].rt)) {
			b_resolution_support = true;
			break;
		}
	}

	// if (!b_resolution_support) {
	// 	LOGERR("origin resolution w:%d h:%d fps:%d rtstich:%d not support", option.width, option.height, option.framerate, b_stiching);
	// 	return INS_ERR_INVALID_MSG_PARAM;
	// }

	return INS_OK;
}


int access_msg_parser::check_picture_option(const ins_picture_option& option) const
{
	// if (option.origin.width != 4000 || option.origin.height != 3000)
	// {
	// 	LOGERR("origin w:%d h:%d not support", option.origin.width, option.origin.height);
	// 	return INS_ERR_INVALID_MSG_PARAM;
	// }

	// if (option.b_stiching)
	// {	
	// 	if (option.stiching.width > 8192 || option.stiching.height > 8192 
	// 		|| option.stiching.width < 64 || option.stiching.height < 64)
	// 	{
	// 		LOGERR("stich w:%d h:%d not support", option.stiching.width, option.stiching.height);
	// 		return INS_ERR_INVALID_MSG_PARAM;
	// 	}
	// }

	return INS_OK;
}

int access_msg_parser::check_audio_option(const ins_audio_option& option) const
{
	if (option.samplerate != 48000) {
		LOGERR("samplerate:%d not support", option.samplerate);
		return INS_ERR_INVALID_MSG_PARAM;
	}

	if (option.samplefmt != ACCESS_MSG_OPT_SAMPLEFMT_S16) {
		LOGERR("samplefmt:%s not support", option.samplefmt.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	if (option.ch_layout != ACCESS_MSG_OPT_CHLAYOUT_MONO && option.ch_layout != ACCESS_MSG_OPT_CHLAYOUT_STEREO) {
		LOGERR("channel layout:%s not support", option.ch_layout.c_str());
		return INS_ERR_INVALID_MSG_PARAM;
	}

	return INS_OK;
}
