
#ifndef _INS_CONFIG_H_
#define _INS_CONFIG_H_

#include "tinyxml2.h"
#include <memory>
#include <string>
#include <vector>
#include <mutex>

using namespace tinyxml2;

//option
#define INS_CONFIG_OPTION                          "option"
#define INS_CONFIG_STORAGE                         "storage"
#define INS_CONFIG_AUDIO_GAIN                      "audiogain"
#define INS_CONFIG_AUDIO_DB_64                     "db_64"
#define INS_CONFIG_AUDIO_DB_96                     "db_96"
#define INS_CONFIG_DENOISE_DB                      "denoisedb"
#define INS_CONFIG_FANLESS_SAMPLE_DENOISE          "fanlessSampleDenoise"
#define INS_CONFIG_FAN_SAMPLE_DENOISE              "fanSampleDenoise"
#define INS_CONFIG_PID                             "pid"
#define INS_CONFIG_FIX_STORAGE                     "fixstorage"
#define INS_CONFIG_MODULE_VERSION                  "moduleVersion"
#define INS_CONFIG_FANLESS                         "fanless"
#define INS_CONFIG_PANO_AUDIO                      "panoAudio"
#define INS_CONFIG_AUDIO_TO_STITCH                 "audiotoStitch"
#define INS_CONFIG_RT_STAB                         "rtStablization"
#define INS_CONFIG_STABLZ_TYPE                     "stablization_type"
#define INS_CONFIG_LOGO                            "logo"
#define INS_CONFIG_VIDEO_FRAGMENT                  "videoFragment"
#define INS_CONFIG_START_TEMP                       "startTemp"
#define INS_CONFIG_STOP_TEMP                        "stopTemp"
//#define INS_CONFIG_ERROR_NO                         "errorno"
//gyro
// #define INS_CONFIG_GYRO                            "gyro"
// #define INS_CONFIG_GYRO_X_OFFSET                   "gyro_x_offset"
// #define INS_CONFIG_GYRO_Y_OFFSET                   "gyro_y_offset"
// #define INS_CONFIG_GYRO_Z_OFFSET                   "gyro_z_offset"
// #define INS_CONFIG_ACCEL_X_OFFSET                  "accel_x_offset"
// #define INS_CONFIG_ACCEL_Y_OFFSET                  "accel_y_offset"
// #define INS_CONFIG_ACCEL_Z_OFFSET                  "accel_z_offset"

// #define INS_CONFIG_D_CALIBRAION                    "driver_calibration"
// #define INS_CONFIG_D_GYRO_X                        "d_gryo_x"
// #define INS_CONFIG_D_GYRO_Y                        "d_gryo_y"
// #define INS_CONFIG_D_GYRO_Z                        "d_gryo_z"
// #define INS_CONFIG_D_ACCEL_X                       "d_accel_x"
// #define INS_CONFIG_D_ACCEL_Y                       "d_accel_y"
// #define INS_CONFIG_D_ACCEL_Z                       "d_accel_z"
 
//offset
#define INS_CONFIG_OFFSET                           "offset"
#define INS_CONFIG_OFFSET_PANO_4_3                  "offset_pano_4_3"
// #define INS_CONFIG_OFFSET_PANO_16_9                 "offset_pano_16_9"
// #define INS_CONFIG_OFFSET_PANO_2_1                  "offset_pano_2_1"
// #define INS_CONFIG_OFFSET_3D_LEFT                   "offset_3d_left"
// #define INS_CONFIG_OFFSET_3D_RIGHT                  "offset_3d_right"

class xml_config
{
public:	  
	static bool is_user_offset_default();
	static int set_factory_offset(std::string& offset);
	static std::string get_factory_offset();
	static std::string get_offset(int32_t crop_flag, int32_t type = 0);
	static int set_accel_offset(double x, double y, double z);
	static int get_accel_offset(double& x, double& y, double& z);
	static int set_gyro_rotation(const std::vector<double>& quat);
	static int get_gyro_rotation(std::vector<double>& quat);
	static int get_gyro_delay_time(int32_t w, int32_t h, int32_t framerate, bool hdr = false);

	static int get_gyro_delay_time(int32_t w, int32_t h, int32_t framerate, int32_t bit_depth = 8, bool hdr = false);

	static int get_value(const char* firstname, const char* secondname, int& value);
	static int get_value(const char* firstname, const char* secondname, float& value);
	static int get_value(const char* firstname, const char* secondname, double& value);
	static int get_value(const char* firstname, const char* secondname, std::string& value);
	static int set_value(const char* firstname, const char* secondname, int value);
	static int set_value(const char* firstname, const char* secondname, float value);
	static int set_value(const char* firstname, const char* secondname, double value);
	static int set_value(const char* firstname, const char* secondname, std::string value);

private:
	static XMLElement* find_element(XMLDocument& xml_doc, const char* firstname, const char* secondname);
	static int load_file(XMLDocument& xml_doc, std::string filename);
	static int save_file(XMLDocument& xml_doc, std::string filename);
	static int recreate_config_file(std::string filename);
	static std::mutex mtx_;
};

#endif

