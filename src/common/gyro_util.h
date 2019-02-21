
#ifndef _GYRO_UTIL_H_
#define _GYRO_UTIL_H_

#include <string>
#include <memory>
#include "metadata.h"
#include "ins_mat.h"

class gyro_util
{
public:
    static void start(const std::string& path, bool b_stab, bool& b_rt_st, bool flag = true);
    static void stop(bool& b_rt_st);
    // static int calc_gpu_rotate_mat(const ins_gyro_info& info, float* mat);
    static std::shared_ptr<ins_mat_3d> calc_cpu_rotate_mat(const ins_gyro_info* info, const ins_accel_data* accel_offset = nullptr);
    static std::shared_ptr<ins_mat_3d> trans_to_cpu_mat(double* in_mat, const ins_accel_data* accel_offset = nullptr);
    //static std::shared_ptr<ins_mat_3d> left_multi_yaw_offset(float in_mat[9]);
};

#endif