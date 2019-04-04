#include "timelapse_mgr.h"
#include "common.h"
#include "inslog.h"
#include "cam_img_repo.h"
#include "img_seq_composer.h"
#include "stream_sink.h"
#include "usb_sink.h"
#include "gps_mgr.h"
#include "prj_file_mgr.h"
#include "pic_seq_sink.h"

timelapse_mgr::~timelapse_mgr()
{
	composer_ = nullptr;
	camera_->stop_all_timelapse_ex();
	cam_repo_ = nullptr;
	
	gps_mgr::get()->del_all_output();
	LOGINFO("timelapse mgr destroy");
}



int timelapse_mgr::start(cam_manager* camera, const ins_video_option& option)
{
    camera_ = camera;

    print_option(option);

    cam_repo_ = std::make_shared<cam_img_repo>(INS_PIC_TYPE_TIMELAPSE);
	cam_photo_param param;

	param.type 		= INS_PIC_TYPE_TIMELAPSE;
	param.width 	= option.origin.width;
	param.height 	= option.origin.height;
	param.mime 		= option.origin.mime;
	param.interval 	= option.timelapse.interval;
	param.file_url 	= option.origin.module_url;


	if (option.origin.storage_mode == INS_STORAGE_MODE_NV) {	// 全存大卡
        param.b_usb_jpeg 	= true;
		param.b_usb_raw 	= true;
		param.b_file_jpeg 	= false;
		param.b_file_raw 	= false;

		#if 0
		if (param.mime != INS_RAW_MIME) {
			auto ret = open_composer(option.path, option.timelapse.interval);
			RETURN_IF_NOT_OK(ret);
		}
		#endif
		
    } else if (option.origin.storage_mode == INS_STORAGE_MODE_AB_NV) {	// jpeg存大卡 raw存小卡
		if (param.mime == INS_RAW_MIME || param.mime == INS_RAW_JPEG_MIME) {
			param.b_usb_raw = false;
			param.b_file_raw = true;
		}

		if (param.mime == INS_JPEG_MIME || param.mime == INS_RAW_JPEG_MIME) {
			param.b_usb_jpeg = true;
			param.b_file_jpeg = false;
		}
	} else {	// 全存小卡
        param.b_usb_jpeg = false;`
		param.b_usb_raw = false;
		param.b_file_jpeg = true;
		param.b_file_raw = false;			// true -> false
		//open_usb_sink(option.prj_path);
    }

	if (param.b_usb_jpeg || param.b_usb_raw) {	/* 需要存timelapse的jpeg或raw数据 */

		std::map<uint32_t,std::shared_ptr<pic_seq_sink>> sinks;		
		for (int32_t i = 0; i < INS_CAM_NUM; i++) {
			bool key_sink = (camera_->master_index() == i) ? true : false;
			
			/* 每个模组创建一个pic_seq_sink对象(对应一个处理线程) */
			auto sink = std::make_shared<pic_seq_sink>(option.path, key_sink);
			sinks.insert(std::make_pair(i, sink));
		}

		cam_repo_->set_sink(sinks);

		std::string gyro_name = option.path + "/gyro.mp4";
		auto sink = std::make_shared<stream_sink>(gyro_name);
		sink->set_gyro(true, false);
		
		auto S = std::static_pointer_cast<gyro_sink>(sink);
		cam_repo_->add_gyro_sink(S);
		
		sink->start(0);
	}

	int ret = camera_->start_all_timelapse_ex(param, cam_repo_);	/* 调用cam_manager的start_all_timelapse_ex来启动timelapse的拍摄 */
	RETURN_IF_NOT_OK(ret);

	LOGINFO("timalapse mgr start");
	return INS_OK;
}


void timelapse_mgr::open_usb_sink(std::string path)
{
	std::string filename = path + "/" + INS_PROJECT_FILE;
	usb_sink_ = std::make_shared<usb_sink>();
	
	usb_sink_->open(camera_);
	usb_sink_->set_gps(true);
	gps_mgr::get()->add_output(usb_sink_);

	usb_sink_->set_prj_file(filename);
}


int32_t timelapse_mgr::open_composer(std::string path, uint32_t interval)
{
	compose_option option;
	option.in_w 		= 4000;
	option.in_h 		= 3000;
	option.mime 		= INS_H264_MIME;
	option.width 		= INS_PREVIEW_WIDTH;
	option.height 		= INS_PREVIEW_HEIGHT;
	option.mode 		= INS_MODE_PANORAMA;
	option.map 			= INS_MAP_FLAT;
	option.framerate 	= Arational(1,1);
	option.bitrate 		= 200000*1000 / interval;

	auto sink = std::make_shared<stream_sink>(RTMP_PREVIEW_URL);
	sink->set_video(true);
	sink->set_preview(true);
	sink->set_live(true);
	option.m_sink.insert(std::make_pair(0, sink));

	std::string url = path + "/" + INS_PREVIEW_FILE;

	/*
	 * 构造一个stream_sink
	 */
	sink = std::make_shared<stream_sink>(url);
	sink->set_video(true);
	sink->set_stitching(true);
	option.m_sink.insert(std::make_pair(1, sink));

	prj_file_mgr::add_preview_info(path, option.bitrate, 1);

	composer_ = std::make_shared<img_seq_composer>();
	composer_->set_repo(cam_repo_);

	return composer_->open(option);
}


void timelapse_mgr::print_option(const ins_video_option& option) const
{
    if (option.timelapse.enable) {
        LOGINFO("timelapse option: enable:%d interval:%d", option.timelapse.enable, option.timelapse.interval);
    }

	LOGINFO("origin option: storage:%d mime:%s width:%d height:%d framerate:%d bitrate:%d override:%d tofile:%d stab:%d", 
		option.origin.storage_mode,
		option.origin.mime.c_str(),
		option.origin.width, 
		option.origin.height, 
		option.origin.framerate, 
		option.origin.bitrate, 
		option.b_override, 
		option.b_to_file, 
		option.b_stabilization);

	if (option.b_stiching) {
		LOGINFO("stiching option: mime:%s width:%d height:%d framerate:%d bitrate:%d, mode:%d url:%s",  
			option.stiching.mime.c_str(),
			option.stiching.width, 
			option.stiching.height, 
			option.stiching.framerate, 
			option.stiching.bitrate, 
			option.stiching.mode, 
			option.stiching.url.c_str());
	}
}


