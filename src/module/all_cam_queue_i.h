
#ifndef _ALL_CAM_QUEUE_INTERFACE_H_
#define _ALL_CAM_QUEUE_INTERFACE_H_

#include "sink_interface.h"
#include <memory>
#include <map>
#include "insbuff.h"

class all_cam_queue_i
{
public:
	virtual ~all_cam_queue_i(){};
	virtual void set_sps_pps(std::map<unsigned, std::shared_ptr<insbuff>>& sps, std::map<unsigned, std::shared_ptr<insbuff>>& pps) = 0;
	virtual void queue_frame(std::map<unsigned, std::shared_ptr<ins_frame>> frame) = 0;
	virtual bool is_sps_pps_set() = 0;
};

#endif