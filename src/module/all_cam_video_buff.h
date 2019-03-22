
#ifndef _ALL_CAM_VIDEO_BUFF_H_
#define _ALL_CAM_VIDEO_BUFF_H_

#include <map>
#include <memory>
#include <vector>
#include "all_cam_queue_i.h"
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "cam_video_buff_i.h"
#include <chrono>
#include "gyro_sink.h"

//#define DEBUG_GYRO_PTS

class all_cam_video_buff : public cam_video_buff_i {
public:
    				all_cam_video_buff(const std::vector<uint32_t>& index, std::string path = "");
    				~all_cam_video_buff(){};
	virtual void 	set_sps_pps(uint32_t index, const std::shared_ptr<insbuff>& sps, const std::shared_ptr<insbuff>& pps) override;
	virtual void 	queue_frame(uint32_t index, std::shared_ptr<ins_frame> frame) override;
    virtual void 	queue_gyro(const uint8_t* data, uint32_t size, uint64_t delta_ts) override; //模组间对齐时间
	virtual void 	queue_expouse(uint32_t pid, uint32_t seq, uint64_t ts, uint64_t exposure_ns) override;
    virtual void 	set_first_frame_ts(uint32_t pid, int64_t timestamp) override;

    void clear();

    void add_output(uint32_t index, const std::shared_ptr<all_cam_queue_i>& out) {
        std::lock_guard<std::mutex> lock(mtx_out_);
        out_.insert(std::make_pair(index, out));
    }
	
    void del_output(uint32_t index) {
        std::lock_guard<std::mutex> lock(mtx_out_);
        auto it = out_.find(index);
        if (it != out_.end()) out_.erase(it);
    }
	
    void add_gyro_sink(std::shared_ptr<gyro_sink>& sink) {
        std::lock_guard<std::mutex> lock(mtx_gyro_);
        if (!buff_pool_) buff_pool_ = std::make_shared<obj_pool<insbuff>>(-1, "gyro buff");
        if (!exp_pool_) exp_pool_ = std::make_shared<obj_pool<exposure_times>>(-1, "exp obj");
        sinks_.push_back(sink);
    }
	
    void del_all_gyro_sink() {
        std::lock_guard<std::mutex> lock(mtx_gyro_);
        sinks_.clear();
    }

private:
    void sync_frame();
    void output(const std::map<uint32_t, std::shared_ptr<ins_frame>>& frame);
	
    std::map<uint32_t, std::shared_ptr<insbuff>> sps_;					/* 存放各模块的SPS数据 */
	std::map<uint32_t, std::shared_ptr<insbuff>> pps_;					/* 存放各模块的PPS数据 */
	
    std::mutex 		mtx_;
    std::mutex 		mtx_out_;
    std::mutex 		mtx_gyro_; //gyor && exposure
    
	std::map<uint32_t, std::queue<std::shared_ptr<ins_frame>>> queue_;	/* 存放所有模块数据帧的缓冲区队列 */

    bool 										b_wait_idr_ = true;

    std::map<uint32_t, std::shared_ptr<all_cam_queue_i>> out_;			/* <Origin/Aux, all_cam_video_buff> */
	
    std::chrono::system_clock::time_point 		sync_point_;
    bool 										quit_ = false;
	
    std::map<uint32_t, std::shared_ptr<exposure_times>> exp_queue_;		/* 存放所有模块的曝光时间 */

	std::shared_ptr<obj_pool<insbuff>> 			buff_pool_;
    std::shared_ptr<obj_pool<exposure_times>> 	exp_pool_;
	
    std::vector<std::shared_ptr<gyro_sink>> 	sinks_;

	std::map<uint32_t, int64_t> 				first_frame_ts_;		/* 各模组第一帧的时间戳 */

	std::string 								path_;					/* 工程文件路径(需要更新第一帧的时间戳) */
	
    #ifdef DEBUG_GYRO_PTS
    int64_t last_pts_ = 0;
    #endif
};

#endif