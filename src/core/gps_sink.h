#ifndef _GPS_SINK_H_
#define _GPS_SINK_H_

#include "metadata.h"

class gps_sink {
public:
    virtual 		~gps_sink(){};
    virtual void 	queue_gps(std::shared_ptr<ins_gps_data>& gps) = 0;
};

#endif