
#include "all_cam_video_queue.h"
#include "common.h"
#include "inslog.h"
#include "access_msg_center.h"
#include <sstream>

#define CAM_VIDEO_MAX_SIZE 60

all_cam_video_queue::all_cam_video_queue(std::vector<unsigned int>& index, bool b_just_keyframe)
	: b_just_keyframe_(b_just_keyframe)
{
	LOGINFO("need just key frame:%d", b_just_keyframe_);
 	for (unsigned int i = 0; i < index.size(); i++) {
		std::queue<std::shared_ptr<ins_frame>> queue;
		queue_.insert(std::make_pair(index[i], queue));
		//key_frames_not_deque_.insert(std::make_pair(index[i], 0));
	}
}

void all_cam_video_queue::set_sps_pps(std::map<unsigned int, std::shared_ptr<insbuff>>& sps, std::map<unsigned int, std::shared_ptr<insbuff>>& pps)
{
	LOGINFO("all_cam_video_queue set sps pps");
	for (auto it = sps.begin(); it != sps.end(); it++) {
		INS_ASSERT(it->second != nullptr);
	}

	for (auto it = pps.begin(); it != pps.end(); it++) {
		INS_ASSERT(it->second != nullptr);
	}

	std::lock_guard<std::mutex> lock(mtx_);
	b_sps_pps_set_ = true;
	sps_ = sps;
	pps_ = pps;
}

void all_cam_video_queue::queue_frame(std::map<unsigned int, std::shared_ptr<ins_frame>> frame)
{
	if (quit_) return;
	if (b_just_keyframe_ && !frame.begin()->second->is_key_frame) return;

	std::lock_guard<std::mutex> lock(mtx_);

	do_discard_frame(frame.begin()->second->is_key_frame);

	for (auto it = queue_.begin(); it != queue_.end(); it++) {
		// if (frame[it->first]->is_key_frame)
		// {
		// 	key_frames_not_deque_[it->first] += 1;
		// }
		// if (key_frames_not_deque_[it->first] > 5)
		// {
		// 	LOGERR("pid:%d havn't dequeue frame for long time, codec may die", frame[it->first]->pid);
		// 	quit_ = true;
		// 	send_reset_msg(INS_ERR_CODEC_DIE);
		// 	return;
		// }

		it->second.push(frame[it->first]);
	}
}

void all_cam_video_queue::do_discard_frame(bool b_keyframe)
{
	// static int64_t test_cnt = 0;
	// if (test_cnt++%100 == 0)
	// {
	// 	for (auto& it : queue_)
	// 	{
	// 		printf("index:%d-%lu ", it.first, it.second.size());
	// 	}
	// 	printf("\n");
	// }

	if (queue_.begin()->second.size() > 45) {
		full_cnt_++;
	} else {
		full_cnt_ = 0;
	}

	if (full_cnt_ > 90) discard_frames_++;

	if (queue_.begin()->second.size() > 120) {
		discard_frames_ = queue_.begin()->second.size() - 10;
	} else if (queue_.begin()->second.size() > 10) {
		discard_frames_ = std::min(discard_frames_, (unsigned)(queue_.begin()->second.size()-10));
	} else {
		discard_frames_ = 0;
	}

	if (!b_keyframe || discard_frames_ <= 0) return;

	LOGINFO("video sync queue discard frames:%d", discard_frames_);

	for (auto it = queue_.begin(); it != queue_.end(); it++) {
		int cnt = discard_frames_;
		while (!it->second.empty() && cnt-- > 0) {
			it->second.pop();
		}

		//一般不会出现这种各路之间相差30帧的情况，如果出现说明解码器肯定异常了，只能reset解决了
		if (cnt > 0) {
			LOGERR("sync queue size diff too many, codec may in error state");
			quit_ = true;
			send_reset_msg(INS_ERR_CODEC_DIE);
		}
	}
	discard_frames_ = 0;
}

std::shared_ptr<insbuff> all_cam_video_queue::get_sps(unsigned int index)
{
	while (1) {
		mtx_.lock();
		auto it = sps_.find(index);
		if (it != sps_.end()) {
			mtx_.unlock();
			return it->second;
		} else {
			mtx_.unlock();
			usleep(10000); //10ms
		}
	}
}

std::shared_ptr<insbuff> all_cam_video_queue::get_pps(unsigned int index)
{
	while (1) {
		mtx_.lock();
		auto it = pps_.find(index);
		if (it != pps_.end()) {
			mtx_.unlock();
			return it->second;
		} else {
			mtx_.unlock();
			usleep(10000); //10ms
		}
	}
}

std::shared_ptr<ins_frame> all_cam_video_queue::deque_frame(unsigned int index)
{
	std::lock_guard<std::mutex> lock(mtx_);

	//key_frames_not_deque_[index] = 0;

	auto it = queue_.find(index);
	if (it == queue_.end()) {
		return nullptr;
	}

	if (it->second.empty()) {
		// printf("index:%d deque frame null\n", index);
		return nullptr;
	} else {
		// printf("index:%d deque frame success\n", index);
		auto frame = it->second.front();
		it->second.pop();
		return frame;
	}
}

void all_cam_video_queue::send_reset_msg(int error)
{
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_RESET);
	json_obj param_obj;
	param_obj.set_int("code", error);
	obj.set_obj("parameters", &param_obj);
	access_msg_center::queue_msg(0, obj.to_string());
}

