#ifndef _ACCESS_MSG_PARSER_H_
#define _ACCESS_MSG_PARSER_H_

#include <string>
#include <map>
#include "json_obj.h"
#include "access_msg_option.h"
#include "access_msg.h"

class access_msg_parser
{
public:
	friend class access_msg_center;
	void setup();
	std::string parse_cmd_name(const char* msg);
	int preview_option(const char* msg, ins_video_option& opt);
	int record_option(const char* msg, ins_video_option& opt);
	int live_option(const char* msg, ins_video_option& opt);
	int take_pic_option(const char* msg, ins_picture_option& opt);
	int offset_option(const char* msg, ins_offset_option& opt);
	int option_option(const char* msg, std::map<std::string, int>& opt1, std::map<std::string, std::string>& opt2);
	int option_get_option(const char* msg, std::string& property);
	int hdmi_option(const char* msg, ins_video_option& opt);
	int storage_option(const char* msg, std::string& path);
	//int fw_version_option(const char* msg, int& index);
	int upgrade_fw_option(const char* msg, std::string& path, std::string& version);
	int internal_pic_over(const char* msg, int& code, std::string& path);
	int power_mode_option(const char* msg, std::string& mode);
	int format_camera_moudle_option(const char* msg, int& index);
	int calibration_option(const char* msg, std::string& mode, int& delay);
	int storage_speed_test_option(const char* msg, std::string& path);
	int module_log_file_option(const char* msg, std::string& file_name);
	int singlen_preview_option(const char* msg, ins_video_option& opt);
	int singlen_take_pic_option(const char* msg, ins_picture_option& opt);
	int update_gamma_curve_option(const char* msg, std::string& data);
	int calibration_blc_option(const char* msg, bool& b_reset);
	int systime_change_option(const char* msg, int64_t& timestamp, std::string& source);

private:
	int parase_index(const std::shared_ptr<json_obj> obj, int& index, int defaut_index) const;
    int parse_video_option(std::shared_ptr<json_obj> param_obj, ins_video_option& option) const;
	int parse_picture_option(std::shared_ptr<json_obj> param_obj, ins_picture_option& option) const ;
	int parse_origin_option(std::shared_ptr<json_obj> stiching_obj, ins_origin_option& option, bool b_pic = false) const;
	int parse_stich_option(std::shared_ptr<json_obj> origin_obj, ins_stiching_option& option) const;
	int parse_picture_file_option(std::shared_ptr<json_obj> param_obj, ins_picture_file_option& option) const;
	int parse_video_file_option(std::shared_ptr<json_obj> param_obj, ins_video_file_option& option) const;
	int parse_origin_cam_option(std::shared_ptr<json_obj> param_obj, ins_origin_option& option, int& index) const;
	int parse_audio_option(std::shared_ptr<json_obj> audio_obj, ins_audio_option& option) const;
	int parse_mode(std::shared_ptr<json_obj> obj, int& mode) const;
	//int parse_img_option(std::shared_ptr<json_obj> img_obj, ins_img_option& option) const;
	int parse_timelapse_option(std::shared_ptr<json_obj> obj, ins_timelapse_option& option) const;

	int check_video_option(const ins_video_option& option) const;
	int check_stich_option(const ins_stiching_option& option) const;
	int check_origin_video_option(const ins_origin_option& option, bool b_stiching, bool b_timelapse) const;
	int check_picture_option(const ins_picture_option& option) const;
	int check_audio_option(const ins_audio_option& option) const;

	int create_file_dir(std::string type, std::string& path);
	std::string gen_file_prefix(std::string type);
	int check_disk_space(unsigned int size, std::string path);
	int hls_dir_prepare(std::string dir);
	int32_t get_audio_type(bool stitch);

	bool b_fanless_ = false;
	bool b_pano_audio_ = true;
	bool b_audio_to_stitch_ = true;
	bool b_stab_rt_ = false;
	bool b_logo_on_ = false;

	std::string storage_path_;
	std::string storage_path2_;
	std::string preview_url_;
	std::string live_url_;
};

#endif