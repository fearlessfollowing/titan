
#include "all_cam_video_buff.h"
#include "common.h"
#include "inslog.h"
#include <fcntl.h>
#include <sstream>
#include "access_msg_center.h"
#include "prj_file_mgr.h"

using namespace std::chrono;

all_cam_video_buff::all_cam_video_buff(const std::vector<unsigned int>& index, std::string path)
{
    INS_ASSERT(index.size() > 0);
    path_ = path;

   for (unsigned int i = 0; i < index.size(); i++)
	{
		std::queue<std::shared_ptr<ins_frame>> queue;
        queue_.insert(std::make_pair(index[i], queue));
        sps_.insert(std::make_pair(index[i], nullptr));
        pps_.insert(std::make_pair(index[i], nullptr));
	}
}

void all_cam_video_buff::clear()
{
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto it = sps_.begin(); it != sps_.end(); it++)
    {
        it->second = nullptr;
    }

    for (auto it = pps_.begin(); it != pps_.end(); it++)
    {
        it->second = nullptr;
    }

    for (auto it = queue_.begin(); it != queue_.end(); it++)
    {
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

void all_cam_video_buff::queue_frame(unsigned int index, std::shared_ptr<ins_frame> frame)
{
    //LOGINFO("index:%d sync queue frame iskey:%d pts:%lld", index, frame->is_key_frame, frame->pts);

	std::lock_guard<std::mutex> lock(mtx_);

    auto it = queue_.find(index);
    if (it == queue_.end()) return;
    it->second.push(frame);

    if (it->second.size() > 135)
    {
        uint32_t discard_cnt = 0;
        while (!it->second.empty())
        {
            if (it->second.front()->is_key_frame && discard_cnt > 25) break;
            it->second.pop();
            discard_cnt++;
        }
        
        LOGERR("pid:%d discard unsync frames:%d, left:%lu", frame->pid, discard_cnt, it->second.size());

        b_wait_idr_ = true;
    }
    else
    {
	    sync_frame();
    }
}

void all_cam_video_buff::sync_frame()
{
    if (quit_) return;

    //计时，如果不同步时间超过10s,作为异常退出
    if (sync_point_.time_since_epoch().count() == 0)
    {
        sync_point_ = system_clock::now();
    } 
    else
    {
        auto not_sync_time = duration_cast<duration<double>>(system_clock::now() - sync_point_).count();
        if (not_sync_time > 10)
        {
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
	while (loop_cnt--)
	{
		if (b_wait_idr_)
		{
            for (auto it = queue_.begin(); it != queue_.end(); it++)
			{   
                int size = it->second.size();
                uint32_t pid = -1;
				while (!it->second.empty())
				{
                    pid = it->second.front()->pid;
					if (it->second.front()->is_key_frame) break;
					it->second.pop();
				}
                size  -= it->second.size();
                if (size > 0)
                {
                    LOGERR("pid:%d discard frames:%d", pid, size);
                }
			}
		}

		for (auto it = queue_.begin(); it != queue_.end(); it++)
		{
			if (it->second.empty()) return;
		}

		b_wait_idr_ = false;

        auto it = queue_.begin();
        long long pts = it->second.front()->pts;
		long long min_pts = pts;
		bool b_sync = true;
        it++;
		for (; it != queue_.end(); it++)
		{
			if (it->second.front()->pts > pts + 30000 || it->second.front()->pts < pts - 30000) b_sync = false;
			if (it->second.front()->pts < min_pts)
			{
				min_pts = it->second.front()->pts;
			}
		}

		if (b_sync)
		{
            std::map<unsigned int, std::shared_ptr<ins_frame>> frames;
            for (auto it = queue_.begin(); it != queue_.end(); it++)
            {
                auto frame = it->second.front();
                it->second.pop();
                frames.insert(std::make_pair(it->first, frame));
            }
            //LOGINFO("frame sync pts:%lld %lld %lld %lld %lld %lld",frame[0]->pts, frame[1]->pts,frame[2]->pts,frame[3]->pts,frame[4]->pts,frame[5]->pts);
            output(frames);
            sync_point_ = system_clock::now();
		}
		else
		{
			LOGINFO("-----------frame not sync");

            for (auto it = queue_.begin(); it != queue_.end(); it++)
            {
                auto& f =  it->second.front();
                LOGINFO("pid:%d iskey:%d pts:%lld size:%lu", f->pid, f->is_key_frame, f->pts, it->second.size());
                if (min_pts == f->pts) it->second.pop();
            }
			b_wait_idr_ = true;
		}
	}
}

void all_cam_video_buff::output(const std::map<unsigned int, std::shared_ptr<ins_frame>>& frame)
{
    std::lock_guard<std::mutex> lock(mtx_out_);

    auto b_key_frame = frame.begin()->second->is_key_frame;

    for (auto it = out_.begin(); it != out_.end(); it++)
    {   
        if (!it->second->is_sps_pps_set())
        {
            if (b_key_frame)
            {
                it->second->set_sps_pps(sps_, pps_);
            }
            else
            {
                continue;
            }
        }
        it->second->queue_frame(frame);
    }
}

void all_cam_video_buff::set_first_frame_ts(uint32_t pid, int64_t timestamp)
{
    if (path_ == "") return;
    
    mtx_.lock();
    first_frame_ts_.insert(std::make_pair(pid, timestamp));
    mtx_.unlock();

    if (first_frame_ts_.size() == INS_CAM_NUM)
    {
        prj_file_mgr::add_first_frame_ts(path_, first_frame_ts_);
    }
}

void all_cam_video_buff::queue_expouse(uint32_t pid, uint32_t seq, uint64_t ts, uint64_t exposure_ns)
{
    std::lock_guard<std::mutex> lock(mtx_gyro_);
    if (sinks_.empty()) return;
    if (pid > INS_CAM_NUM) return;

    auto it = exp_queue_.find(seq);
    if (it == exp_queue_.end())
    {
        auto exp = exp_pool_->pop();
        exp->cnt = 0;
        exp_queue_.insert(std::make_pair(seq, exp));
        it = exp_queue_.find(seq);
    }
    if (pid == INS_CAM_NUM) it->second->pts = ts; //时间戳以pid6为准
    it->second->expousre_time_ns[pid-1] = exposure_ns;
    it->second->cnt++;
    //LOGINFO("pid:%d seq:%d ts:%ld cnt:%d exp queue:%ld 1", pid, seq, ts, it->second->cnt, exp_queue_.size());
    if (it->second->cnt == INS_CAM_NUM) 
    {
        for (auto& sink : sinks_) sink->queue_exposure_time(it->second);
        exp_queue_.erase(it);
        //LOGINFO("pid:%d exp queue:%ld  2", pid, exp_queue_.size());
    }
}

void all_cam_video_buff::queue_gyro(const uint8_t* data, uint32_t size, uint64_t delta_ts)
{
    if (sinks_.empty()) return;
    std::shared_ptr<insbuff> buff;
    if (size > 4096*4)
    {
        buff = std::make_shared<insbuff>(size);
        LOGINFO("gyro buff size:%d !!!!!!!!!!!!!!!!!", size);
    }
    else
    {
        buff = buff_pool_->pop();
        if (!buff->data()) buff->alloc(4096*4);
    }
    memcpy(buff->data(), data, size);
    buff->set_offset(size);

    #ifdef DEBUG_GYRO_PTS
    uint32_t offset = 0;
    while (offset + sizeof(amba_gyro_data) <= size)
    {
        auto gyro = (amba_gyro_data*)(data+offset);
        if (gyro->pts <= last_pts_)
        {
            LOGERR("-----last pts:%ld cur pts:%ld offset:%d", last_pts_, gyro->pts, offset);
            abort();
        }
        offset += sizeof(amba_gyro_data);
        last_pts_ = gyro->pts;
    }
    #endif

    for (auto& it : sinks_)
    {
        it->queue_gyro(buff, delta_ts);
    }
}
