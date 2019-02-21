
#ifndef _PROJECT_FILE_H_
#define _PROJECT_FILE_H_

#include <string>
#include <map>
#include "access_msg_option.h"
#include "metadata.h"

class cam_video_param;

class prj_file_mgr
{
public:
    static int32_t add_origin_frag_file(const std::string& path, int sequence);
    static int32_t create_vid_prj(const ins_video_option& opt, std::string path);
    static int32_t create_pic_prj(const ins_picture_option& opt, std::string path);
    static int32_t add_audio_info(const std::string& path, std::string dev_name, bool b_spatial,int32_t storage_loc, bool single);
    static int32_t add_gyro_gps(const std::string& path,int32_t storage_loc, double rs_time, int32_t delay_time, int32_t orientation, bool single);
    static int32_t updata_video_info(std::string path, cam_video_param* param);
    static int32_t add_aux_file_info(std::string path, const std::shared_ptr<cam_video_param> aux_info, int32_t storage_loc);
    static int32_t add_preview_info(std::string path, uint32_t bitrate, uint32_t framerate);
    static int32_t add_first_frame_ts(std::string path, std::map<uint32_t, int64_t>& first_frame_ts);
    static int32_t add_first_frame_ts(std::string path, int64_t ts);
};

#endif