#ifndef _INS_I2C_H_
#define _INS_I2C_H_

#include <mutex>
#include <stdint.h>

class ins_i2c
{
public:
    ~ins_i2c();
    int32_t open(uint32_t adapter, uint32_t addr, bool force = true); // 0/0x77/true
    int32_t write(uint8_t reg, uint8_t value);
    int32_t read(uint8_t reg, uint8_t& value);//0x02 

private:
    int32_t fd_ = -1;
    std::mutex mtx_;
};
#endif 