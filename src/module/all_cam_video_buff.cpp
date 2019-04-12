
#include "all_cam_video_buff.h"
#include "common.h"
#include "inslog.h"
#include <fcntl.h>
#include <sstream>
#include "access_msg_center.h"
#include "prj_file_mgr.h"

using namespace std::chrono;



/***********************************************************************************************
** 函数名称: set_first_frame_ts
** 函数功能: 设置第一帧的时间戳
** 入口参数:	
**		pid - 模块的PID
**		timestamp - 时间戳
** 返 回 值: 无
*************************************************************************************************/
all_cam_video_buff::all_cam_video_buff(const std::vector<unsigned int>& index, std::string path)
{
    INS_ASSERT(index.size() > 0);
    path_ = path;

	for (unsigned int i = 0; i < index.size(); i++) {
		std::queue<std::shared_ptr<ins_frame>> queue;
        queue_.insert(std::make_pair(index[i], queue));
        sps_.insert(std::make_pair(index[i], nullptr));
        pps_.insert(std::make_pair(index[i], nullptr));
	}
}



void all_cam_video_buff::clear()
{
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto it = sps_.begin(); it != sps_.end(); it++) {
        it->second = nullptr;
    }

    for (auto it = pps_.begin(); it != pps_.end(); it++) {
        it->second = nullptr;
    }

    for (auto it = queue_.begin(); it != queue_.end(); it++) {
        std::queue<std::shared_ptr<ins_frame>> empty;
        std::swap(it->second, empty);
        //it->second.clear();
    }

    b_wait_idr_ = true;
}


void all_cam_video_buff::set_sps_pps(unsigned int index, const std::shared_ptr<insbuff>& sps, const std::shared_ptr<insbuff>& pps)
{
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = sps_.find(index);
    if (it != sps_.end()) it->second = sps;

    auto it1 = pps_.find(index);
    if (it1 != pps_.end()) it1->second = pps;
}


/***********************************************************************************************
** 函数名称: queue_frame
** 函数功能: 设置第一帧的时间戳
** 入口参数:	
**		pid - 模块的PID
**		timestamp - 时间戳
** 返 回 值: 无
** 注: 各读模组数据的线程,在接收到视频帧后都会调用queue_frame,将视频帧丢入到all_cam_video_buff::queue_
** 		中,需要线程互斥保护
*************************************************************************************************/
void all_cam_video_buff::queue_frame(unsigned int index, std::shared_ptr<ins_frame> frame)
{
    //LOGINFO("index:%d sync queue frame iskey:%d pts:%lld", index, frame->is_key_frame, frame->pts);

	std::lock_guard<std::mutex> lock(mtx_);

	/* 1.从map中找到index对应的queue(it->second) */
    auto it = queue_.find(index);
    if (it == queue_.end()) return;

	/* 2.将数据放入queue */
    it->second.push(frame);

	/* 3.检查是否需要进行丢帧处理 */
    if (it->second.size() > 135) {	/* 队列中的数据帧大于135帧 */
        uint32_t discard_cnt = 0;
        while (!it->second.empty()) {	/* 队列不为空 */
            if (it->second.front()->is_key_frame && discard_cnt > 25) break;	/* 丢最少25帧 */
            it->second.pop();
            discard_cnt++;
        }
        
        LOGERR("pid:%d discard unsync frames:%d, left:%lu", frame->pid, discard_cnt, it->second.size());
        b_wait_idr_ = true;
    } else {
	    sync_frame();	/* 进行帧同步 */
    }
}




/***********************************************************************************************
** 函数名称: sync_frame
** 函数功能: 帧同步(10s之内必须进行帧同步操作(map中各模组队列中的队首数据帧的时间戳必须控制在一个合理的范围内))
** 入口参数:	 无
** 返 回 值: 无
*************************************************************************************************/
void all_cam_video_buff::sync_frame()
{
    if (quit_) return;

    if (sync_point_.time_since_epoch().count() == 0) {	/* 第一次 */
        sync_point_ = system_clock::now();
    } else {	/* 本次进行帧同步的时间距上次大于10s,将报467错误 */
        auto not_sync_time = duration_cast<duration<double>>(system_clock::now() - sync_point_).count();
        if (not_sync_time > 10) {	/* 报帧不同步错误(-467) */
            LOGINFO("not sync time:%lf, quit", not_sync_time);
            quit_ = true;
            json_obj obj;
            obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
            obj.set_int("code", INS_ERR_CAMERA_FRAME_NOT_SYNC);
            access_msg_center::queue_msg(0, obj.to_string());
            return;
        }
    }

    int loop_cnt = 10;

	/*
	 * 一切OK的情况下调用sync_frame最大能处理10组同步帧
	 */
	while (loop_cnt--) {

		/*
	 	 * 需要等待关键帧的情况:
	 	 * 1.第一次调用sync_frame()
	 	 * 2.有丢帧
	 	 */
		if (b_wait_idr_) {	/* 需要等关键帧: 需要确保所有模组的帧队列中的第一帧为关键帧 */
            for (auto it = queue_.begin(); it != queue_.end(); it++) {   
                int size = it->second.size();	/* 记录当前模组帧队列中帧数 */
                uint32_t pid = -1;
				while (!it->second.empty()) {	/* 一直丢,直到找到第一个关键帧 */
                    pid = it->second.front()->pid;
					if (it->second.front()->is_key_frame) break;
					it->second.pop();
				}
                size  -= it->second.size();
                if (size > 0) {	/* 打印被丢的帧数 */
                    LOGERR("pid:%d discard frames:%d", pid, size);
                }
			}
		}

		/* 因为有可能有丢帧操作,需要检查此时所有模组的帧队列中是否都还有数据帧, 如果没有直接返回 */
		for (auto it = queue_.begin(); it != queue_.end(); it++) {
			if (it->second.empty()) return;
		}


		b_wait_idr_ = false;

        auto it = queue_.begin();
        long long pts = it->second.front()->pts;
		long long min_pts = pts;	/* master第一帧的时间戳 */
		bool b_sync = true;
        it++;

		/* 依次遍历各模组的第一帧的时间戳 */
		for (; it != queue_.end(); it++) {
			if (it->second.front()->pts > pts + 30000 || it->second.front()->pts < pts - 30000) 	/* 如果某个模组的第一帧时间戳与Master的时间戳在3ms范围之外 */
				b_sync = false;		/* 需要进行帧同步操作 */

			if (it->second.front()->pts < min_pts) {	/* min_pts将保存一组视频帧中时间戳最小那个 */
				min_pts = it->second.front()->pts;
			}
		}

		if (b_sync) {	/* 帧同步成功 */
            std::map<unsigned int, std::shared_ptr<ins_frame>> frames;
            for (auto it = queue_.begin(); it != queue_.end(); it++) {	/* 取出各模组的第一帧数据丢入到frames map中 */
                auto frame = it->second.front();
                it->second.pop();
                frames.insert(std::make_pair(it->first, frame));
            }
            //LOGINFO("frame sync pts:%lld %lld %lld %lld %lld %lld",frame[0]->pts, frame[1]->pts,frame[2]->pts,frame[3]->pts,frame[4]->pts,frame[5]->pts);

            output(frames);		/* 将同步帧丢入到输出队列中 */

            sync_point_ = system_clock::now();	/* 更新上次同步的时间点 */
		} else {		/* 帧同步失败 */
			LOGINFO("-----------frame not sync");

			/* 帧不同步,将时间戳最早的模组的第一帧丢掉 */
            for (auto it = queue_.begin(); it != queue_.end(); it++) {
                auto& f =  it->second.front();
                LOGINFO("pid:%d iskey:%d pts:%lld size:%lu", f->pid, f->is_key_frame, f->pts, it->second.size());
                if (min_pts == f->pts) 
					it->second.pop();
            }
			b_wait_idr_ = true;	/* 有丢帧操作,需等下一个关键帧 */
		}
	}
}



/***********************************************************************************************
** 函数名称: output
** 函数功能: 将同步好一组视频帧丢入所有的输出组件队列中
** 入口参数:	 
**		frame - 一组同步帧
** 返 回 值: 无
*************************************************************************************************/
void all_cam_video_buff::output(const std::map<unsigned int, std::shared_ptr<ins_frame>>& frame)
{
    std::lock_guard<std::mutex> lock(mtx_out_);

    auto b_key_frame = frame.begin()->second->is_key_frame;		/* 判断是否为关键帧 */

	/* 遍历所有的输出接口(比如拼接器) */
    for (auto it = out_.begin(); it != out_.end(); it++) {   
        if (!it->second->is_sps_pps_set()) {	/* 如果没有设置SPS/PPS */
            if (b_key_frame) {					/* 当前一组数据的关键帧 */
                it->second->set_sps_pps(sps_, pps_);
            } else {
                continue;
            }
        }
        it->second->queue_frame(frame); /* 将同步好的一组视频数据丢入到下一组建中处理 */
    }
}



/***********************************************************************************************
** 函数名称: set_first_frame_ts
** 函数功能: 设置第一帧的时间戳
** 入口参数:	
**		pid - 模块的PID
**		timestamp - 时间戳
** 返 回 值: 无
*************************************************************************************************/
void all_cam_video_buff::set_first_frame_ts(uint32_t pid, int64_t timestamp)
{
    if (path_ == "") return;
    
    mtx_.lock();
    first_frame_ts_.insert(std::make_pair(pid, timestamp));
    mtx_.unlock();

	/* 收集齐了所有模组的第一帧时间戳, 将时间戳写入工程文件中 */
    if (first_frame_ts_.size() == INS_CAM_NUM) {	
        prj_file_mgr::add_first_frame_ts(path_, first_frame_ts_);
    }
}


void all_cam_video_buff::queue_expouse(uint32_t pid, uint32_t seq, uint64_t ts, uint64_t exposure_ns)
{
    std::lock_guard<std::mutex> lock(mtx_gyro_);
    if (sinks_.empty()) return;
    if (pid > INS_CAM_NUM) return;

    auto it = exp_queue_.find(seq);
    if (it == exp_queue_.end()) {
        auto exp = exp_pool_->pop();
        exp->cnt = 0;
        exp_queue_.insert(std::make_pair(seq, exp));
        it = exp_queue_.find(seq);
    }
	
    if (pid == INS_CAM_NUM) 
		it->second->pts = ts; //时间戳以pid6为准

    it->second->expousre_time_ns[pid-1] = exposure_ns;
    it->second->cnt++;

    //LOGINFO("pid:%d seq:%d ts:%ld cnt:%d exp queue:%ld 1", pid, seq, ts, it->second->cnt, exp_queue_.size());
    if (it->second->cnt == INS_CAM_NUM) {
        for (auto& sink : sinks_) 
			sink->queue_exposure_time(it->second);
        exp_queue_.erase(it);
        //LOGINFO("pid:%d exp queue:%ld  2", pid, exp_queue_.size());
    }
	
}



void all_cam_video_buff::queue_gyro(const uint8_t* data, uint32_t size, uint64_t delta_ts)
{
    if (sinks_.empty()) return;
    std::shared_ptr<insbuff> buff;

    if (size > 4096*4) {
        buff = std::make_shared<insbuff>(size);
        LOGINFO("gyro buff size:%d !!!!!!!!!!!!!!!!!", size);
    } else {
        buff = buff_pool_->pop();
        if (!buff->data()) buff->alloc(4096*4);
    }
    memcpy(buff->data(), data, size);
    buff->set_offset(size);

    #ifdef DEBUG_GYRO_PTS
    uint32_t offset = 0;
    while (offset + sizeof(amba_gyro_data) <= size) {
        auto gyro = (amba_gyro_data*)(data + offset);
        if (gyro->pts <= last_pts_) {
            LOGERR("-----last pts:%ld cur pts:%ld offset:%d", last_pts_, gyro->pts, offset);
            abort();
        }
        offset += sizeof(amba_gyro_data);
        last_pts_ = gyro->pts;
    }
    #endif

    for (auto& it : sinks_) {
        it->queue_gyro(buff, delta_ts);
    }
}


