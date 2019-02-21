#ifndef _CAMM_UTIL_H_
#define _CAMM_UTIL_H_

#include <memory>
#include <vector>
#include "insbuff.h"
#include "metadata.h"

class camm_util
{
public:
    static int32_t gen_camm_packet(const ins_camm_data& camm, std::shared_ptr<insbuff>& buff);
    static int32_t gen_camm_gyro_packet(const ins_gyro_data& gyro, std::shared_ptr<insbuff>& buff);
    static int32_t gen_camm_accel_packet(const ins_accel_data& accel, std::shared_ptr<insbuff>& buff);
    static int32_t gen_camm_gps_packet(const ins_gps_data* gps, std::shared_ptr<insbuff>& buff);
    static int32_t gen_gyro_accel_packet(const amba_gyro_data* data, std::shared_ptr<insbuff>& buff);
    static int32_t gen_gyro_accel_mgm_packet(const amba_gyro_data* data, std::shared_ptr<insbuff>& buff);
    static int32_t gen_camm_exposure_packet(const exposure_times* exp, std::shared_ptr<insbuff>& buff);
    static const uint32_t gps_size;
    static const uint32_t gyro_size;
    static const uint32_t accel_size;
    static const uint32_t gyro_accel_size;
    static const uint32_t gyro_accel_mgm_size;
    static const uint32_t exposure_size;
    static const uint32_t camm_size;
};

#endif