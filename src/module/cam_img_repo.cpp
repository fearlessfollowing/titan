
#include "cam_img_repo.h"
#include "access_msg_center.h"
#include "json_obj.h"
#include "gps_mgr.h"
#include "inslog.h"
#include "pic_seq_sink.h"
#include "stream_sink.h"


void cam_img_repo::queue_frame(uint32_t index, const std::shared_ptr<ins_frame>& frame)
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (frame->metadata.raw) {   // raw/jpeg队列分开
        do_queue_frame(index, frame, single_queue_raw_);
    } else {
        do_queue_frame(index, frame, single_queue_);
    }
}

void cam_img_repo::do_queue_frame(uint32_t index, 
                                    const std::shared_ptr<ins_frame>& frame, 
                                    std::map<uint32_t, std::map<uint32_t, std::shared_ptr<ins_frame>>>& queue)
{
    auto it = queue.find(index);
    if (it == queue.end())
    {
        std::map<uint32_t, std::shared_ptr<ins_frame>> q;  
        queue.insert(std::make_pair(index, q));
        it = queue.find(index);
    }

    it->second.insert(std::make_pair(frame->sequence, frame));
    if (queue.size() != INS_CAM_NUM) return;

    while (1)
    {
        //每一个都是按sequence升序排列的，只要比较begin
        int32_t seq = -1;
        for (auto& it : queue)
        {
            if (it.second.empty()) return;
            if (seq == -1) 
            {
                seq = it.second.begin()->second->sequence;
            }
            if (seq != it.second.begin()->second->sequence) return;
        }

        //取出第一个帧
        std::map<uint32_t, std::shared_ptr<ins_frame>> sync_queue;
        for (auto& it : queue)
        {
            auto front_item = it.second.begin();
            sync_queue.insert(std::make_pair(it.first, front_item->second));
            it.second.erase(front_item);
        }
        out_sync_queue(sync_queue);
    }
}


void cam_img_repo::out_sync_queue(std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame)
{
    acquire_gps_info(m_frame);

    if (type_ == INS_PIC_TYPE_TIMELAPSE)
    {
        output(m_frame); //timelapse是主动推到sink
    }
    else
    {
        queue_.push_back(m_frame); //非timelapse存储着，等调用者自己获取
    }
}

std::map<uint32_t, std::shared_ptr<ins_frame>> cam_img_repo::dequeue_frame()
{
    std::lock_guard<std::mutex> lock(mtx_);
    std::map<uint32_t, std::shared_ptr<ins_frame>> group;

    if (!queue_.empty()) {
        group = queue_.front();
        queue_.pop_front();
    }
    return group;
}

void cam_img_repo::output(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame)
{
    for (auto it = sink_.begin(); it != sink_.end(); it++) {
        auto f_it = m_frame.find(it->first);
        if (f_it != m_frame.end()) {
            it->second->queue_frame(f_it->second);
        }
    }
}

void cam_img_repo::acquire_gps_info(std::map<uint32_t, std::shared_ptr<ins_frame>>& group)
{   
    auto gps = gps_mgr::get()->get_gps(group.begin()->second->pts);
    if (gps) {
        for (auto& it : group) {
            it.second->metadata.b_gps = true;
            it.second->metadata.gps = *(gps.get());
        }
    }
}

void cam_img_repo::queue_gyro(const uint8_t* data, uint32_t size)
{
    if (gyro_sinks_.empty()) return;
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
    for (auto& it : gyro_sinks_)
    {
        it->queue_gyro(buff, 0);
    }
}