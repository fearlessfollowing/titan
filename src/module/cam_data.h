
#ifndef _CAM_DATA_H_
#define _CAM_DATA_H_

#include <string>
#include "insbuff.h"

struct cam_video_param {
	std::string mime = "h264"; //h264/h265
	int32_t bitdepth = 8; //8/10
	int32_t width = 0;
	int32_t height = 0;
	int32_t framerate = 0; //整数帧率
	int32_t bitrate = 0; 
	int32_t logmode = 0;
	double rolling_shutter_time = 0;//us
	int32_t gyro_delay_time = 78000;//us
	int32_t gyro_orientation = 0; //1:横放 0:竖放
	int32_t crop_flag = 0; //0:不裁剪 1:裁两边
	int32_t rec_seq = -1; //录像序号

	bool hdr = false;
	bool b_usb_stream = true; 
	bool b_file_stream = false;
	std::string file_url;
};


/*
 * cam_photo_param - 拍照参数(用于与模组交互)
 */
struct cam_photo_param {
	std::string 	type = "photo";
	std::string 	mime = "jpeg";
	int32_t 		width = 0;				/* 照片的宽度 */
	int32_t 		height = 0;				/* 照片的高度 */
	bool 			b_usb_raw = false; 
	bool 			b_usb_jpeg = false; 
	bool 			b_file_raw = false;
	bool 			b_file_jpeg = false;
	
	//bool b_file_stream = false;
	std::string 	file_url;				/* 存照片的文件名 */

	int32_t 		sequence = 0; 			/* 拍timelapse的序列 */
	int32_t 		interval = 0; 			/* timelapse的间隔 */
	int32_t 		count = 1; 				/* 一组照片的张数burst/hdr */
	int32_t 		min_ev = 32; 			/* 最小ev */
	int32_t 		max_ev = 32; 			/* 最大ev */
};

#define AMBA_INVALID_PROPERTY 0x0fffffff

struct cam_image_property {
	std::string property;
	int32_t value;
};

#endif