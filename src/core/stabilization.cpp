#include "stabilization.h"
#include <iostream>
#include "inslog.h"
#include "common.h"
#include "xml_config.h"
#include "Base.hpp"
#include "GyroStabilizer.hpp"
#include "SimpleFilter.hpp"
#include "Utils.hpp"
#include "metadata.h"
#include "camera_info.h"

static ins::Matrix3x3d gravity_mat_;
static ins::Matrix3x3d left_mat_;
static ins::Matrix3x3d right_mat_;

static ins::Matrix3x3d eulerAngle_to_rotation(ins::Vector3d& er)
{
    double pitch = er.x()/2.0;   
    double yaw = er.y()/2.0;
    double roll = er.z()/2.0;

    double sinp = sin(pitch);
    double siny = sin(yaw);
    double sinr = sin(roll);
    double cosp = cos(pitch);
    double cosy = cos(yaw);
    double cosr = cos(roll);

    ins::Quaternion quat;
    quat.x() = sinr * cosp * cosy - cosr * sinp * siny;
    quat.y() = cosr * sinp * cosy + sinr * cosp * siny;
    quat.z() = cosr * cosp * siny - sinr * sinp * cosy;
    quat.w() = cosr * cosp * cosy + sinr * sinp * siny;
    quat.normalize();

    double x2 = quat.x() * quat.x();
    double y2 = quat.y() * quat.y();
    double z2 = quat.z() * quat.z();
    double w2 = quat.w() * quat.w();
    double xy = quat.x() * quat.y();
    double xz = quat.x() * quat.z();
    double yz = quat.y() * quat.z();
    double wx = quat.w() * quat.x();
    double wy = quat.w() * quat.y();
    double wz = quat.w() * quat.z();

    ins::Matrix3x3d matrix;
    matrix << 1.0 - 2.0*(y2 + z2), 2.0*(xy + wz), 2.0*(xz - wy),
        2.0*(xy - wz), 1.0 - 2.0*(x2 + z2), 2.0*(yz + wx),
        2.0*(xz + wy), 2.0*(yz - wx), 1.0 - 2.0*(x2 + y2);

    return matrix;
}

static ins::Matrix3x3d getInitCorrectionMatrix(ins::Vector3d& accel_offset, ins::Vector3d& flat)
{
    ins::Matrix3x3d rot;
    ins::so3FromTwoVecN(flat, accel_offset, rot);
    return rot;
}

stabilization::stabilization(int32_t delay)
{
    delay_ = delay; //78000; //140000;
    ins::Vector3d euler_left;
    ins::Vector3d euler_right;
    ins::Vector3d gravity_flat;

    auto orientation = 3; // camera_info::get_gyro_orientation();
    if (orientation == INS_GYRO_ORIENTATION_HORIZON) //横放
    {
        euler_left << ins::INSTA_PI, ins::INSTA_PI/2.0, 0;
        euler_right << ins::INSTA_PI, ins::INSTA_PI/2.0, 0;
        gravity_flat << 0, 0, 1;
    }
    else if (orientation == INS_GYRO_ORIENTATION_VERTICAL)
    {
        euler_left << ins::INSTA_PI/2.0, -ins::INSTA_PI/2.0, 0;
        euler_right << ins::INSTA_PI, -ins::INSTA_PI/2.0, 0;
        gravity_flat << 0, -1, 0;
    }
    else
    {
        euler_left << ins::INSTA_PI/2.0, ins::INSTA_PI/2.0, 0;
        euler_right << ins::INSTA_PI, ins::INSTA_PI/2.0, 0;
        gravity_flat << 0, 1, 0;
    }

    left_mat_ = eulerAngle_to_rotation(euler_left);
    right_mat_ = eulerAngle_to_rotation(euler_right);

    double x, y, z;
    xml_config::get_accel_offset(x, y, z);
    ins::Vector3d gravity_offset(x, y, z);
    gravity_mat_ = getInitCorrectionMatrix(gravity_offset, gravity_flat);

    stabilizer_ = std::make_unique<ins::Stabilizer::GyroStabilizer>(ins::Stabilizer::GYROTYPE::INS_PRO_GYRO, ins::Stabilizer::FILTERTYPE::INS_AKF_FILTER);
    stabilizer_->setCacheSize(2500);

    motionFilter_ = std::make_unique<ins::FootageMotionFilter>();

    LOGINFO("orientation:%d type:%d delay:%d accel offset:%lf %lf %lf", 
        orientation, 
        camera_info::get_stablz_type(), 
        delay,
        x, y, z);

    th_ = std::thread(&stabilization::task, this);
}

stabilization::~stabilization()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

void stabilization::task()
{
    LOGINFO("-------stabilization task run");

    bool flag = true;
    ins::Stabilizer::GyroData si_data;
    while(!quit_)
    {
        auto pkt = deque_gyro();
        if (pkt == nullptr)
        {
            usleep(10*1000);
            continue;
        }

        uint8_t* p = pkt->data();
        uint32_t offset = 0;
        int64_t pts = 0;
        
        while (offset + sizeof(amba_gyro_data) <= pkt->offset())
        {
            auto frame = (amba_gyro_data*)(p+offset);
            pts = frame->pts - delta_ts_ - delay_;
            offset += sizeof(amba_gyro_data);

            si_data.timestamp = (double)pts/1000000.0;
            si_data.gyro << frame->gyro[0], frame->gyro[1], frame->gyro[2];
            si_data.accel << frame->accel[0], frame->accel[1], frame->accel[2];

            // printf("gyro:%lf %lf %lf accel:%lf %lf %lf\n",  
            //     frame->gyro[0], 
            //     frame->gyro[1], 
            //     frame->gyro[2],
            //     frame->accel[0],
            //     frame->accel[1],
            //     frame->accel[2]);

            mtx_s_.lock();
            stabilizer_->inputGyroData(si_data);
            mtx_s_.unlock();
        }
    }

    LOGINFO("-------stabilization task exit");
}

uint32_t stabilization::get_rotation_mat(long long pts, ins_mat_4f& out_mat)
{
    ins::Matrix3x3d mat;
    mtx_s_.lock();
    auto ret = stabilizer_->getSmoothedMatrix(&mat, (double)pts/1000000.0);
    mtx_s_.unlock();

    if (ret) 
    {
        LOGERR("getSmoothedMatrix fail:%d", ret);
        return INS_ERR;
    } 
    
    if (camera_info::get_stablz_type() == INS_STABLZ_TYPE_Z)
    {
        auto footageMotion = motionFilter_->getFootageMotion(mat, ins::Stabilizer::STABTYPE::INS_STAB_Z_DIRECTIONAL);
        mat = mat*footageMotion;
    }

    mat = gravity_mat_*mat;
    mat = left_mat_*mat;
    mat = mat*right_mat_;

    //转置
    out_mat.m[0] = mat(0, 0);
    out_mat.m[1] = mat(1, 0);
    out_mat.m[2] = mat(2, 0);
    out_mat.m[3] = 0;
    out_mat.m[4] = mat(0, 1);
    out_mat.m[5] = mat(1, 1);
    out_mat.m[6] = mat(2, 1);
    out_mat.m[7] = 0;
    out_mat.m[8] = mat(0, 2);
    out_mat.m[9] = mat(1, 2);
    out_mat.m[10] = mat(2, 2);
    out_mat.m[11] = 0;
    out_mat.m[12] = 0;
    out_mat.m[13] = 0;
    out_mat.m[14] = 0;
    out_mat.m[15] = 1;

    // LOGINFO("get pts:%ld mat:%lf %lf %lf %lf %lf %lf %lf %lf %lf", pts, 
    //     out_mat.m[0], 
    //     out_mat.m[1],
    //     out_mat.m[2],
    //     out_mat.m[3], 
    //     out_mat.m[4],
    //     out_mat.m[5],
    //     out_mat.m[6], 
    //     out_mat.m[7],
    //     out_mat.m[8]);

    return INS_OK;
}

uint32_t stabilization::get_cpu_rotation_mat(long long pts, ins_mat_3d& out_mat)
{
    ins::Vector3d euler;
    auto orientation = 3;//camera_info::get_gyro_orientation();
    if (orientation == INS_GYRO_ORIENTATION_HORIZON)
    {
        euler << 0, ins::INSTA_PI/2.0, 0;
    }
    else if (orientation == INS_GYRO_ORIENTATION_VERTICAL)
    {
        euler << -ins::INSTA_PI/2.0, -ins::INSTA_PI/2.0, 0;
    }
    else 
    {
        euler << -ins::INSTA_PI/2.0, ins::INSTA_PI/2.0, 0;
    }

    ins::Matrix3x3d mat;
    mtx_s_.lock();
    stabilizer_->getSmoothedMatrix(&mat, (double)pts/1000000.0);
    mtx_s_.unlock();

    if (camera_info::get_stablz_type() == INS_STABLZ_TYPE_Z)
    {
        auto footageMotion = motionFilter_->getFootageMotion(mat, ins::Stabilizer::STABTYPE::INS_STAB_Z_DIRECTIONAL);
        mat = mat*footageMotion;
    }

    mat = gravity_mat_*mat;
    ins::Matrix3x3d left_mat = eulerAngle_to_rotation(euler);
    mat = left_mat*mat;

    out_mat.m[0]= mat(0, 0);
    out_mat.m[1]= mat(0, 1);
    out_mat.m[2]= mat(0, 2);
    out_mat.m[3]= mat(1, 0); 
    out_mat.m[4]= mat(1, 1);
    out_mat.m[5]= mat(1, 2);
    out_mat.m[6]= mat(2, 0); 
    out_mat.m[7]= mat(2, 1);
    out_mat.m[8]= mat(2, 2);

    return INS_OK;
}

void stabilization::queue_gyro(std::shared_ptr<insbuff>& data, int64_t delta_ts)
{
    std::lock_guard<std::mutex> lock(mtx_);
    delta_ts_ = delta_ts;
    queue_.push(data);
}

std::shared_ptr<insbuff> stabilization::deque_gyro()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return nullptr;
    auto pkt = queue_.front();
    queue_.pop();

    return pkt;
}



