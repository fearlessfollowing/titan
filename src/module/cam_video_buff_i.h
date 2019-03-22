#ifndef _CAM_VIDEO_BUFF_INTERFACE_H_
#define _CAM_VIDEO_BUFF_INTERFACE_H_

#include "sink_interface.h"
#include <memory>
#include <stdint.h>

class cam_video_buff_i {
public:
	virtual 		~cam_video_buff_i(){};
    virtual void 	set_sps_pps(uint32_t index, const std::shared_ptr<insbuff>& sps, const std::shared_ptr<insbuff>& pps) = 0;
	virtual void 	queue_frame(uint32_t index, std::shared_ptr<ins_frame> frame) = 0;
	virtual void 	queue_gyro(const uint8_t* data, uint32_t size, uint64_t delta_ts) {};
	virtual void 	queue_expouse(uint32_t pid, uint32_t seq, uint64_t ts, uint64_t exposure_ns) {};
	virtual void 	set_first_frame_ts(uint32_t pid, int64_t timestamp) {};
};

#endif