
#ifndef _ALL_CAM_VIDEO_QUEUE_H_
#define _ALL_CAM_VIDEO_QUEUE_H_

#include "all_cam_queue_i.h"
#include <mutex>
#include <queue>
#include <vector>

class all_cam_video_queue : public all_cam_queue_i {
public:
								all_cam_video_queue(std::vector<unsigned>& index, bool b_just_keyframe = false);
	virtual 					~all_cam_video_queue(){};
	virtual void 				set_sps_pps(std::map<unsigned, std::shared_ptr<insbuff>>& sps, std::map<unsigned, std::shared_ptr<insbuff>>& pps) override;
	virtual void 				queue_frame(std::map<unsigned, std::shared_ptr<ins_frame>> frame) override;
	virtual bool 				is_sps_pps_set() override { return b_sps_pps_set_;}
	std::shared_ptr<insbuff> 	get_sps(unsigned index);
	std::shared_ptr<insbuff> 	get_pps(unsigned index);
	std::shared_ptr<ins_frame>	deque_frame(unsigned index);

private:
	void 	do_discard_frame(bool b_keyframe);
	void 	send_reset_msg(int error);
	
	std::map<unsigned, std::shared_ptr<insbuff>> 				sps_;
	std::map<unsigned, std::shared_ptr<insbuff>> 				pps_;
	std::mutex 													mtx_;
	std::map<unsigned, std::queue<std::shared_ptr<ins_frame>>> 	queue_;
	std::map<unsigned, long long> 								key_frames_not_deque_;
	bool 														b_just_keyframe_ = false;
	bool 														b_sps_pps_set_ = false;
	unsigned 													discard_frames_ = 0;
	unsigned 													full_cnt_ = 0;
	bool 														quit_  = false;
};

#endif
