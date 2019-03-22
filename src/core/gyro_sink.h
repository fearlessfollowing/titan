#ifndef _GYRO_SINK_H_
#define _GYRO_SINK_H_

#include <stdint.h>
#include <memory>
#include "insbuff.h"
#include "metadata.h"

class gyro_sink {
public:
    virtual 		~gyro_sink(){};
    virtual void 	queue_gyro(std::shared_ptr<insbuff>& data, int64_t delta_ts){};
	virtual void 	queue_exposure_time(std::shared_ptr<exposure_times>& data){};
};

#endif