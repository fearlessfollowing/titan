#ifndef _ONE_CAM_VIDEO_BUFF_H_
#define _ONE_CAM_VIDEO_BUFF_H_

#include "cam_video_buff_i.h"
#include <deque>
#include <mutex>
#include "insbuff.h"

class one_cam_video_buff : public cam_video_buff_i
{
public:
	virtual ~one_cam_video_buff(){};
    virtual void set_sps_pps(unsigned int index, const std::shared_ptr<insbuff>& sps, const std::shared_ptr<insbuff>& pps) override;
	virtual void queue_frame(unsigned int index, std::shared_ptr<ins_frame> frame) override;
    std::shared_ptr<insbuff> get_sps();
	std::shared_ptr<insbuff> get_pps();
	std::shared_ptr<ins_frame> deque_frame();

private:
    std::deque<std::shared_ptr<ins_frame>> queue_;
    std::shared_ptr<insbuff> sps_;
    std::shared_ptr<insbuff> pps_;
    std::mutex mtx_;
};

#endif