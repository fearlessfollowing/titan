#include "video_mgr.h"
#include "cam_manager.h"
#include "all_cam_video_buff.h"
#include "all_cam_origin_stream.h"
#include "common.h"
#include "inslog.h"
#include "stream_sink.h"
#include <sstream>
#include "video_composer.h"
#include "all_cam_video_queue.h"
#include <assert.h>
#include "xml_config.h"
#include "ins_util.h"
#include "gps_mgr.h"
#include "gps_file.h"
#include "audio_mgr.h"
#include "usb_sink.h"
#include "prj_file_mgr.h"
#include "access_msg_center.h"
#include "stabilization.h"

//#define STABILIZER_ENABLE

#define ENCODE_INDEX_MAIN      	0   	/* 编码拼接流 */
#define ENCODE_INDEX_PRE       	1   	/* 编码预览流：文件预览/网络预览 */

#define SINK_INDEX_NET 0 				/* 直播存文件的网络流 或者 预览的网络流 */
#define SINK_INDEX_FILE  1 				/* 直播存文件的文件流 或者 预览的文件流 */

#define VIDEO_BUFF_INDEX_ORIGIN  0    	/* 原始流 */
#define VIDEO_BUFF_INDEX_COMPOSE 1    	/* 拼接流 */

#define AUDIO_INDEX_ORIGIN       0
#define AUDIO_INDEX_STITCH_NET   1
#define AUDIO_INDEX_STITCH_FILE  2
#define AUDIO_INDEX_PRE_NET      3
#define AUDIO_INDEX_PRE_FILE     4

int32_t video_mgr::rec_seq_ = 0;

video_mgr::~video_mgr()
{
    camera_->stop_all_video_rec(); 
	audio_ = nullptr;
	composer_ = nullptr;
	video_buff_ = nullptr;
	gps_mgr::get()->del_all_output();
    LOGINFO("video mgr stop");
}

int32_t video_mgr::get_snd_type()
{
	if (audio_) {
		return audio_->dev_type();
	} else {
		return INS_SND_TYPE_NONE;
	}
}

void video_mgr::set_first_frame_ts(int32_t rec_seq, int64_t ts)
{
	if (rec_seq != rec_seq_) {
		LOGINFO("-------seq:%d %d not same", rec_seq, rec_seq_);
		return;
	}
	
	if (prj_path_ != "") 
		prj_file_mgr::add_first_frame_ts(prj_path_, ts);
	
	if (local_sink_) 
		local_sink_->start(ts);
}

void video_mgr::notify_video_fragment(int32_t sequence)
{
	if (prj_path_ != "") 
		prj_file_mgr::add_origin_frag_file(prj_path_, sequence);
}

void video_mgr::switch_stablz(bool enable)
{
	b_stablz_ = enable;

	if (composer_) {
		if (enable) {
			if (!stablz_) setup_stablz();
			composer_->set_stabilizer(stablz_);
		} else {
			composer_->set_stabilizer(nullptr);
		}
	}
}


int32_t video_mgr::start(cam_manager* camera, const ins_video_option& option)
{
    camera_ 	= camera;
	audio_type_ = option.audio.type;
	prj_path_ 	= option.prj_path;
    
    print_option(option);

	video_composer::init_cuda();

	int enable = 1;
	xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_VIDEO_FRAGMENT, enable);
	b_frag_ = enable ? true : false;

	if (option.b_audio) 	/* 需要音频,打开音频设备 */
		open_audio(option);

	bool storage_aux = false;
	if (option.origin.storage_mode == INS_STORAGE_MODE_AB) {			/* 音频/gps/工程文件都传到模组 */
		open_usb_sink(option.prj_path);  
	} else if (option.origin.storage_mode == INS_STORAGE_MODE_AB_NV) {	/* NV和Amba两边都存 */
		storage_aux = !option.b_stiching;
		if (!storage_aux) 
			open_local_sink(option.path); 	/* 不存辅码流的时候，只能单独存一个音频文件 */
	}
	
    auto ret = open_camera_rec(option, storage_aux);
    RETURN_IF_NOT_OK(ret);

#if 0
	if (storage_aux || option.origin.storage_mode == INS_STORAGE_MODE_NV || option.origin.live_prefix != "") {
		open_origin_stream(option);
	}
#endif

	if (option.b_stiching) {	/* 如果需要实时拼接,打开视频合成器 */
		ret = open_composer(option, option.type == INS_PREVIEW);
		RETURN_IF_NOT_OK(ret);
	}

	/* 1.本身是预览就不用在开启一路预览编码 2.直播原始流没有预览 */
	if (option.type != INS_PREVIEW && option.origin.live_prefix == "" && option.index == -1) {
		ret = open_preview(option);
		RETURN_IF_NOT_OK(ret);
	}


	if (prj_path_ != "" && option.origin.storage_mode != INS_STORAGE_MODE_NONE) {
		prj_file_mgr::updata_video_info(prj_path_, &video_param_);	/* 更新工程文件 */

		if (aux_param_ && storage_aux) {
			aux_param_->crop_flag = video_param_.crop_flag;
			prj_file_mgr::add_aux_file_info(prj_path_, aux_param_, option.origin.storage_mode);
		}
		
		if (audio_) {
			prj_file_mgr::add_audio_info(prj_path_, audio_->dev_name(), audio_->is_spatial(),option.origin.storage_mode,!storage_aux);
		}
		
		prj_file_mgr::add_gyro_gps(prj_path_, 
			option.origin.storage_mode, 
			video_param_.rolling_shutter_time, 
			video_param_.gyro_delay_time, 			/* 陀螺仪延时时间 */
			video_param_.gyro_orientation, 
			!storage_aux);
	}

    LOGINFO("video mgr start");

	return INS_OK;
}


int32_t video_mgr::open_preview(const ins_video_option& option)
{
	if (composer_) {	/* 实时拼接，添加一路预览编码流 */
		if (option.origin.framerate > 60) {	// 大于60帧实时拼接的时候，性能不够出第二路
			LOGINFO("!!!!!!framerate:%d no preview", option.origin.framerate);
			return INS_OK;
		}

		compose_option c_opt;
		c_opt.index = ENCODE_INDEX_PRE;
		c_opt.mime = INS_H264_MIME;
		c_opt.width = INS_PREVIEW_WIDTH;
		c_opt.height = INS_PREVIEW_HEIGHT;
		c_opt.mode = INS_MODE_PANORAMA;
		c_opt.map = INS_MAP_FLAT;
		uint32_t framerate;
		if (option.origin.framerate > 30) {	// 大于30帧实时拼接的时候，性能不够出,预览流降为15fps
			c_opt.bitrate = INS_PREVIEW_BITRATE/2;
			framerate = 15;
		} else {
			c_opt.bitrate = INS_PREVIEW_BITRATE;
			framerate = 30;
		}

		c_opt.framerate = ins_util::to_real_fps(framerate);

		auto sink = std::make_shared<stream_sink>(RTMP_PREVIEW_URL);
		sink->set_video(true);
		sink->set_preview(true);
		sink->set_live(true);
		audio_vs_sink(sink, AUDIO_INDEX_PRE_NET, false);
		c_opt.m_sink.insert(std::make_pair(SINK_INDEX_NET, sink));

		if (option.path != "") {	// 文件预览流
			std::string url = option.path + "/" + INS_PREVIEW_FILE;
			auto sink = std::make_shared<stream_sink>(url);
			sink->set_video(true);
			sink->set_fragment(b_frag_, false); 
			if (!option.b_to_file) sink->set_just_last_frame(true);
			audio_vs_sink(sink, AUDIO_INDEX_PRE_FILE, false);
			c_opt.m_sink.insert(std::make_pair(SINK_INDEX_FILE, sink));

			prj_file_mgr::add_preview_info(option.path, c_opt.bitrate, framerate);
		}

		composer_->add_encoder(c_opt);
	} else {	/* 没有打开composer */
		ins_video_option opt;
		opt.type 				= INS_PREVIEW;
		opt.origin 				= option.origin;
		opt.b_stiching 			= true;
		opt.logo_file 			= option.logo_file;
		opt.stiching.mime 		= INS_H264_MIME;		/* 预览流: h264,1920x960@30fps,3000kbits */
		opt.stiching.width 		= INS_PREVIEW_WIDTH;
		opt.stiching.height 	= INS_PREVIEW_HEIGHT;
		opt.stiching.framerate 	= 30;
		opt.stiching.bitrate 	= INS_PREVIEW_BITRATE;
		opt.stiching.mode 		= INS_MODE_PANORAMA;
		opt.stiching.map_type 	= INS_MAP_FLAT;
		opt.stiching.url 		= RTMP_PREVIEW_URL; 
		if (option.path != "") {
			opt.stiching.url_second = option.path + "/" + INS_PREVIEW_FILE;		/* "preview.mp4" */
			prj_file_mgr::add_preview_info(option.path, opt.stiching.bitrate, opt.stiching.framerate);
		}
		open_composer(opt, true);	/* 用工厂offset */
	}

	return INS_OK;
}

int32_t video_mgr::start_preview(ins_video_option option)
{
	return INS_OK;
}

void video_mgr::stop_preview()
{
	// LOGINFO("stop preview");

	// if (composer_) composer_->encoder_del_output(ENCODE_INDEX_PRE, SINK_INDEX_NET);

	// if (audio_) audio_->del_output(AUDIO_INDEX_PRE_NET);
}

void video_mgr::stop_live_file()
{	
	LOGINFO("stop file record");
	if (video_buff_) {	// origin
		video_buff_->del_output(VIDEO_BUFF_INDEX_ORIGIN);
	}

	//如果原始流存在模组的话，还要停止模组的存储，暂无接口

	if (composer_) {	// stitching
		composer_->encoder_del_output(ENCODE_INDEX_MAIN, SINK_INDEX_FILE);
		composer_->encoder_del_output(ENCODE_INDEX_PRE, SINK_INDEX_FILE);
	}

	if (audio_) { 
		audio_->del_output(AUDIO_INDEX_ORIGIN);
		audio_->del_output(AUDIO_INDEX_STITCH_FILE);
		audio_->del_output(AUDIO_INDEX_PRE_FILE);
	}

	gps_mgr::get()->del_all_output();
	if (video_buff_) video_buff_->del_all_gyro_sink();
}


int32_t video_mgr::open_composer(const ins_video_option& option, bool preview) // 这个preview只用来区分offset使用哪个
{
	uint32_t ori_w, ori_h, ori_fps;
	if (aux_param_ && aux_param_->b_usb_stream) {
		ori_w = aux_param_->width;
		ori_h = aux_param_->height;
		ori_fps = aux_param_->framerate;
	} else {
		ori_w = option.origin.width;
		ori_h = option.origin.height;
		ori_fps = option.origin.framerate;
	}

	if (ori_w*ori_h > 2560*1920 || ori_fps > 60) {
		LOGINFO("!!!!!!orign w:%d h:%d fps:%d not support rtstitch", ori_w, ori_h, ori_fps);
		return INS_ERR;
	}

	assert(composer_ == nullptr);
	assert(option.b_stiching);

	std::vector<unsigned int> v_index;
	for (unsigned int i = 0; i < INS_CAM_NUM; i++) {
		v_index.push_back(i);  
	}
	
	auto Q = std::make_shared<all_cam_video_queue>(v_index, false);
	video_buff_->add_output(VIDEO_BUFF_INDEX_COMPOSE, std::dynamic_pointer_cast<all_cam_queue_i>(Q));

	composer_ = std::make_shared<video_composer>();
	composer_->set_video_repo(Q);

	int32_t ret;
	compose_option param;
	param.in_w 		= ori_w;
	param.in_h 		= ori_h;
	param.index 	= (option.type == INS_PREVIEW) ? ENCODE_INDEX_PRE : ENCODE_INDEX_MAIN;
	param.mime 		= option.stiching.mime;
	param.width 	= option.stiching.width;
	param.height 	= option.stiching.height;
	param.bitrate 	= option.stiching.bitrate;
	
	param.ori_framerate = ins_util::to_real_fps(ori_fps);
	param.framerate = ins_util::to_real_fps(option.stiching.framerate);

	param.mode = option.stiching.mode;
	param.map = option.stiching.map_type;
	param.logo_file = option.logo_file;
	param.hdmi_display = option.stiching.hdmi_display;
	param.crop_flag = video_param_.crop_flag;
	auto default_offset = xml_config::is_user_offset_default();
	
	param.offset_type = (preview && default_offset) ? INS_OFFSET_FACTORY : INS_OFFSET_USER;

	if (preview && option.stiching.format == "jpeg") param.jpeg = true; 

	//以下为sink
	if (option.stiching.url != "" || option.stiching.url_second != "") {
		auto sink = std::make_shared<stream_sink>(option.stiching.url);
		sink->set_video(true);
		sink->set_stitching(true);
		if (option.type == INS_PREVIEW) sink->set_preview(true);
		if (option.stiching.url.find(".mp4", 0) == std::string::npos) {	//网络流
			sink->set_live(true);
			sink->set_auto_connect(option.auto_connect);
			audio_vs_sink(sink, ((option.type == INS_PREVIEW)?AUDIO_INDEX_PRE_NET:AUDIO_INDEX_STITCH_NET), false);
			param.m_sink.insert(std::make_pair(SINK_INDEX_NET, sink));
		} else {	//文件流
			if (option.b_override) sink->set_override(true);
			if (!option.b_to_file) sink->set_just_last_frame(true);
			sink->set_fragment(b_frag_, false);
			if (option.type != INS_PREVIEW) 
			{
				sink->set_gps(true);
				gps_mgr::get()->add_output(sink);
			}
			audio_vs_sink(sink, ((option.type == INS_PREVIEW)?AUDIO_INDEX_PRE_FILE:AUDIO_INDEX_STITCH_FILE), false);
			param.m_sink.insert(std::make_pair(SINK_INDEX_FILE, sink));
		}
		

		if (option.stiching.url_second != "") {	//肯定是文件流
			auto sink = std::make_shared<stream_sink>(option.stiching.url_second);
			sink->set_video(true);
			sink->set_stitching(true);
			sink->set_fragment(b_frag_, false);
			audio_vs_sink(sink, ((option.type == INS_PREVIEW)?AUDIO_INDEX_PRE_FILE:AUDIO_INDEX_STITCH_FILE), false);
			if (option.type != INS_PREVIEW) {
				sink->set_gps(true);
				gps_mgr::get()->add_output(sink);
			} 
			
			if (option.b_override) sink->set_override(true);
			if (!option.b_to_file) sink->set_just_last_frame(true);
			param.m_sink.insert(std::make_pair(SINK_INDEX_FILE, sink));
		}
	}

	ret = composer_->open(param);
	if (ret != INS_OK) {
		composer_ = nullptr;
	} else {
		composer_->set_stabilizer(stablz_); 
	}

	return ret;
}


void video_mgr::open_usb_sink(std::string prj_path)
{
	std::string filename = prj_path + "/" + INS_PROJECT_FILE;
	usb_sink_ = std::make_shared<usb_sink>();
	usb_sink_->open(camera_);
	usb_sink_->set_gps(true);
	gps_mgr::get()->add_output(usb_sink_);
	usb_sink_->set_prj_file(filename);

	if (audio_) {
		usb_sink_->set_audio(true);
		audio_->add_output(AUDIO_INDEX_ORIGIN, usb_sink_);
	}
}

void video_mgr::open_local_sink(std::string path)
{
	if (path == "") return;

	std::string url = path + "/audio.mp4";
	local_sink_ = std::make_shared<stream_sink>(url);
	local_sink_->set_auto_start(false);
	local_sink_->set_gyro(true);
	local_sink_->set_gps(true);
	gps_mgr::get()->add_output(local_sink_);

	if (audio_) {
		local_sink_->set_audio(true);
		audio_->add_output(AUDIO_INDEX_ORIGIN, local_sink_);
	}
}

void video_mgr::open_audio(const ins_video_option& option)
{
	audio_param param;
	param.fanless 		= option.audio.fanless;
	param.type 			= option.audio.type;
	param.hdmi_audio 	= option.stiching.hdmi_display;

	audio_ = std::make_shared<audio_mgr>();
	if (audio_->open(param) != INS_OK) {
		LOGERR("audio open fail");
		audio_ = nullptr;
		send_snd_state_msg(INS_SND_TYPE_NONE, "", false);
	} else {
		send_snd_state_msg(audio_->dev_type(), audio_->dev_name(), audio_->is_spatial());
	}
}

int32_t video_mgr::open_camera_rec(const ins_video_option& option, bool storage_aux)
{
	std::vector<uint32_t> v_index;
	for (uint32_t i = 0; i < INS_CAM_NUM; i++) {
		v_index.push_back(i);
	}
	
	std::string path = option.b_to_file ? option.path : ""; //老化不存文件，也不存陀螺仪数据
	video_buff_ = std::make_shared<all_cam_video_buff>(v_index, path);

	std::map<int32_t,std::shared_ptr<cam_video_buff_i>> m_queue;
	for (int32_t i = 0; i < INS_CAM_NUM; i++) {
		m_queue.insert(std::make_pair(i, video_buff_));
	}

	cam_video_param param;
	param.mime = option.origin.mime;
	param.bitdepth = option.origin.bitdepth;
	param.width = option.origin.width;
	param.height = option.origin.height;
	param.framerate = option.origin.framerate;
	param.bitrate = option.origin.bitrate;
	param.logmode = option.origin.logmode;
	param.hdr = option.origin.hdr;
	param.rec_seq = ++rec_seq_;
	
	if (option.origin.module_url != "") {
		param.b_file_stream = true;
		param.file_url = option.origin.module_url;
	} else {
		param.b_file_stream = false; 
	}

	//framerate_ = option.origin.framerate;
 
	//usb传主码流： 1.直播原始流 2.只拼接不存原始流 3.存储模式为存nv的时候
	//usb传辅码流： 除以上情况外的其他情况
	if (option.origin.live_prefix != "" 
		|| (option.b_stiching && option.origin.storage_mode == INS_STORAGE_MODE_NONE)
		|| option.origin.storage_mode == INS_STORAGE_MODE_NV) {
		param.b_usb_stream = true;
	} else {
		param.b_usb_stream = false;
	}

	if (!param.b_usb_stream) {
		aux_param_ = std::make_shared<cam_video_param>();
		aux_param_->b_usb_stream = true;
		if (option.b_stiching) {	
			if (option.origin.height * 4 == option.origin.width * 3) {
				aux_param_->width 	= 1920;
				aux_param_->height 	= 1440;
			} else {
				aux_param_->width 	= 2048; //1792;
				aux_param_->height 	= 1152; //1008;
			}
			aux_param_->framerate 	= 30;
			aux_param_->bitrate 	= 25000;
		} else {
			if (option.origin.height * 4 == option.origin.width * 3) {	//4:3 
				aux_param_->width 	= 768;
				aux_param_->height 	= 576;
			} else {	//16:9
				aux_param_->width 	= 768;
				aux_param_->height 	= 432;
			}

			aux_param_->framerate 	= 30;
			aux_param_->bitrate 	= 5000;
		}
		
		if (option.origin.storage_mode == INS_STORAGE_MODE_AB && !option.b_stiching) {
			aux_param_->b_file_stream = true;
			aux_param_->file_url = option.origin.module_url;
		}
	}

	auto ret = camera_->set_all_video_param(param, aux_param_.get()); 
	RETURN_IF_NOT_OK(ret);

	ret = camera_->get_video_param(video_param_, aux_param_); //设置的参数可能和实际设置的不一样，要获取一次
	RETURN_IF_NOT_OK(ret);
	
	// rolling_shutter_time_ = param.rolling_shutter_time;
	// gyro_delay_time_ = param.gyro_delay_time;
	// gyro_orientation_ = param.gyro_orientation;
	// crop_flag_ = param.crop_flag;

	video_param_.gyro_delay_time = xml_config::get_gyro_delay_time(video_param_.width, video_param_.height, video_param_.framerate, video_param_.hdr);

	if (local_sink_) {	// 存陀螺仪数据
		auto sink = std::static_pointer_cast<gyro_sink>(local_sink_);
		video_buff_->add_gyro_sink(sink);
	}

	if (storage_aux
		|| option.origin.storage_mode == INS_STORAGE_MODE_NV
		|| option.origin.live_prefix != "") {
    	open_origin_stream(option);  //录像开始前打开原始流sink
	}

	/* 单镜头录像/合焦不用防抖 */
	if (option.b_stabilization && option.index == -1) 
		setup_stablz();
  
	ret = camera_->start_all_video_rec(m_queue); 
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

void video_mgr::open_origin_stream(const ins_video_option& option)
{
	ins_video_param video_param;
	video_param.mime = INS_H264_MIME;  //now origin all be h264
	if (aux_param_ && aux_param_->b_usb_stream) {	// 存辅码流
		video_param.width = aux_param_->width;
		video_param.height = aux_param_->height;
		video_param.bitrate = aux_param_->bitrate;
		video_param.fps = ins_util::to_real_fps(aux_param_->framerate).to_double();
	} else {	// 存主码流
		video_param.width = option.origin.width;
		video_param.height = option.origin.height;
		video_param.bitrate = option.origin.bitrate;
		video_param.fps = ins_util::to_real_fps(option.origin.framerate).to_double();
	}

	std::map<uint32_t, std::shared_ptr<sink_interface>> map_sink;
	for (uint32_t i = 0; i < INS_CAM_NUM; i++) {
		std::stringstream ss;

		if (option.origin.live_prefix != "") {
			ss << option.origin.live_prefix << "/origin" << camera_->get_pid(i); //origin live
		} else if (aux_param_) {
			ss << option.path << "/origin_" << camera_->get_pid(i) << "_lrv.mp4";
		} else {
			ss << option.path << "/origin_" << camera_->get_pid(i) << ".mp4";
		}
		
		auto sink = std::make_shared<stream_sink>(ss.str());
		sink->set_video(true);
		sink->set_fragment(b_frag_, true); 
		if (option.origin.live_prefix != "") {	// 原始流直播
			sink->set_live(true);
		} else {
			if (option.b_override) sink->set_override(true);
			if (!option.b_to_file) sink->set_just_last_frame(true);
		}
		
		if (option.index != -1 && camera_->get_pid(i) != option.index) { // 单镜头录像，其他镜头不存文件
			sink->set_just_last_frame(true);
		}
		
		//单镜头录像的时候存在对应镜头，非单镜头录像存在6号镜头
		if ((option.index != -1 && camera_->get_pid(i) == option.index) 
			|| (option.index == -1 && i == 0)) {
			sink->set_origin_key(true);
			audio_vs_sink(sink, AUDIO_INDEX_ORIGIN, true);
			if (option.origin.live_prefix == "") {	/* 非直播原始流，也就是文件流 */
				sink->set_gyro(true);
				auto S = std::static_pointer_cast<gyro_sink>(sink);
				video_buff_->add_gyro_sink(S);
				sink->set_gps(true);
				gps_mgr::get()->add_output(sink);
			}
		}
		map_sink.insert(std::make_pair(i, sink));
	}

	std::shared_ptr<all_cam_queue_i> origin_stream = std::make_shared<all_cam_origin_stream>(map_sink, video_param);
	video_buff_->add_output(VIDEO_BUFF_INDEX_ORIGIN, origin_stream);
}


void video_mgr::audio_vs_sink(std::shared_ptr<stream_sink>& sink, uint32_t index, bool is_origin)
{
	if (!audio_) return;

	if (is_origin) {
		sink->set_audio(true);
		audio_->add_output(index, sink);
	} else {
		if (audio_type_ != INS_AUDIO_Y_N) {
			sink->set_audio(true);
			if (audio_type_ == INS_AUDIO_Y_C) {
				audio_->add_output(index, sink, true); //内置mic的时候存在两路编码音频，第二路为非全景声
			} else {
				audio_->add_output(index, sink);
			}
		}
	}
}

void video_mgr::setup_stablz()
{
	if (!video_buff_) return;

	// int32_t delay = 0;
	// if (framerate_ >= 120)
	// {
	// 	delay = 17000; //us
	// }
	// else if (framerate_ >= 60)
	// {
	// 	delay = 35000;
	// }
	// else
	// {
	// 	delay = 78000;
	// }

	b_stablz_ = true;
	stablz_ = std::make_shared<stabilization>(video_param_.gyro_delay_time);
	auto S = std::static_pointer_cast<gyro_sink>(stablz_);
	video_buff_->add_gyro_sink(S);
}

void video_mgr::send_snd_state_msg(int type, std::string name, bool b_spatial)
{
	json_obj param_obj;
    param_obj.set_int("type", type);
	param_obj.set_bool("is_spatial", b_spatial);
	param_obj.set_string("dev_name", name);
	access_msg_center::send_msg(ACCESS_CMD_SND_STATE_, &param_obj); 
}


void video_mgr::print_option(const ins_video_option& option) const
{
	LOGINFO("origin option: type:%d %s-%d w:%d h:%d fps:%d bitrate:%d loc:%d module:%s live:%s override:%d tofile:%d stab:%d log:%d hdr:%d logo:%s index:%d", 
		option.type,
		option.origin.mime.c_str(),
		option.origin.bitdepth,
		option.origin.width, 
		option.origin.height, 
		option.origin.framerate, 
		option.origin.bitrate, 
		option.origin.storage_mode,
		option.origin.module_url.c_str(), 
		option.origin.live_prefix.c_str(), 
		option.b_override, 
		option.b_to_file, 
		option.b_stabilization, 
		option.origin.logmode, 
		option.origin.hdr, 
		option.logo_file.c_str(),
		option.index);

	if (option.b_stiching) {
		LOGINFO("stiching option: mime:%s width:%d height:%d framerate:%d bitrate:%d, mode:%d map_type:%d hdmi:%d url:%s %s",  
			option.stiching.mime.c_str(),
			option.stiching.width, 
			option.stiching.height, 
			option.stiching.framerate, 
			option.stiching.bitrate, 
			option.stiching.mode, 
			option.stiching.map_type, 
			option.stiching.hdmi_display,
			option.stiching.url.c_str(), 
			option.stiching.url_second.c_str());
	}

	if (option.b_audio) {
		LOGINFO("audio samplerate:%d bitrate:%d type:%d fanless:%d", 
			option.audio.samplerate,
			option.audio.bitrate,
			option.audio.type,
			option.audio.fanless);
	}

	if (option.auto_connect.enable) {
		LOGINFO("auto connect interval:%d ms count:%d", option.auto_connect.interval, option.auto_connect.count);
	}
}

// 存原片:
// 主Amba+辅Nv         usb辅
// 主Amba+辅Amba       usb辅
// 主Nv + 辅无         usb主

// 存原片+拼接/hdmi：
// 主Amba+辅无         usb辅
// 主Amba+辅无         usb辅
// 主Nv + 辅无         usb主

// 只拼接：
// 主无+辅无           usb主

// 直播原始流：
// 主无+辅无           usb主