
#include "gyro_util.h"
#include "xml_config.h"
#include "stab_mgr.h"
#include "GyroStabilizer.h"
#include "Base.h"
#include "inslog.h"

void gyro_util::start(const std::string& path, bool b_stab, bool& b_rt_st, bool flag)
{
    int gyro_cfg = 1;
	xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_GYRO,gyro_cfg);
	if (gyro_cfg && flag) 
	{
        if (path != "") stab_mgr::get()->start_record(path);
		if (b_stab) 
		{
			stab_mgr::get()->start_realtime();
			b_rt_st = true;
		}
	}
}

void gyro_util::stop(bool& b_rt_st)
{
    if (b_rt_st) stab_mgr::get()->stop_realtime();
	stab_mgr::get()->stop_record();
    b_rt_st = false;
}

//in_mat:防抖算法库计算出的mat
//out_mat:拼接库使用的mat
//这个函数的作用是进行旋转矩阵转换
// std::shared_ptr<ins_mat_3d> gyro_util::left_multi_yaw_offset(float in_mat[9])
// {
//     ins::Matrix3x3d mat(in_mat[0], in_mat[1], in_mat[2], in_mat[3], in_mat[4], in_mat[5],in_mat[6], in_mat[7], in_mat[8]);

//     ins::Quaternion q0;
//     ins::Vector3d euler0(0, -INSTA_PI/6.0, 0);
//     ins::Quaternion::EulrAngle2Quat(&euler0, &q0);
//     ins::Matrix3x3d left_mat;
//     ins::Quaternion::Quat2Matrix(&q0, &left_mat);

//     ins::Matrix3x3d::mult(&left_mat, &mat, &mat); 

//     auto out_mat = std::make_shared<ins_mat_3d>();
//     memcpy(out_mat->m, mat.m, sizeof(out_mat->m));

//     return out_mat;
// }

// int gyro_util::calc_gpu_rotate_mat(const ins_gyro_info& info, float* mat) //INS_CARDBOARD_EKF INS_MAHONY_FILTER
// {
//     auto stabilizer = GYRO_CREATE_STABILIZER;
    
//     ins::Stabilizer::GyroData si_data; 
//     ins::Matrix3x3d so_data;
//     std::shared_ptr<ins_gyro_info> o_data;

//     ins::Quaternion q1;
//     ins::Vector3d euler1(INSTA_PI, -INSTA_PI/6.0, 0);
//     ins::Quaternion::EulrAngle2Quat(&euler1, &q1);
//     ins::Matrix3x3d left_mat;
//     ins::Quaternion::Quat2Matrix(&q1, &left_mat);

//     ins::Quaternion q2;
//     ins::Vector3d euler2(INSTA_PI, -INSTA_PI/6.0, 0);
//     ins::Quaternion::EulrAngle2Quat(&euler2, &q2);
//     ins::Matrix3x3d right_mat;
//     ins::Quaternion::Quat2Matrix(&q2, &right_mat);

//     si_data.gyro.x = 0.0;
//     si_data.gyro.y = 0.0;
//     si_data.gyro.z = 0.0;
//     si_data.accel.x = info.accel_x;
//     si_data.accel.y = info.accel_y;
//     si_data.accel.z = info.accel_z;
//     si_data.timestamp = 0.0;

//     stabilizer->inputGyroData(si_data);
//     stabilizer->getPredictedMatrix(&so_data, 0);

//     int gyro_gravatiy_cfg = 1;
//     xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_GYRO_GRAVITY, gyro_gravatiy_cfg);
//     if (gyro_gravatiy_cfg != 0)
//     {
//         ins::Matrix3x3d::mult(&gravity_mat_, &so_data, &so_data); //左乘
//     }
//     ins::Matrix3x3d::mult(&left_mat, &so_data, &so_data); //左乘
//     ins::Matrix3x3d::mult(&so_data, &right_mat, &so_data); //右乘

//     mat[0] = so_data.m[0];
//     mat[1] = so_data.m[3];
//     mat[2] = so_data.m[6];
//     mat[3] = 0;
//     mat[4] = so_data.m[1];
//     mat[5] = so_data.m[4];
//     mat[6] = so_data.m[7];
//     mat[7] = 0;
//     mat[8] = so_data.m[2];
//     mat[9] = so_data.m[5];
//     mat[10] = so_data.m[8];
//     mat[11] = 0;
//     mat[12] = 0;
//     mat[13] = 0;
//     mat[14] = 0;
//     mat[15] = 1;
//     return INS_OK;
// }

std::shared_ptr<ins_mat_3d> gyro_util::calc_cpu_rotate_mat(const ins_gyro_info* info, const ins_accel_data* accel_offset)
{
    auto stabilizer = std::make_shared<ins::Stabilizer::GyroStabilizer>(
                            ins::Stabilizer::GYROTYPE::INS_PRO_GYRO, 
                            ins::Stabilizer::FILTERTYPE::INS_CARDBOARD_EKF);
    
    ins::Stabilizer::GyroData si_data;
    ins::Matrix3x3d so_data;
    std::shared_ptr<ins_gyro_info> o_data;

    si_data.gyro.x = 0.0;
    si_data.gyro.y = 0.0;
    si_data.gyro.z = 0.0;
    si_data.accel.x = info->accel_x;
    si_data.accel.y = info->accel_y;
    si_data.accel.z = info->accel_z;
    si_data.timestamp = 0.0;

    stabilizer->inputGyroData(si_data);
    stabilizer->getPredictedMatrix(&so_data, 0);

    return trans_to_cpu_mat(so_data.m);
}

std::shared_ptr<ins_mat_3d> gyro_util::trans_to_cpu_mat(double* in_mat, const ins_accel_data* accel_offset)
{
    ins::Matrix3x3d mat(in_mat[0], in_mat[1], in_mat[2], in_mat[3], in_mat[4], in_mat[5],in_mat[6], in_mat[7], in_mat[8]);

    ins::Quaternion q0;
    ins::Vector3d euler0(0, -INSTA_PI/6.0, 0);
    ins::Quaternion::EulrAngle2Quat(&euler0, &q0);
    ins::Matrix3x3d left_mat;
    ins::Quaternion::Quat2Matrix(&q0, &left_mat);

    if (accel_offset)
    {
        //LOGINFO("gravity offset:%f %f %f", accel_offset->x, accel_offset->y, accel_offset->z);
        ins::Vector3d gravity_offset(accel_offset->x, accel_offset->y, accel_offset->z);
        ins::Vector3d gravity_flat(0, 0, 1);
        auto gravity_mat = ins::Stabilizer::GyroStabilizer::getInitCorrectionMatrix(gravity_flat, gravity_offset);
        ins::Matrix3x3d::mult(&gravity_mat, &mat, &mat); //左乘
    }

    ins::Matrix3x3d::mult(&left_mat, &mat, &mat); //左乘
    
    auto out_mat = std::make_shared<ins_mat_3d>();
    memcpy(out_mat->m, mat.m, sizeof(out_mat->m));

    return out_mat;
}