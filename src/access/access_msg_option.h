#ifndef _ACCESS_MSG_OPTION_H_
#define _ACCESS_MSG_OPTION_H_

#include <string>
#include <vector>


/*
 * 参数的origin字段参数
 */
struct ins_origin_option {
	std::string 	mime;				/* mime: "h264"/"h265" */
	int 			bitdepth = 8;		/* bitdepth: 8/10bit */
	int 			width = 0;			/* width: 原片的宽度 */
	int 			height = 0;			/* height: 原片的高度 */
	int 			framerate = 0;		/* framerate:帧率 */
	int 			bitrate = 0; 		/* bitrate: 码率kbits */
	int 			logmode = 0;		/* logmode: 是否为log模式 */
	bool 			hdr = false;		/* hdr: 是否为hdr */
	//std::string url_prefix; 			//store in nvidia
	std::string 	live_prefix; 		//live origin stream(rtmp/rtsp/hls )
	std::string 	module_url; 		/* module_url: 模组中存储的位置 store in module(amba) */
	int 			storage_mode = -1;	/* 原始流的存储模式 */
};

struct ins_stiching_option {
 	int 		mode = 0;
	int 		algorithm = 0;
	int 		map_type = 0;
 	std::string mime;		//h264/h265/jpeg
 	int 		width = 0;
	int 		height = 0;
	int 		framerate = 0;
	int 		bitrate = 0; //kbits
 	std::string url;
	std::string url_second; //file url for living
	bool 		hdmi_display = false;
	std::string format;
 };


/*
 * burst节点的参数
 */
struct ins_burst_option {
	bool enable = false;		/* 是否使能burst */
	int count = 0;				/* burst拍照的张数 */
};

/*
 * hdr节点参数
 */
struct ins_hdr_option {
	bool enable = false;		/* 是否使能 */
	int count = 3;				/* 张数 */
	int min_ev = 32;			/* 最小ev */
	int max_ev = 32;			/* 最大ev */
};

struct ins_timelapse_option {
	bool enable = false;
	int interval = 0;
};

struct ins_auto_connect {
	bool enable = false;
	int interval = 0; //ms
	int count = 0;
};

struct ins_audio_option {
	std::string 	mime;//aac
	std::string 	samplefmt = "s16"; //s16 f32 
	std::string 	ch_layout = "stereo"; //mono stereo
	int 			samplerate = 0;
	int 			bitrate = 0;
	int 			type = 0;
	bool 			fanless = false; //是否无风扇模式
};

struct ins_picture_option {
 	ins_origin_option 		origin;
 	bool 					b_stiching = false;
 	ins_stiching_option 	stiching;
	ins_burst_option 		burst;
	ins_hdr_option 			hdr;
	ins_hdr_option 			bracket;
	ins_timelapse_option 	timelapse;
 	unsigned int 			delay = 0;//second
	std::string 			path;
	std::string 			prj_path;
	bool 					b_stabilization = false;
	bool 					b_thumbnail = true;
	std::string 			logo_file;
	int index = -1;
 };

struct ins_video_option {
	ins_timelapse_option 	timelapse;
	ins_origin_option 		origin;
	bool 					b_stiching = false;
	ins_stiching_option 	stiching;
	bool 					b_audio = false;
	ins_audio_option 		audio;
	int 					delay = 0;
	int 					duration = -1;//second
	int 					index = -1; //单镜头合焦HDMI预览
	std::string 			path;
	std::string 			prj_path;
	bool 					b_override = false; //文件覆盖写
	bool 					b_to_file = true; //不写文件只保存最后一帧
	bool 					b_stabilization = false;
	ins_auto_connect 		auto_connect; // for living
	std::string 			logo_file;
	uint8_t 				type = 0; 	/* 0:preview; 1:rec; 2:live */
 };

struct ins_video_file_option {
	std::vector<std::string> 	url;
	ins_stiching_option 		stiching;
	ins_audio_option 			audio;
};


struct ins_picture_file_option {
	std::vector<std::string> url;
	ins_stiching_option stiching;
};

struct ins_offset_option {
	std::string pano_4_3;
	std::string pano_16_9;
	std::string left_3d;
	std::string right_3d;
	bool 		factory_setting = false;
	bool 		using_factory_offset = false;
};

struct ins_image_property_option {
	int 		index = -1; 	/* index: 操作模组的索引值 -1:表示设置所有模组 */
	std::string property;		/* 属性名 */
	int 		value = 0;		/* 属性值 */
};


#endif