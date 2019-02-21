#ifndef _INS_BATTERY_H_
#define _INS_BATTERY_H_

#include <stdint.h>
#include "ins_i2c.h"

class ins_battery
{
public:
    //int32_t open();
    int32_t read_temp(double& temp);
private:
    int32_t read_value(ins_i2c& i2c, int32_t type, int16_t& val);
    double convert_k_to_c(int16_t k);
    //ins_i2c i2c_;
};

#endif