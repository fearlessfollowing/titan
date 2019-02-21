#ifndef _CAM_IMG_REPO_H_
#define _CAM_IMG_REPO_H_

#include <stdint.h>
#include <map>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include "ins_frame.h"
#include "common.h"
#include "insbuff.h"
#include "obj_pool.h"

class pic_seq_sink;
class stream_sink;
class gyro_sink;

class cam_img_repo
{
public:
    cam_img_repo() {};
    cam_img_repo(std::string type) : type_(type) {};
    void queue_frame(uint32_t index, const std::shared_ptr<ins_frame>& frame);
    void queue_gyro(const uint8_t* data, uint32_t size);
    std::map<uint32_t, std::shared_ptr<ins_frame>> dequeue_frame();
    void set_sink(std::map<uint32_t,std::shared_ptr<pic_seq_sink>>& sinks) { sink_ = sinks; };
    void add_gyro_sink(std::shared_ptr<gyro_sink>& sink)
    {
        if (!buff_pool_) buff_pool_ = std::make_shared<obj_pool<insbuff>>(-1, "gyro buff");
        gyro_sinks_.push_back(sink);
    }

private:
    void output(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame);
    void acquire_gps_info(std::map<uint32_t, std::shared_ptr<ins_frame>>& group);
    void out_sync_queue(std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame);
    void do_queue_frame(uint32_t index, 
        const std::shared_ptr<ins_frame>& frame, 
        std::map<uint32_t, std::map<uint32_t, std::shared_ptr<ins_frame>>>& queue);

    std::map<uint32_t, std::map<uint32_t, std::shared_ptr<ins_frame>>> single_queue_;
    std::map<uint32_t, std::map<uint32_t, std::shared_ptr<ins_frame>>> single_queue_raw_;
    std::deque<std::map<uint32_t, std::shared_ptr<ins_frame>>> queue_; //non timelapse 
    std::map<uint32_t, std::shared_ptr<pic_seq_sink>> sink_; //timelapse
    std::mutex mtx_;
    std::string type_ = INS_PIC_TYPE_PHOTO;

    std::shared_ptr<obj_pool<insbuff>> buff_pool_;
    std::vector<std::shared_ptr<gyro_sink>> gyro_sinks_;
};

#endif