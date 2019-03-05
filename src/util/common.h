#ifndef _INS_UTIL_UTIL_H_
#define _INS_UTIL_UTIL_H_

#include "stdlib.h"
#include "inserr.h"

#define INS_DIR "/home/nvidia/insta360"

#define INS_FIFO_TO_SERVER   		INS_DIR"/fifo/ins_fifo_to_server"
#define INS_FIFO_TO_CLIENT   		INS_DIR"/fifo/ins_fifo_to_client"
#define INS_FIFO_TO_CLIENT_A 		INS_DIR"/fifo/ins_fifo_to_client_a"
#define INS_FIFO_TO_SERVER_FATHER  	INS_DIR"/fifo/ins_fifo_to_server_father"
#define INS_FIFO_TO_CLIENT_FATHER  	INS_DIR"/fifo/ins_fifo_to_client_father"


#define RTMP_PREVIEW_URL 			"rtmp://127.0.0.1/live/preview"
#define RTSP_PREVIEW_URL 			"rtsp://127.0.0.1/live/preview"
#define RTMP_LIVE_URL    			"rtmp://127.0.0.1/live/live"
#define HLS_PREVIEW_URL  			"/tmp/preview/preview.m3u8"
#define HLS_LIVE_URL     			"/tmp/live/live.m3u8"
#define HLS_PREVIEW_STREAM_DIR   	"/tmp/preview"
#define HLS_LIVE_STREAM_DIR      	"/tmp/live"

#define INS_DEFAULT_XML_PATH 		INS_DIR"/etc"
#define INS_DEFAULT_XML_FILE 		INS_DIR"/etc/cam_config.xml"
#define INS_DEFAULT_XML_2_FILE 		INS_DIR"/etc/cam_config_2.xml"
#define INS_FACTORY_SETING_XML 		INS_DIR"/etc/factory_setting.xml"
#define INS_GYRO_OFFSET_XML 		INS_DIR"/etc/gyro_offset.xml"
#define INS_GYRO_CALIBRATION_XML 	INS_DIR"/etc/gyro_calibration.xml"
#define INS_GYRO_DELAY_XML 			INS_DIR"/etc/gyro_delay.xml"

#define CAMERA_VERSION "1.1"

#define INS_GYRO_VERSION "4"

#define INS_LOG_PATH 				INS_DIR"/log"

#define PID_LOCK_FILE 				"/var/run/camerad.pid"

#define INS_TASK_LIST_DB_DIR 		INS_DIR"/database"
#define INS_TASK_LIST_DB_PATH 		INS_DIR"/database/box_task"

#define INS_PROJECT_FILE 			"pro.prj"
#define INS_TMEP_PRJ_PATH 			INS_DIR

#define INS_NOISE_SAMPLE_PATH 		INS_DIR"/noise_sample"
#define INS_DEPTH_MAP_FILE 			INS_DIR"/depthMap.dat"
#define INS_DEPTH_MAP_FILE_NAME 	"depthMap.dat"

#define INS_GPS_DATA_FILE 			"gps.dat"

#define INS_AUDIO_DEV_NAME 			"insta360"
#define INS_H2N_AUDIO_DEV_NAME 		"H2n"
#define INS_H3VR_AUDIO_DEV_NAME 	"H3-VR"

#define INS_AUDIO_N_N  0 //原始没有 + 拼接没有
#define INS_AUDIO_N_C  1 //原始没有 + 拼接普通
#define INS_AUDIO_Y_N  2 //原始有 + 拼接没有
#define INS_AUDIO_Y_C  3 //原始有 + 拼接普通
#define INS_AUDIO_Y_S  4 //原始有 + 拼接全景声

#define INS_STORAGE_MODE_NONE -1
#define INS_STORAGE_MODE_NV 0
#define INS_STORAGE_MODE_AB_NV 1
#define INS_STORAGE_MODE_AB 2
#define INS_STORAGE_MODE_DEFAULT 3

#define INS_PREVIEW   0
#define INS_RECORD    1
#define INS_LIVE      2
#define INS_TAKE_PIC  3

#define INS_AUDIO_FRAME_SIZE 1024
// #define INS_AUDIO_ORIGIN 0x01
// #define INS_AUDIO_S_COMMON 0x02
// #define INS_AUDIO_S_SPATIAL 0x04

#define INS_MAKE "Insta360"
#define INS_MODEL "Insta360 Titan"

#define INS_SND_TYPE_NONE   0
#define INS_SND_TYPE_INNER  1
#define INS_SND_TYPE_35MM   2
#define INS_SND_TYPE_USB    3

#define INS_BLEND_TYPE_TEMPLATE 0
#define INS_BLEND_TYPE_OPTFLOW  1

#define INS_MODE_PANORAMA 0
#define INS_MODE_3D_TOP_LEFT 1
#define INS_MODE_3D_TOP_RIGHT 2

#define INS_MAP_FLAT 	0				/* "flat" */	
#define INS_MAP_CUBE 	1				/* "cube" */

#define INS_ALGORITHM_NORMAL 		0
#define INS_ALGORITHM_OPTICALFLOW 	1

#define INS_MEDIA_NONE      0
#define INS_MEDIA_AUDIO     1
#define INS_MEDIA_VIDEO     2
#define INS_MEDIA_SUBTITLE  3
#define INS_MEDIA_CAMM      4
#define INS_MEDIA_CAMM_GYRO  5
#define INS_MEDIA_CAMM_EXP   6
#define INS_MEDIA_CAMM_GPS   7

#define INS_AUDIO_TYPE_NONE 0
#define INS_AUDIO_TYPE_NORMAL 1
#define INS_AUDIO_TYPE_SPATIAL 2

#define INS_SPEED_FAST 0
#define INS_SPEED_MEDIUM 1
#define INS_SPEED_SLOW 2

#define INS_MIN_SPACE_MB 1024

#define INS_CAM_NUM 8

#define INS_PREVIEW_FILE "preview.mp4"
#define INS_PREVIEW_WIDTH   1920
#define INS_PREVIEW_HEIGHT  960
#define INS_PREVIEW_BITRATE 3000 //Kbits

#define INS_THUMBNAIL_WIDTH 1920
#define INS_THUMBNAIL_HEIGHT 960

#define INS_THUMBNAIL_WIDTH_CUBE 1440
#define INS_THUMBNAIL_HEIGHT_CUBE 960

#define INS_FANLESS_START_TEMP 65
#define INS_FANLESS_STOP_TEMP 85

#define INS_DEFAULT_AUDIO_GAIN 64

#define INS_GPS_STATE_NONE 0
#define INS_GPS_STATE_UNLOCATED 1
#define INS_GPS_STATE_2D 2
#define INS_GPS_STATE_3D 3

#define INS_GYRO_ORIENTATION_VERTICAL 0
#define INS_GYRO_ORIENTATION_HORIZON  1
#define INS_GYRO_ORIENTATION_TITAN    2

#define INS_STABLZ_TYPE_XYZ  0
#define INS_STABLZ_TYPE_Z 1

#define INS_CROP_FLAG_PIC 0
#define INS_CROP_FLAG_4_3 1
#define INS_CROP_FLAG_16_9 2
#define INS_CROP_FLAG_2_1 3
#define INS_CROP_FLAG_11 11
#define INS_CROP_FLAG_12 12
#define INS_CROP_FLAG_13 13

#define INS_OFFSET_FACTORY 0
#define INS_OFFSET_USER 1

#define INS_H264_MIME "h264"
#define INS_H265_MIME "h265"
#define INS_AAC_MIME  "aac"
#define INS_JPEG_MIME "jpeg"
#define INS_RAW_MIME  "raw"
#define INS_RAW_JPEG_MIME  "raw+jpeg"

//#define INS_PIC_TYPE_RAW "raw"
#define INS_PIC_TYPE_BURST "burst"
#define INS_PIC_TYPE_HDR "hdr"
#define INS_PIC_TYPE_BRACKET "bracket"
#define INS_PIC_TYPE_TIMELAPSE "timelapse"
//#define INS_PIC_TYPE_JPEG "jpeg"
#define INS_PIC_TYPE_PHOTO "photo" //normal
#define INS_VIDEO_TYPE "video"

#define INS_CAM_ALL_INDEX -1
#define INS_CAM_MASTER_INDEX -2

#define INS_RAW_EXT ".dng"
#define INS_JPEG_EXT ".jpg"

#define INS_LOGO_FILE "_logo.png"

#define INS_COLOR_FORMAT_SURFACE   0x7F000789
#define INS_AAC_OBJ_LC                           2
#define INS_H264_PROFILE_HIGH             0x08
#define INS_H264_LEVEL_50                     0x4000
#define INS_H264_LEVEL_40                     0x800

#define INS_MC_KEY_PROFILE                     "profile"
#define INS_MC_KEY_LEVEL                         "level"
#define INS_MC_KEY_MIME                          "mime"
#define INS_MC_KEY_WIDTH 		             "width"
#define INS_MC_KEY_HEIGHT 		             "height"
#define INS_MC_KEY_COLOR_FORMAT        "color-format"
#define INS_MC_KEY_IFRMAE_INTERVAL     "i-frame-interval"
#define INS_MC_KEY_FRAME_RATE              "frame-rate"
#define INS_MC_KEY_BITRATE                      "bitrate"
#define INS_MC_KEY_AAC_PROFILE              "aac-profile"
#define INS_MC_KEY_SAMPLE_RATE             "sample-rate"
#define INS_MC_KEY_CHANNEL_CNT            "channel-count"
#define INS_MC_KEY_CSD0                            "csd-0"
#define INS_MC_KEY_CSD1                     	  "csd-1"

#define SYS_PROPERTY_CPU_TEMP "sys.cpu_temp"
#define SYS_PROPERTY_GPU_TEMP "sys.gpu_temp"
#define SYS_PROPERTY_BATTERY_TEMP "sys.bat_temp"
#define SYS_PROPERTY_BATTERY_EXIST "sys.bat_exist"
#define SYS_PROPERTY_AUDIO_ON "sys.use_audio"
#define SYS_PROPERTY_STAB_ON  "sys.stab_on"

#define INS_PTS_NO_VALUE ((long long)0x8000000000000000)
#define INS_MAX_INT64    ((long long)0x7fffffffffffffff)

#define INS_ABS_MAX(a,b) (((abs(a)) > (abs(b))) ? (a) : (b))
#define INS_ABS_MIN(a,b) (((abs(a)) < (abs(b))) ? (a) : (b))

#define RETURN_IF_NOT_OK(err) \
if (INS_OK != err) \
{ \
	return err; \
}

#define RETURN_IF_TRUE(cond, str, ret) \
if (cond) \
{ \
    LOGERR("%s", str); \
    return ret; \
} 

#define BREAK_IF_TRUE(cond, str) \
if (cond) \
{ \
	LOGERR("%s", str); \
    break; \
}

#define BREAK_IF_TRUE_S(cond) \
if (cond) \
{ \
    break; \
}

#define BREAK_IF_NOT_OK(err) \
if (INS_OK != err) \
{ \
	break; \
}

#define BREAK_IF_NOT_OK_SET_EXCETPTION(err) \
if (INS_OK != err) \
{ \
	b_module_exception_ = true;\
	break; \
}

#define INS_THREAD_JOIN(thread) \
if (thread.joinable()) \
{\
	thread.join();\
}

#define INS_DELETE(p) \
if (p) \
{\
delete p;\
p = nullptr;\
}

#define INS_DELETE_ARRAY(p) \
if (p) \
{\
delete[] p;\
p = nullptr;\
}

#define INS_FREE(p) \
if (p) \
{\
free(p);\
p = nullptr;\
}

#define INS_ASSERT(a) \
if (!(a)) \
{ \
	printf("abort in %s %d\n",  __FILE__, __LINE__); \
	abort(); \
}

#endif