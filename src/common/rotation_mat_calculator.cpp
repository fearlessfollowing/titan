#include "rotation_mat_calculator.h"
//#include "Base.h"
#include "inslog.h"
#include "common.h"
//#include "gyro_util.h"

int rotation_mat_calculator::open(const std::string& filename, std::shared_ptr<ins_accel_data>& accel_offset)
{
    // int ret = data_parser_.open(filename);
    // RETURN_IF_NOT_OK(ret);

    // stabilizer_ = std::make_shared<ins::Stabilizer::GyroStabilizer>(
    //     ins::Stabilizer::GYROTYPE::INS_PRO_GYRO, 
    //     ins::Stabilizer::FILTERTYPE::INS_AKF_FILTER);
    // stabilizer_->setCacheSize(150);
    // stabilizer_->setLowpassFilter(250, 250 * 0.08, 21);
    // stabilizer_->setUsePreFilter(true);
    // stabilizer_->setSearchType(false);

    // accel_offset_ = accel_offset;
    
    return INS_OK;
}

void rotation_mat_calculator::set_start_timestamp(double timestamp)
{
    // start_timestamp_ = timestamp;
    // LOGINFO("gyro set start timestamp:%lf", start_timestamp_);
}

std::shared_ptr<ins_mat_3d> rotation_mat_calculator::get_rotation_mat(double timestamp)
{
    // while (1)
    // {
    //     if (stabilizer_->getLastTimeStamp() > timestamp+0.2) break;
    //     if (feed_a_gyro_frame() != INS_OK) break;
    // }

    // if (stabilizer_->getLastTimeStamp() < timestamp)
    // {
    //     LOGINFO("timestamp:%lf cann't get gyro", timestamp);
    //     //return nullptr;
    // }

    // //LOGINFO("timestamp:%lf last:%lf", timestamp, stabilizer_->getLastTimeStamp());

    // ins::Matrix3x3d so_data;
    // stabilizer_->getSmoothedMatrix(&so_data, timestamp);
    // return gyro_util::trans_to_cpu_mat(so_data.m, accel_offset_.get());

    return nullptr;
}

int rotation_mat_calculator::feed_a_gyro_frame()
{
    // ins_g_a_data f_data;
    // int ret = data_parser_.get_next(f_data);
    // RETURN_IF_NOT_OK(ret);
    
    // ins::Stabilizer::GyroData si_data;
    // si_data.gyro.x = f_data.gyro_x;
    // si_data.gyro.y = f_data.gyro_y;
    // si_data.gyro.z = f_data.gyro_z;
    // si_data.accel.x = f_data.accel_x;
    // si_data.accel.y = f_data.accel_y;
    // si_data.accel.z = f_data.accel_z;
    // si_data.timestamp = (double)f_data.pts/1000000.0;

    // if (si_data.timestamp < start_timestamp_ - 60.0)
    // {
    //     //LOGINFO("gyro data timestamp:%lf < start timestamp:%lf 60s drop\n", si_data.timestamp, start_timestamp_);
    //     return INS_OK;
    // }

    // //printf("---feed time:%lf\n", si_data.timestamp);

    // stabilizer_->inputGyroData(si_data);

    return INS_OK;
}

void rotation_mat_calculator::close_file()
{
    data_parser_.close();
}

int rotation_mat_calculator::reopen_file()
{
    return data_parser_.reopen();
}