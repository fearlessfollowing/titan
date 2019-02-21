
#ifndef ALL_CAM_ORIGIN_STREAM_H_
#define ALL_CAM_ORIGIN_STREAM_H_

#include "all_cam_queue_i.h"
#include <map>
#include <memory>
#include <mutex>
#include "common.h"

class all_cam_origin_stream : public all_cam_queue_i
{
public:
	virtual ~all_cam_origin_stream(){ };
	all_cam_origin_stream(std::map<unsigned int, std::shared_ptr<sink_interface>>& sink, ins_video_param& video_param);
	virtual void set_sps_pps(std::map<unsigned int, std::shared_ptr<insbuff>>& sps, std::map<unsigned int, std::shared_ptr<insbuff>>& pps) override;
	virtual void queue_frame(std::map<unsigned int, std::shared_ptr<ins_frame>> frame) override;
	virtual bool is_sps_pps_set() override { return b_sps_pps_set_;}

private:
	ins_video_param video_param_;
	std::map<unsigned int, std::shared_ptr<sink_interface>> stream_sink_;
	bool b_sps_pps_set_ = false;
	std::map<unsigned int, long long> position_;
};

#endif