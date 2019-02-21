
#include "singlen_mgr.h"
#include "one_cam_video_buff.h"
#include "common.h"
#include "inslog.h"
#include "xml_config.h"
#include "Offset/Offset.h"
#include "singlen_focus.h"

int32_t singlen_mgr::start_focus(cam_manager* camera, const ins_video_option& opt)
{
    print_option(opt);
    
	camera_ = camera; 

	int32_t index = camera_->get_index(opt.index);

    auto video_buff = std::make_shared<one_cam_video_buff>();
	std::map<int,std::shared_ptr<cam_video_buff_i>> m_video_buff;
    m_video_buff.insert(std::make_pair(index, video_buff));

    cam_video_param param;
	param.width = opt.origin.width;
	param.height = opt.origin.height;
	param.framerate = opt.origin.framerate;
	param.bitrate = opt.origin.bitrate;
	param.b_file_stream = false; 
	param.b_usb_stream = true;

	auto ret = camera_->set_all_video_param(param, nullptr); 
	RETURN_IF_NOT_OK(ret);

	ret = camera_->start_all_video_rec(m_video_buff); 
	RETURN_IF_NOT_OK(ret);

    float x = 0.0; 
    float y = 0.0;
    get_circle_pos(index, x, y);

    focus_ = std::make_shared<singlen_focus>();
	focus_->set_video_repo(video_buff);
    ret = focus_->open(x, y);
    RETURN_IF_NOT_OK(ret);

    return INS_OK;
}

int32_t singlen_mgr::stop_focus()
{
	if (camera_)
	{
		auto ret = camera_->stop_all_video_rec();
		camera_ = nullptr;
		return ret;
	}
	else
	{
		return INS_OK;
	}
}

void singlen_mgr::print_option(const ins_video_option& option) const
{
	LOGINFO("index:%d origin w:%d h:%d fps:%d bitrate:%d", 
        option.index,
		option.origin.width, 
		option.origin.height, 
		option.origin.framerate, 
		option.origin.bitrate);
}

int32_t singlen_mgr::get_circle_pos(int32_t index, float& x, float& y)
{
	std::string offset = xml_config::get_offset(INS_CROP_FLAG_PIC);

	ins::Offset offset_tool;
	offset_tool.setOffset(offset);
	std::vector<float> c_x;
	std::vector<float> c_y;
	std::vector<float> c_r;
	offset_tool.getCircle(c_x, c_y, c_r);

    std::vector<int32_t> w;
    std::vector<int32_t> h;
    offset_tool.getWidthAndHeight(w, h);

	if ((int32_t)c_x.size() <= index || (int32_t)w.size() <= index)
	{
		LOGERR("circle info size:%d < index:%d", c_x.size(), index);
		return INS_ERR;
	}
	
	x = 2.0*c_x[index]/w[index] - 1.0;
	y = 1.0 - 2.0*c_y[index]/h[index];

	LOGINFO("circle x:%f y:%f w:%d h:%d x:%f y:%f", c_x[index], c_y[index], w[index], h[index], x, y);

	return INS_OK;
}