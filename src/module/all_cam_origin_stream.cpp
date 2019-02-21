
#include "all_cam_origin_stream.h"
#include "stream_sink.h"
#include <sstream>

all_cam_origin_stream::all_cam_origin_stream(
	std::map<unsigned int, std::shared_ptr<sink_interface>>& sink, 
	ins_video_param& video_param)
	: video_param_(video_param)
	, stream_sink_(sink)
{
	for (auto it = sink.begin(); it != sink.end(); it++)
	{
		position_.insert(std::make_pair(it->first, 0));
	}
}

void all_cam_origin_stream::set_sps_pps(
	std::map<unsigned int, std::shared_ptr<insbuff>>& sps, 
	std::map<unsigned int, std::shared_ptr<insbuff>>& pps)
{
	b_sps_pps_set_ = true;

	for (auto it = stream_sink_.begin(); it != stream_sink_.end(); it++)
	{
		unsigned int index = it->first;
		video_param_.config = std::make_shared<insbuff>(sps[index]->size() + pps[index]->size());
		memcpy(video_param_.config->data(), sps[index]->data(), sps[index]->size());
		memcpy(video_param_.config->data()+sps[index]->size(), pps[index]->data(), pps[index]->size());
		it->second->set_video_param(video_param_);
	}
}

void all_cam_origin_stream::queue_frame(std::map<unsigned int, std::shared_ptr<ins_frame>> frame)
{
	//6路原始流MP4文件分段要同步 3500000000
	bool b_frag = false;
	for (auto it = frame.begin(); it != frame.end(); it++)
	{
		position_[it->first] += it->second->page_buf->size();
		if (position_[it->first] > 3500000000 && it->second->is_key_frame)
		{
			b_frag = true;
			break;
		}
	}

	for (auto it = stream_sink_.begin(); it != stream_sink_.end(); it++)
	{
		if (b_frag) 
		{
			position_[it->first] = 0;
			frame[it->first]->b_fragment = true;
		}
		it->second->queue_frame(frame[it->first]);
	}
}

