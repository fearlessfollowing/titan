
#include "prj_file_mgr.h"
#include "tinyxml2.h"
#include <sstream>
#include "inslog.h"
#include "common.h"
#include "ins_util.h"
#include "xml_config.h"
#include "camera_info.h"
#include "access_msg.h"
#include "cam_data.h"
#include <mutex>
//#include "ins_offset_conv.h"

#define AUDIO_CAMM_FILE "origin_8.mp4"
#define AUDIO_CAMM_FILE_AUX "origin_8_lrv.mp4"

using namespace tinyxml2;

static std::mutex _mtx;

static std::string to_amba_real_fps_str(int framerate)
{
	double fps = ins_util::to_real_fps(framerate).to_double();
	char buff[64];
	sprintf(buff, "%0.4lf", fps); //保留4位小数
	return buff;
}

static void add_accel_offset(XMLDocument& xml_doc, XMLElement* e_gyro)
{
	double x = 0, y = 0, z = 1;//default value
	xml_config::get_accel_offset(x, y, z);
	auto e_calibration = xml_doc.NewElement("calibration");  
	if (!e_calibration) {
		LOGERR("new element acc_offset fail");
		return;
	}

	auto e_x = xml_doc.NewElement("gravity_x");
	auto e_y = xml_doc.NewElement("gravity_y");
	auto e_z = xml_doc.NewElement("gravity_z");

	if (e_x) e_x->SetText(x);
	if (e_y) e_y->SetText(y);
	if (e_z) e_z->SetText(z);

	e_calibration->LinkEndChild(e_x);
	e_calibration->LinkEndChild(e_y);
	e_calibration->LinkEndChild(e_z);

	e_gyro->LinkEndChild(e_calibration);
}

static void add_depth_map(XMLDocument& xml_doc, XMLElement* e_root, std::string path)
{
	//depth map
	if (!access(INS_DEPTH_MAP_FILE, 0)) {
		std::string cmd;
		cmd += "cp " ;
		cmd += INS_DEPTH_MAP_FILE;
		cmd += " " + path + "/" + INS_DEPTH_MAP_FILE_NAME;
		system(cmd.c_str());
		auto e_depth_map = xml_doc.NewElement("depth_map");
		auto e_file = xml_doc.NewElement("file");
		e_file->SetText(INS_DEPTH_MAP_FILE_NAME);
		e_depth_map->LinkEndChild(e_file);
		e_root->LinkEndChild(e_depth_map);
	} else {
		LOGINFO("no depth map info");
	}
}

//offset
static void add_offset(XMLDocument& xml_doc, XMLElement* e_root)
{
	std::string pano_4_3;
	//std::string pano_16_9;
	xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3, pano_4_3);
	//xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9, pano_16_9);
	//ns_offset_conv::conv(pano_4_3, pano_16_9, INS_CROP_FLAG_16_9);

	std::string factory_offset = xml_config::get_factory_offset();
	//std::string factory_16_9;
	//ins_offset_conv::conv(factory_offset, factory_16_9, INS_CROP_FLAG_16_9);

	auto e_offset = xml_doc.NewElement("offset");

	auto e_pano_4_3 = xml_doc.NewElement("pano_4_3");
	e_pano_4_3->SetText(pano_4_3.c_str());
	//e_pano_4_3->SetAttribute("factory_setting", false);
	e_offset->LinkEndChild(e_pano_4_3);

	auto e_pano_16_9 = xml_doc.NewElement("pano_16_9");
	e_pano_16_9->SetText(pano_4_3.c_str());
	//e_pano_16_9->SetAttribute("factory_setting", false);
	e_offset->LinkEndChild(e_pano_16_9);

	e_root->LinkEndChild(e_offset);

	auto e_offset_factory = xml_doc.NewElement("origin_offset");

	auto e_factory_4_3 = xml_doc.NewElement("pano_4_3");
	e_factory_4_3->SetText(factory_offset.c_str());
	//e_factory_4_3->SetAttribute("factory_setting", true);
	e_offset_factory->LinkEndChild(e_factory_4_3);

	auto e_factory_16_9 = xml_doc.NewElement("pano_16_9");
	e_factory_16_9->SetText(factory_offset.c_str());
	//e_factory_16_9->SetAttribute("factory_setting", true);
	e_offset_factory->LinkEndChild(e_factory_16_9);

	e_root->LinkEndChild(e_offset_factory);

	// std::string left_3d;
	// xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT, left_3d);
	// if (left_3d == "") left_3d = INS_DEFAULT_OFFSET_3D_LEFT;
	// std::string right_3d;
	// xml_config::get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT, right_3d);
	// if (right_3d == "") right_3d = INS_DEFAULT_OFFSET_3D_RIGHT;
	
	// auto e_3d_left = xml_doc.NewElement("left_3d");
	// e_3d_left->SetText(left_3d.c_str());
	// e_offset->LinkEndChild(e_3d_left);
	// auto e_3d_right = xml_doc.NewElement("right_3d");
	// e_3d_right->SetText(right_3d.c_str());
	// e_offset->LinkEndChild(e_3d_right);
}

int32_t prj_file_mgr::add_first_frame_ts(std::string path, int64_t ts)
{
	std::lock_guard<std::mutex> lock(_mtx);

	std::string url = path + "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* e_root = xml_doc.RootElement();
	if (!e_root) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}

	auto e_fft = xml_doc.NewElement("first_frame_timestamp");
	if (!e_fft) {
		LOGERR("new first_frame_timestamp fail");
		return INS_ERR;
	}

	std::stringstream ss;
	ss << ts;
	e_fft->SetText(ss.str().c_str());

	e_root->LinkEndChild(e_fft);

	xml_doc.SaveFile(url.c_str());

	return INS_OK;
}



int32_t prj_file_mgr::add_first_frame_ts(std::string path, std::map<uint32_t, int64_t>& first_frame_ts)
{
	std::lock_guard<std::mutex> lock(_mtx);

	std::string url = path + "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* e_root = xml_doc.RootElement();
	if (!e_root) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}

	auto e_gyro = e_root->FirstChildElement("gyro");  
	if (!e_gyro) {
		LOGERR("xml: cann't find element:gyro");
		return INS_ERR;
	}

	auto sts_group = xml_doc.NewElement("sts_group");
	for (uint32_t i = 1; i <= INS_CAM_NUM; i++) {
		auto it = first_frame_ts.find(i);
		assert(it != first_frame_ts.end());
		auto e_start_ts = xml_doc.NewElement("start_ts");
		e_start_ts->SetText((int32_t)it->second);
		LOGINFO("first_frame_ts pid:%d ts:%lld", it->first, it->second);
		sts_group->LinkEndChild(e_start_ts);
	}

	e_gyro->LinkEndChild(sts_group);

	xml_doc.SaveFile(url.c_str());

	LOGINFO("projet file add first frame timestamp");

	return INS_OK;	
}

int32_t prj_file_mgr::add_audio_info(const std::string& path, std::string dev_name, bool b_spatial, int32_t storage_loc, bool single)
{
	std::lock_guard<std::mutex> lock(_mtx);

	if (storage_loc == INS_STORAGE_MODE_NONE) return INS_ERR;

	std::string url = path + "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* e_root = xml_doc.RootElement();
	if (!e_root) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}
	
	auto e_audio = xml_doc.NewElement("audio");  
	if (!e_audio) {
		LOGERR("xml: cann't find element:audio");
		return INS_ERR;
	}

	if (storage_loc == INS_STORAGE_MODE_AB) {
		e_audio->SetAttribute("storage_loc", 1);
	} else if (storage_loc == INS_STORAGE_MODE_NV) {
		e_audio->SetAttribute("storage_loc", 0);
		e_audio->SetAttribute("file", AUDIO_CAMM_FILE);
	} else if (storage_loc == INS_STORAGE_MODE_AB_NV) {
		e_audio->SetAttribute("storage_loc", 0);
		if (single) {
			e_audio->SetAttribute("file", "audio.mp4");
		} else {
			e_audio->SetAttribute("file", AUDIO_CAMM_FILE_AUX);
		}
	}

	e_audio->SetAttribute("audio_device", dev_name.c_str());

	if (b_spatial) {
		e_audio->SetAttribute("spatial_audio", "true");
	} else {
		e_audio->SetAttribute("spatial_audio", "false");
	}

	e_root->LinkEndChild(e_audio);

	xml_doc.SaveFile(url.c_str());

	LOGINFO("project file add audio metadata");

	return INS_OK;
}


int32_t prj_file_mgr::add_gyro_gps(const std::string& path,int32_t storage_loc, double rs_time, int32_t delay_time, int32_t orientation, bool single)
{
	std::lock_guard<std::mutex> lock(_mtx);

	if (storage_loc == INS_STORAGE_MODE_NONE) return INS_ERR;

	std::string url = path + "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* e_root = xml_doc.RootElement();
	if (!e_root) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}

	auto e_gyro = e_root->FirstChildElement("gyro"); 
	if (!e_gyro) {
		LOGERR("xml: cann't find element:gyro");
		return INS_ERR;
	}
	e_gyro->SetAttribute("rolling_shutter_time_us", rs_time);
	e_gyro->SetAttribute("delay_time_us", delay_time);
	e_gyro->SetAttribute("orientation", orientation);

	add_accel_offset(xml_doc, e_gyro);

	auto e_gps = xml_doc.NewElement("gps");  
	if (!e_gps) {
		LOGERR("xml: cann't find element:gps");
		return INS_ERR;
	}

	if (storage_loc == INS_STORAGE_MODE_AB) {
		e_gyro->SetAttribute("storage_loc", 1);
		e_gps->SetAttribute("storage_loc", 1);
	} else if (storage_loc == INS_STORAGE_MODE_NV) {
		e_gyro->SetAttribute("storage_loc", 0);
		e_gyro->SetAttribute("file", AUDIO_CAMM_FILE);

		e_gps->SetAttribute("storage_loc", 0);
		e_gps->SetAttribute("file", AUDIO_CAMM_FILE);
	} else if (storage_loc == INS_STORAGE_MODE_AB_NV) {
		e_gyro->SetAttribute("storage_loc", 0);
		e_gps->SetAttribute("storage_loc", 0);
		if (single) {
			e_gyro->SetAttribute("file", "audio.mp4");
			e_gps->SetAttribute("file", "audio.mp4");
		} else {
			e_gyro->SetAttribute("file", AUDIO_CAMM_FILE_AUX);
			e_gps->SetAttribute("file", AUDIO_CAMM_FILE_AUX);
		}
	}

	e_root->LinkEndChild(e_gyro);
	e_root->LinkEndChild(e_gps);

	xml_doc.SaveFile(url.c_str());

	LOGINFO("project file add gyro gps");

	return INS_OK;
}

int prj_file_mgr::add_origin_frag_file(const std::string& path, int sequence)
{
	std::lock_guard<std::mutex> lock(_mtx);

	std::string url = path + "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* element = xml_doc.RootElement();
	if (!element) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}
	
	element = element->FirstChildElement("origin");  
	if (!element) {
		LOGERR("xml: cann't find element:origin");
		return INS_ERR;
	}

	auto filegroup_node = xml_doc.NewElement("filegroup");  
	
	for (int i = INS_CAM_NUM; i > 0; i--) {
		char str[64] = {0};
		sprintf(str, "origin_%d_%03d.mp4", i, sequence);
    	auto file_node = xml_doc.NewElement("file"); 
		file_node->SetText(str);
		filegroup_node->LinkEndChild(file_node); 
	}

	element->LinkEndChild(filegroup_node);
	xml_doc.SaveFile(url.c_str());

	LOGINFO("project file add fragment mp4");

	return INS_OK;
}

int32_t prj_file_mgr::updata_video_info(std::string path, cam_video_param* param)
{
	std::lock_guard<std::mutex> lock(_mtx);

	std::string url = path +  "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* e_root = xml_doc.RootElement();
	if (!e_root) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}

	auto e_origin = e_root->FirstChildElement("origin");
	if (!e_origin) {	
		LOGERR("video prj no origin");
		return INS_ERR;
	}

	auto e_metadata = e_origin->FirstChildElement("metadata"); 
	if (!e_metadata) {	
		LOGERR("video prj no metadata");
		return INS_ERR;
	}

	auto fps = to_amba_real_fps_str(param->framerate);
	e_metadata->SetAttribute("type", "video");
	e_metadata->SetAttribute("mime", param->mime.c_str());
	e_metadata->SetAttribute("bitDepth", param->bitdepth);
	e_metadata->SetAttribute("width", param->width);
	e_metadata->SetAttribute("height", param->height);
	e_metadata->SetAttribute("crop_flag", param->crop_flag);
	e_metadata->SetAttribute("framerate", fps.c_str());

	xml_doc.SaveFile(url.c_str());

	LOGINFO("project file update video param");

	return INS_OK;
}

int32_t prj_file_mgr::add_aux_file_info(std::string path, const std::shared_ptr<cam_video_param> aux_info,int32_t storage_loc)
{
	std::lock_guard<std::mutex> lock(_mtx);

	std::string url = path +  "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* e_root = xml_doc.RootElement();
	if (!e_root) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}

	auto e_aux = xml_doc.NewElement("auxorigin");

	auto fps = to_amba_real_fps_str(aux_info->framerate);
	auto e_metadata = xml_doc.NewElement("metadata");
	e_metadata->SetAttribute("type", "video");
	e_metadata->SetAttribute("mime", aux_info->mime.c_str());
	e_metadata->SetAttribute("bitDepth", aux_info->bitdepth);
	e_metadata->SetAttribute("width", aux_info->width);
	e_metadata->SetAttribute("height", aux_info->height);
	e_metadata->SetAttribute("logMode", aux_info->logmode);
	e_metadata->SetAttribute("hdr", aux_info->hdr);
	e_metadata->SetAttribute("crop_flag", aux_info->crop_flag);
	e_metadata->SetAttribute("framerate", fps.c_str());
	std::stringstream ss;
	ss << aux_info->bitrate << "K";
	e_metadata->SetAttribute("bitrate", ss.str().c_str());
	if (storage_loc == INS_STORAGE_MODE_AB) {
		e_metadata->SetAttribute("storage_loc", 1);
	} else {
		e_metadata->SetAttribute("storage_loc", 0);
	}

	e_aux->LinkEndChild(e_metadata);

	auto e_file_group = xml_doc.NewElement("filegroup");
	for (int i = INS_CAM_NUM; i > 0; i--) {
		auto e_file = xml_doc.NewElement("file");
		std::stringstream ss;
		ss << "origin_" << i << "_lrv.mp4";
		e_file->SetText(ss.str().c_str());
		e_file_group->LinkEndChild(e_file);
	}
	e_aux->LinkEndChild(e_file_group);

	e_root->LinkEndChild(e_aux);

	xml_doc.SaveFile(url.c_str());

	LOGINFO("project file add aux stream info");

	return INS_OK;
}

int32_t prj_file_mgr::add_preview_info(std::string path, uint32_t bitrate, uint32_t framerate)
{
	std::lock_guard<std::mutex> lock(_mtx);

	std::string url = path +  "/" + INS_PROJECT_FILE;

	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str())) {
		LOGERR("xml file:%s load fail", url.c_str());
		return INS_ERR; 
	}

	XMLElement* e_root = xml_doc.RootElement();
	if (!e_root) {
		LOGERR("xml: cann't find root element");
		return INS_ERR;
	}

	bool stitching_exist = true;
	auto e_stitching = e_root->FirstChildElement("stitching");  
	if (!e_stitching) {
		e_stitching = xml_doc.NewElement("stitching");
		stitching_exist = false;
	}

	auto e_preview = xml_doc.NewElement("preview");
	e_preview->SetAttribute("type", "video");
	e_preview->SetAttribute("mime", INS_H264_MIME);
	e_preview->SetAttribute("src", INS_PREVIEW_FILE);
	e_preview->SetAttribute("width", INS_PREVIEW_WIDTH);
	e_preview->SetAttribute("height", INS_PREVIEW_HEIGHT);
	auto fps = to_amba_real_fps_str(framerate);
	e_preview->SetAttribute("framerate", fps.c_str());
	std::stringstream ss;
	ss << bitrate << "K";
	e_preview->SetAttribute("bitrate", ss.str().c_str());
	e_preview->SetAttribute("map", "flat");
	e_stitching->LinkEndChild(e_preview);

	if (!stitching_exist) e_root->LinkEndChild(e_stitching);

	xml_doc.SaveFile(url.c_str());

	return INS_OK;
}


int prj_file_mgr::create_pic_prj(const ins_picture_option& opt, std::string path)
{
	std::lock_guard<std::mutex> lock(_mtx);

	XMLDocument xml_doc;
	auto dec = xml_doc.NewDeclaration();
	auto e_root = xml_doc.NewElement("project");
	xml_doc.LinkEndChild(dec);
	xml_doc.LinkEndChild(e_root);

	//offset
	add_offset(xml_doc, e_root);

	auto e_info = xml_doc.NewElement("camera_info");
	e_info->SetAttribute("make", INS_MAKE);
	e_info->SetAttribute("model", INS_MODEL);
	e_info->SetAttribute("sn", camera_info::get_sn().c_str()); //sn
	e_root->LinkEndChild(e_info);

	//version
	auto e_version = xml_doc.NewElement("version");
	e_version->SetAttribute("firmware", camera_info::get_fw_ver().c_str());
	e_version->SetAttribute("camerad", camera_info::get_ver().c_str());
	e_version->SetAttribute("module", camera_info::get_m_ver().c_str());
	e_root->LinkEndChild(e_version);

	//origin
	if (opt.origin.storage_mode != INS_STORAGE_MODE_NONE) {
		std::string type;
		std::string file_patten;
		auto e_origin = xml_doc.NewElement("origin");
		auto e_metadata = xml_doc.NewElement("metadata");
		auto e_metadata_aux = xml_doc.NewElement("metadata");
		
		if (opt.origin.mime == INS_JPEG_MIME) {
			e_metadata->SetAttribute("mime", INS_JPEG_MIME);
		} else {
			e_metadata->SetAttribute("mime", INS_RAW_MIME);
		}
		e_metadata_aux->SetAttribute("mime", INS_JPEG_MIME);

		e_metadata->SetAttribute("width", opt.origin.width);
		e_metadata->SetAttribute("height", opt.origin.height);
		e_metadata_aux->SetAttribute("width", opt.origin.width);
		e_metadata_aux->SetAttribute("height", opt.origin.height);
		
		if (opt.burst.enable) {
			type = "burst";
			file_patten = "origin_%d_";
			e_metadata->SetAttribute("count", opt.burst.count);
			e_metadata_aux->SetAttribute("count", opt.burst.count);
		} else if (opt.hdr.enable) {
			type = "hdr";
			file_patten = "origin_%d_";
			e_metadata->SetAttribute("count", opt.hdr.count);
			e_metadata_aux->SetAttribute("count", opt.hdr.count);
		} else if (opt.bracket.enable) {
			type = "bracket";
			file_patten = "origin_%d_";
			e_metadata->SetAttribute("count", opt.bracket.count);
			e_metadata_aux->SetAttribute("count", opt.bracket.count);
		} else if (opt.timelapse.enable) {
			type = "timelapse";
			file_patten = "origin_%d_";
			e_metadata->SetAttribute("interval", opt.timelapse.interval);
			e_metadata->SetAttribute("count", 100000);
			e_metadata_aux->SetAttribute("interval", opt.timelapse.interval);
			e_metadata_aux->SetAttribute("count", 100000);
		} else {
			type = "photo";
			file_patten = "origin_";
		}
		e_metadata->SetAttribute("type", type.c_str());
		e_metadata_aux->SetAttribute("type", type.c_str());
		
		if (opt.origin.storage_mode == INS_STORAGE_MODE_NV) {
			e_metadata->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 0);
			e_metadata_aux->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 0);
		} else if (opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV) {
			if (opt.origin.mime == INS_JPEG_MIME)  {
				e_metadata->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 0);
			} else if (opt.origin.mime == INS_RAW_MIME) {
				e_metadata->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 1);
			} else {
				e_metadata->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 1); //raw
				e_metadata_aux->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 0); //jpeg
			}
		} else {
			e_metadata->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 1);
			e_metadata_aux->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, 1);
		}
		e_origin->LinkEndChild(e_metadata);
		
		for (int i = INS_CAM_NUM; i > 0; i--) {
			auto e_file = xml_doc.NewElement("file");
			std::stringstream ss;
			if (opt.origin.mime == INS_JPEG_MIME) {
				ss << file_patten << i << INS_JPEG_EXT;
			} else {
				ss << file_patten << i << INS_RAW_EXT;
			}
			
			e_file->SetText(ss.str().c_str());
			e_origin->LinkEndChild(e_file);
		}

		e_root->LinkEndChild(e_origin);

		if (opt.origin.mime == INS_RAW_JPEG_MIME) {
			auto e_aux_origin = xml_doc.NewElement("auxorigin");
			e_aux_origin->LinkEndChild(e_metadata_aux);

			for (int i = INS_CAM_NUM; i > 0; i--) {
				auto e_file = xml_doc.NewElement("file");
				std::stringstream ss;
				ss << file_patten << i << INS_JPEG_EXT;
				e_file->SetText(ss.str().c_str());
				e_aux_origin->LinkEndChild(e_file);
			}
			e_root->LinkEndChild(e_aux_origin);
		}
	}

	//thumbnail
	if (!opt.timelapse.enable && opt.origin.mime != INS_RAW_MIME) {	/* 只要非timelapse非raw都有缩略图 */
		auto e_thumbnail = xml_doc.NewElement("thumbnail");
		e_thumbnail->SetAttribute("src", "thumbnail.jpg");
		e_thumbnail->SetAttribute("mime",INS_JPEG_MIME);
		if (opt.stiching.map_type == INS_MAP_CUBE && opt.b_stiching) {	/* 非实时拼接的时候，缩略图都是flat的 */
			e_thumbnail->SetAttribute("width", INS_THUMBNAIL_WIDTH_CUBE);
			e_thumbnail->SetAttribute("height", INS_THUMBNAIL_HEIGHT_CUBE);
		} else {
			e_thumbnail->SetAttribute("width", INS_THUMBNAIL_WIDTH);
			e_thumbnail->SetAttribute("height", INS_THUMBNAIL_HEIGHT);
		}
		e_root->LinkEndChild(e_thumbnail);
	}

	//stitching
	if (opt.b_stiching) {
		auto e_stitching = xml_doc.NewElement("stitching");
		auto e_file_stitching = xml_doc.NewElement("file_stitching");
		e_file_stitching->SetAttribute("type", "photo");
		e_file_stitching->SetAttribute("mime", opt.stiching.mime.c_str());
		std::string file_name = ins_util::create_name_by_mode(opt.stiching.mode) + ".jpg";
		e_file_stitching->SetAttribute("src", file_name.c_str());
		e_file_stitching->SetAttribute("width", opt.stiching.width);
		e_file_stitching->SetAttribute("height", opt.stiching.height);

		if (opt.stiching.map_type == INS_MAP_CUBE) {
			e_file_stitching->SetAttribute("map", "cube");
		} else {
			e_file_stitching->SetAttribute("map", "flat");
		}
		e_stitching->LinkEndChild(e_file_stitching);
		e_root->LinkEndChild(e_stitching);
	}

	if (opt.origin.storage_mode == INS_STORAGE_MODE_NV || opt.origin.storage_mode == INS_STORAGE_MODE_AB_NV) {
		auto e_gyro = xml_doc.NewElement("gyro");
		e_gyro->SetAttribute("version", INS_GYRO_VERSION);
		e_gyro->SetAttribute("storage_loc", 0);
		e_gyro->SetAttribute("file", "gyro.mp4");
		e_gyro->SetAttribute("orientation", camera_info::get_gyro_orientation());
		
		e_root->LinkEndChild(e_gyro);
		add_accel_offset(xml_doc, e_gyro);
	}

	//depth map
	add_depth_map(xml_doc, e_root, path);

	std::string url = path + "/" + INS_PROJECT_FILE;
	xml_doc.SaveFile(url.c_str());

	return INS_OK;
}


int prj_file_mgr::create_vid_prj(const ins_video_option& opt, std::string path)
{
	std::lock_guard<std::mutex> lock(_mtx);

	XMLDocument xml_doc;
	auto dec = xml_doc.NewDeclaration();
	auto e_root = xml_doc.NewElement("project");
	xml_doc.LinkEndChild(dec);
	xml_doc.LinkEndChild(e_root);

	//offset
	add_offset(xml_doc, e_root);

	auto e_info = xml_doc.NewElement("camera_info");
	e_info->SetAttribute("make", INS_MAKE);
	e_info->SetAttribute("model", INS_MODEL);
	e_info->SetAttribute("sn", camera_info::get_sn().c_str()); //sn
	e_root->LinkEndChild(e_info);

	//version
	auto e_version = xml_doc.NewElement("version");
	e_version->SetAttribute("firmware", camera_info::get_fw_ver().c_str());
	e_version->SetAttribute("camerad", camera_info::get_ver().c_str());
	e_version->SetAttribute("module", camera_info::get_m_ver().c_str());
	e_root->LinkEndChild(e_version);
	
	//origin
	if (opt.origin.storage_mode != INS_STORAGE_MODE_NONE) {
		int32_t storage_loc = (opt.origin.storage_mode == INS_STORAGE_MODE_NV)?0:1;
		std::string type;
		auto e_origin = xml_doc.NewElement("origin");

		//metadata
		auto fps = to_amba_real_fps_str(opt.origin.framerate);
		auto e_metadata = xml_doc.NewElement("metadata");
		e_metadata->SetAttribute("type", "video");
		e_metadata->SetAttribute("mime", opt.origin.mime.c_str());
		e_metadata->SetAttribute("bitDepth", opt.origin.bitdepth);
		e_metadata->SetAttribute("width", opt.origin.width);
		e_metadata->SetAttribute("height", opt.origin.height);
		e_metadata->SetAttribute("framerate", fps.c_str());
		std::stringstream ss;
		ss << opt.origin.bitrate << "K";
		e_metadata->SetAttribute("bitrate", ss.str().c_str());
		e_metadata->SetAttribute("logMode", opt.origin.logmode);
		e_metadata->SetAttribute("hdr", opt.origin.hdr);
		e_metadata->SetAttribute(ACCESS_MSG_OPT_STORAGE_LOC, storage_loc);

		e_origin->LinkEndChild(e_metadata);

		auto e_file_group = xml_doc.NewElement("filegroup");
		for (int i = INS_CAM_NUM; i > 0; i--) {
			auto e_file = xml_doc.NewElement("file");
			std::stringstream ss;
			ss << "origin_" << i << ".mp4";
			e_file->SetText(ss.str().c_str());
			e_file_group->LinkEndChild(e_file);
		}
		e_origin->LinkEndChild(e_file_group);
		e_root->LinkEndChild(e_origin);

		auto e_gyro = xml_doc.NewElement("gyro");  
		e_gyro->SetAttribute("version", INS_GYRO_VERSION);
		std::vector<double> v_quat;
		xml_config::get_gyro_rotation(v_quat);
		if (v_quat.size() == 4) {
			auto e_rotation = xml_doc.NewElement("imu_rotation");
			e_rotation->SetAttribute("x", v_quat[0]);
			e_rotation->SetAttribute("y", v_quat[1]);
			e_rotation->SetAttribute("z", v_quat[2]);
			e_rotation->SetAttribute("w", v_quat[3]);
			e_gyro->LinkEndChild(e_rotation);
		} else {
			LOGERR("cann't get imu rotation");
		}
		e_root->LinkEndChild(e_gyro);
	}

	//stitching
	if ((opt.stiching.url != "" &&  (opt.stiching.url.find(".mp4", 0) != std::string::npos)) 
		|| (opt.stiching.url_second != "" &&  (opt.stiching.url_second.find(".mp4", 0) != std::string::npos))) {
		auto e_stitching = xml_doc.NewElement("stitching");
		auto e_file_stitching = xml_doc.NewElement("file_stitching");
		e_file_stitching->SetAttribute("type", "video");
		e_file_stitching->SetAttribute("mime", opt.stiching.mime.c_str());
		std::string file_name = ins_util::create_name_by_mode(opt.stiching.mode) + ".mp4";
		e_file_stitching->SetAttribute("src", file_name.c_str());
		e_file_stitching->SetAttribute("width", opt.stiching.width);
		e_file_stitching->SetAttribute("height", opt.stiching.height);
		auto fps = to_amba_real_fps_str(opt.stiching.framerate);
		e_file_stitching->SetAttribute("framerate", fps.c_str());
		std::stringstream ss;
		ss << opt.stiching.bitrate << "K";
		e_file_stitching->SetAttribute("bitrate", ss.str().c_str());
		if (opt.stiching.map_type == INS_MAP_CUBE) {
			e_file_stitching->SetAttribute("map", "cube");
		} else {
			e_file_stitching->SetAttribute("map", "flat");
		}
		e_stitching->LinkEndChild(e_file_stitching);

		e_root->LinkEndChild(e_stitching);
	}

	//depth map	
	add_depth_map(xml_doc, e_root, path);

	std::string url = path + "/" + INS_PROJECT_FILE;
	xml_doc.SaveFile(url.c_str());

	return INS_OK;
}
