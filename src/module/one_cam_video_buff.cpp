
#include "one_cam_video_buff.h"
#include "inslog.h"

void one_cam_video_buff::set_sps_pps(unsigned int index, const std::shared_ptr<insbuff>& sps, const std::shared_ptr<insbuff>& pps)
{
    std::lock_guard<std::mutex> lock(mtx_);
    sps_ = sps;
    pps_ = pps;
}

void one_cam_video_buff::queue_frame(unsigned int index, std::shared_ptr<ins_frame> frame)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.size() > 20 && frame->is_key_frame)
    {
        LOGINFO("one cam queue discard 10 frames");
        while (queue_.size() > 10) queue_.pop_back();
    }
    queue_.push_back(frame);
}

std::shared_ptr<insbuff> one_cam_video_buff::get_sps()
{
    while (1)
    {
        mtx_.lock();
        if (sps_ != nullptr)
        {
            mtx_.unlock();
            break;
        }
        else
        {
            mtx_.unlock();
            usleep(10000);
            continue;
        }
    }

    return sps_;
}

std::shared_ptr<insbuff> one_cam_video_buff::get_pps()
{
    while (1)
    {
        mtx_.lock();
        if (pps_ != nullptr)
        {
            mtx_.unlock();
            break;
        }
        else
        {
            mtx_.unlock();
            usleep(10000);
            continue;
        }
    }

    return pps_;

}

std::shared_ptr<ins_frame> one_cam_video_buff::deque_frame()
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (queue_.empty()) return nullptr;
    auto frame = queue_.front();
    queue_.pop_front();
    return frame;
}