#include "image_mgr.h"
#include "common.h"
#include "inslog.h"
#include "cam_manager.h"
#include "access_msg_center.h"
#include "ins_blender.h"
#include "offset_wrap.h"
#include "tjpegdec.h"
#include "img_enc_wrap.h"
#include "cam_img_repo.h"
#include "jpeg_muxer.h"
#include "xml_config.h"
#include "ins_clock.h"
#include "ins_util.h"
#include "usb_sink.h"
#include "gps_mgr.h"
#include "stream_sink.h"
#include "all_lens_hdr.h"
#include "stabilization.h"


image_mgr::~image_mgr()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
    gps_mgr::get()->del_all_output();
    LOGINFO("image mgr destroy");
}



/**********************************************************************************************
 * 函数名称: start
 * 功能秒数: 启动拍照线程
 * 参   数: 
 *      camera - 相机管理器指针
 *      option - 拍照参数option
 *      b_calibration - 是否为拼接校准
 * 返 回 值: 无
 *********************************************************************************************/
int32_t image_mgr::start(cam_manager* camera, const ins_picture_option& option, bool b_calibration)
{
    print_option(option);
    camera_ = camera;
    option_ = option;
    b_calibration_ = b_calibration;

    /* 启动拍照线程 */
    th_ = std::thread(&image_mgr::task, this);
    return INS_OK;
}



/**********************************************************************************************
 * 函数名称: task
 * 功能秒数: 拍照线程
 * 参   数: 
 * 返 回 值: 无
 *********************************************************************************************/
void image_mgr::task()
{
    if (option_.delay > 0) {    /* 如果需要延时 */
		LOGINFO("delay:%ds to take picture", option_.delay);
		usleep(option_.delay*1000*1000);
	}

    int32_t cnt = 0;
    int32_t cnt_cur = 0;
    int32_t ret = INS_OK;

    if (option_.origin.storage_mode == INS_STORAGE_MODE_AB) open_usb_sink(option_.prj_path);

	auto future = open_camera_capture(cnt);     /* 打开模组,设置拍照参数,启动拍照 */
    bool future_got = false;

	while (!quit_) {

		if (cnt_cur >= cnt) 
			break;	/* 当前完成的组数大于等于需求组数,退出 */

        if (!future_got) {
            if (std::future_status::ready == future.wait_for(0s)) {
                future_got = true;
                ret = future.get();
                BREAK_IF_NOT_OK(ret);
            }
        }

		/*
		 * std::map<uint32_t, std::shared_ptr<ins_frame>>
		 */
        auto v_frame = img_repo_->dequeue_frame();
        if (v_frame.empty()) {
            usleep(20 * 1000);
            continue;
        }

        cnt_cur++;

        if (b_calibration_) {   /* calibration只拍一张 */
            calibration(v_frame);
            break;  
        } else {
            process(v_frame);   /* 出错也继续处理下一组 */
        }
	}

    if (ret == INS_OK && option_.b_stiching 
		&& (option_.hdr.enable || option_.bracket.enable)) {
        ret = hdr_compose();
    }

    LOGINFO("image loop exit");

    if (!future_got) 
        ret = future.get();

    if (!quit_) 
        send_pic_finish_msg(ret, option_.path);	/* 发送拍照完成消息(内部消息) */

    LOGINFO("image task exit");
}


int image_mgr::process(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame)
{
	int ret = INS_OK;

    if (!m_frame.begin()->second->metadata.raw) jpeg_seq_++;

    if (jpeg_seq_ == 1) {
        metadata_ = m_frame.begin()->second->metadata;
    }

	if (option_.origin.storage_mode == INS_STORAGE_MODE_NV || option_.origin.storage_mode == INS_STORAGE_MODE_AB_NV) {
		ret = save_origin(m_frame);
		RETURN_IF_NOT_OK(ret);
	}

	/*
	 * 实时拼接(raw/burst不支持，bracket/hdr不在此处理)
	 * compose: 解码一组照片 -> 拼接 -> 生成jpeg拼接照片和缩略图照片
	 */
	if (option_.b_stiching
        && !option_.hdr.enable
        && !option_.bracket.enable
        && !option_.burst.enable 
        && !m_frame.begin()->second->metadata.raw) {
        ret = compose(m_frame, option_); 
        RETURN_IF_NOT_OK(ret);
    }
		
    //非实时拼接也要拼接一张缩略图
    else if (!option_.b_stiching
        && !m_frame.begin()->second->metadata.raw
        && jpeg_seq_ == 1 		//burst/bracket/hdr有多组，用第一组生成缩略图
        && option_.index == -1) //单镜头拍照不生成缩略图
    {
        ins_picture_option option; 
        option.stiching.mode 		= INS_MODE_PANORAMA; 
        option.stiching.map_type 	= INS_MAP_FLAT;
        option.stiching.algorithm 	= INS_ALGORITHM_NORMAL;
        option.stiching.width 		= INS_THUMBNAIL_WIDTH;
        option.stiching.height 		= INS_THUMBNAIL_HEIGHT;
        option.logo_file 			= option_.logo_file;
        ret = compose(m_frame, option);
        RETURN_IF_NOT_OK(ret);
    }

	return ret;
}



/**********************************************************************************************
 * 函数名称: task
 * 功能秒数: 拍照线程
 * 参   数: 
 * 返 回 值: 无
 *********************************************************************************************/
std::future<int32_t> image_mgr::open_camera_capture(int32_t& cnt)
{
	cam_photo_param param;
	param.width = option_.origin.width;
	param.height = option_.origin.height;
    param.mime = option_.origin.mime;
    param.file_url = option_.origin.module_url;		/* 文件名 */

    /* 
     * 需要模组传流的情况：
     *	1.原始流存在nvidia 
     *	2.需要实时拼接 
     *	3.拼接校准
     */
    if (option_.origin.storage_mode == INS_STORAGE_MODE_NV) {   /* jpg/raw都存nv */
        param.b_usb_raw 	= true;
        param.b_usb_jpeg 	= true;
        param.b_file_raw 	= false;
        param.b_file_jpeg 	= false;
    } else if (option_.origin.storage_mode == INS_STORAGE_MODE_AB_NV) {   /* jpg存nv,raw存amba */
        param.b_usb_raw 	= false;
        param.b_usb_jpeg 	= true;
        param.b_file_raw 	= true;
        param.b_file_jpeg 	= false;
    } else {		
        param.b_usb_raw 	= false;
        param.b_usb_jpeg 	= false;
        param.b_file_raw 	= true;
        param.b_file_jpeg 	= true;
    }

    if (option_.b_stiching || b_calibration_) {
        param.b_usb_jpeg = true;
    }
	
	if (option_.burst.enable) {				/* 拍照类型为burst */
		param.type	= INS_PIC_TYPE_BURST;
		param.count	= option_.burst.count;
		cnt 		= option_.burst.count;
	} else if (option_.hdr.enable) {		/* 拍照类型为hdr */
		param.type 		= INS_PIC_TYPE_HDR;
		param.count 	= option_.hdr.count;
		param.min_ev 	= option_.hdr.min_ev;
		param.max_ev 	= option_.hdr.max_ev;
		cnt = option_.hdr.count;
	} else if (option_.bracket.enable) {	/* 拍照类型为bracket */
		param.type 		= INS_PIC_TYPE_BRACKET;
		param.count 	= option_.bracket.count;
		param.min_ev 	= option_.bracket.min_ev;
		param.max_ev 	= option_.bracket.max_ev;
		cnt 			= option_.bracket.count;
	} else {
        param.type = "jpeg";
		cnt = 1;
	}

    if (!param.b_usb_jpeg && !param.b_usb_raw) 
        cnt = 0; // 代表不需要去接收流
        
    if (param.b_usb_jpeg && param.b_usb_raw && param.mime == INS_RAW_JPEG_MIME) 
        cnt *= 2;

    img_repo_ = std::make_shared<cam_img_repo>();

    if (option_.origin.storage_mode == INS_STORAGE_MODE_NV || option_.origin.storage_mode == INS_STORAGE_MODE_AB_NV) {
        std::string gyro_name = option_.path + "/gyro.mp4";
        auto sink = std::make_shared<stream_sink>(gyro_name);
        sink->set_gyro(true, false);
        auto S = std::static_pointer_cast<gyro_sink>(sink);
        img_repo_->add_gyro_sink(S);
        sink->start(0); 
    }

    if (option_.b_stabilization && option_.b_stiching) {
        stablz_ = std::make_shared<stabilization>();
	    auto S = std::static_pointer_cast<gyro_sink>(stablz_);
	    img_repo_->add_gyro_sink(S);
    }

	/* 把img_repo_传递给所有采集数据图片数据的线程 */
    return camera_->start_all_still_capture(param, img_repo_);
}



int32_t image_mgr::compose(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame, const ins_picture_option& option)
{   
    /*
	 * 1.使用JPEG解码器,将每个模组的照片进行解码,解码后丢入v_dec_img容器中
	 * 2.调用do_compose将解码后的数据进行合并并编码
	 * Thinking:
	 *	- jpeg解码后是什么格式的数据?
	 */
    tjpeg_dec dec;
    auto ret = dec.open(TJPF_BGR);
    RETURN_IF_NOT_OK(ret);

    std::vector<ins_img_frame> v_dec_img;
    for (int32_t i = 0; i < INS_CAM_NUM; i++) {
        auto it = m_frame.find(i);
        assert(it != m_frame.end());
	
        ins_img_frame F;
        ret = dec.decode(it->second->page_buf->data(), it->second->page_buf->size(), F);
        BREAK_IF_NOT_OK(ret);
        v_dec_img.push_back(F);
    }

    return do_compose(v_dec_img, m_frame.begin()->second->metadata, option);
}


int32_t image_mgr::do_compose(std::vector<ins_img_frame>& v_dec_img, const jpeg_metadata& ori_meta, const ins_picture_option& option)
{
    int32_t ret = INS_OK;

	#if 0
    // offset
    // int32_t hw_version = 3; 
    // camera_->get_module_hw_version(hw_version);
    // offset_wrap ow; 
    // auto ret = ow.calc(v_dec_img, hw_version);
    // std::string offset = ow.get_4_3_offset();
	#endif

    /* 默认使用工厂标定的offset,如果没有标定的使用用户校准的 */
    std::string offset = xml_config::get_offset(INS_CROP_FLAG_PIC);
    LOGINFO("pic offset:%s", offset.c_str());

    /*
	 * 拼接算法选择
	 */
    uint32_t type;
    if (option.stiching.algorithm == INS_ALGORITHM_OPTICALFLOW) {
        type = INS_BLEND_TYPE_OPTFLOW;
    } else {
        type = INS_BLEND_TYPE_TEMPLATE;
    }

    ins_blender blender(type, option.stiching.mode, ins::BLENDERTYPE::BGR_CUDA); 
    blender.set_map_type(option.stiching.map_type);
    blender.set_speed(ins::SPEED::SLOW);
    blender.setup(offset);

	/*
	 * 如果需要叠加水印,设置水印
	 */
    if (option.logo_file != "") 	/* 必须在setup后调用 */
        blender.set_logo(option.logo_file); 

    if (stablz_) {
        LOGINFO("-----pic pts:%lld", ori_meta.pts);
        ins_mat_3d mat;
        stablz_->get_cpu_rotation_mat(ori_meta.pts, mat);
        blender.set_rotation(mat.m);
    }

    /*
	 * 进行拼接处理
	 */
    ins_img_frame stitch_img;
    stitch_img.w = option.stiching.width;
    stitch_img.h = option.stiching.height;
    ret = blender.blend(v_dec_img, stitch_img);
    RETURN_IF_NOT_OK(ret);

	/*
	 * 拼接完成后进行编码,将图片转换为jpeg格式
	 */
    jpeg_metadata metadata = ori_meta;
    metadata.width = option.stiching.width;
    metadata.height = option.stiching.height;
    metadata.b_spherical = true;
    metadata.map_type = option.stiching.map_type;
    img_enc_wrap enc(option.stiching.mode, option.stiching.map_type, &metadata);
    std::stringstream ss;
    if (option.b_stiching) {   	/* 表示实时拼接 */
        ss << option.stiching.url << "/" << ins_util::create_name_by_mode(option.stiching.mode) << ".jpg";
    } else {   					/* 表示非实时拼接生成缩略图 */
        ss << option_.path << "/thumbnail.jpg";
    }

	/*
	 * 编码图片为jpeg格式
	 */
    std::vector<std::string> url;
    url.push_back(ss.str());
    ret = enc.encode(stitch_img, url);
    RETURN_IF_NOT_OK(ret);

    if (option.b_stiching) {   /* 表示实时拼接，要同时生成缩略图 */
        std::string s = option.stiching.url + "/thumbnail.jpg";
        enc.create_thumbnail(stitch_img, s);	/* 创建缩略图 */
    }
	
    return INS_OK;
}

int32_t image_mgr::hdr_compose()
{
    int32_t count;
    if (option_.hdr.enable) {				/* 使能HDR */
        count = option_.hdr.count;
    } else if (option_.bracket.enable) {	/* 使能bracket */
        count = option_.bracket.count;
    } else {
        LOGERR("not hdr/bracket no hdr compose");
        return INS_ERR;
    }

    std::vector<std::vector<std::string>> vv_file;
    for (int32_t i = INS_CAM_NUM; i > 0; i--) {   /* 拼接顺序为 8 - 1 */
        std::vector<std::string> v_file;
        for (int32_t j = 1; j <= count; j++) {
            std::stringstream ss;
            ss << option_.path << "/origin_" << j << "_" << i << ".jpg";
            v_file.push_back(ss.str());
        }
        vv_file.push_back(v_file);
    }

    all_lens_hdr HDR;
    auto v_hdr_img = HDR.process(vv_file);
    if (v_hdr_img.size() != INS_CAM_NUM) 
        return INS_ERR;

    metadata_.width = option_.stiching.width;
    metadata_.height = option_.stiching.height;
    metadata_.b_spherical = true;
    metadata_.map_type = option_.stiching.map_type;
    return do_compose(v_hdr_img, metadata_, option_);
}


int32_t image_mgr::calibration(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame)
{
    //dec
    tjpeg_dec dec;
    auto ret = dec.open(TJPF_BGR);
    RETURN_IF_NOT_OK(ret);

    std::vector<ins_img_frame> dec_img;
    for (int32_t i = 0; i < INS_CAM_NUM; i++) {
        auto it = m_frame.find(i);
        assert(it != m_frame.end());
        ins_img_frame F;
        ret = dec.decode(it->second->page_buf->data(),it->second->page_buf->size(), F);
        BREAK_IF_NOT_OK(ret);
        dec_img.push_back(F);
    }

    //offset
    int32_t hw_version = 3;
    camera_->get_module_hw_version(hw_version);
    offset_wrap ow;
    ret = ow.calc(dec_img, hw_version);
    RETURN_IF_NOT_OK(ret);
    
    std::string offset = ow.get_offset();
    if (offset != "") {
		LOGINFO("picture offset:%s", offset.c_str());
		xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3, offset);
		// xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT, offset_4_3);
		// xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT, offset_4_3);
	}

	#if 0
	std::string offset_16_9 = ow.get_16_9_offset();
	if (offset_16_9 != "") {
		LOGINFO("pano 16:9 offset:\n%s", offset_16_9.c_str());
		xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9, offset_16_9);
	}

	std::string offset_2_1 = ow.get_2_1_offset();
	if (offset_2_1 != "") {
		LOGINFO("pano 2:1 offset:\n%s", offset_2_1.c_str());
		xml_config::set_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_2_1, offset_2_1);
	}
	#endif
	
    return INS_OK;
}


int32_t image_mgr::save_origin(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame)
{   
	for (auto it = m_frame.begin(); it != m_frame.end(); it++) {
        /* 单模组拍照：所有都拍但是只存一个 */
		if (option_.index != -1 && (int32_t)it->second->pid != option_.index) continue;

		std::stringstream ss;
		if (option_.burst.enable || option_.hdr.enable || option_.bracket.enable) {
			ss << option_.path << "/origin_" << it->second->sequence << "_" << it->second->pid;
		} else {
			ss << option_.path << "/origin_" << it->second->pid;
		}

        if (it->second->metadata.raw) 
            ss << INS_RAW_EXT;
        else 
            ss << INS_JPEG_EXT;

        jpeg_muxer muxer;
		auto ret = muxer.create(ss.str(), it->second->page_buf->data(),it->second->page_buf->offset(), &it->second->metadata);
		RETURN_IF_NOT_OK(ret);
	}

	return INS_OK;
}


void image_mgr::open_usb_sink(std::string path)
{
    std::string filename = path + "/" + INS_PROJECT_FILE;
	usb_sink_ = std::make_shared<usb_sink>();
	usb_sink_->open(camera_);
	usb_sink_->set_gps(true);
	gps_mgr::get()->add_output(usb_sink_);
	usb_sink_->set_prj_file(filename);
}



/**********************************************************************************************
 * 函数名称: send_pic_finish_msg
 * 功能秒数: 发送拍照完成的异步消息
 * 参   数: 
 *      err - 错误码
 *      path - 存储照片的路径
 * 返 回 值: 无
 *********************************************************************************************/
void image_mgr::send_pic_finish_msg(int err, const std::string& path) const
{
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_PIC_FINISH);
	obj.set_string("path", path);
	obj.set_int("code", err);
	access_msg_center::queue_msg(0, obj.to_string());
}



/**********************************************************************************************
 * 函数名称: print_option
 * 功能秒数: 打印拍照参数option
 * 参   数: 
 *      option - 拍照参数
 * 返 回 值: 无
 *********************************************************************************************/
void image_mgr::print_option(const ins_picture_option& option) const
{
	LOGINFO("origin option: storage:%d mime:%s width:%d height:%d module url:%s delay:%d stab:%d thumbnail:%d logo:%s", 
        option.origin.storage_mode,
        option.origin.mime.c_str(),
		option.origin.width, 
		option.origin.height,   
        option.origin.module_url.c_str(), 
		option.delay, 
		option.b_stabilization, 
		option.b_thumbnail, 
		option.logo_file.c_str());

	if (option.b_stiching) {
		LOGINFO("stiching option: width:%d height:%d framerate:%d bitrate:%d, mode:%d map:%d url:%s",  
			option.stiching.width, 
			option.stiching.height, 
			option.stiching.framerate, 
			option.stiching.bitrate, 
			option.stiching.mode, 
			option.stiching.map_type,
			option.stiching.url.c_str());
	}

	if (option.burst.enable) {
		LOGINFO("burst enable:%d count:%d", option.burst.enable, option.burst.count);
	}

	if (option.hdr.enable) {
		LOGINFO("hdr enable:%d count:%d min ev:%d max ev:%d", option.hdr.enable, option.hdr.count, option.hdr.min_ev, option.hdr.max_ev);
	}

    if (option.bracket.enable) {
		LOGINFO("bracket enable:%d count:%d min ev:%d max ev:%d", option.bracket.enable, option.bracket.count, option.bracket.min_ev, option.bracket.max_ev);
	}
}	

