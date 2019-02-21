#include "gyro_mgr.h"
#include <iostream>
#include "inslog.h"
#include "common.h"
#include "xml_config.h"
#include <fcntl.h>
#include "Base.h"
#include "cam_manager.h"

ins::Matrix3x3d gravity_mat_;
ins::Matrix3x3d left_mat_;
ins::Matrix3x3d right_mat_;

#define _GYRO_CALIBRATION_CNT_ 1500
#define _GYRO_FILE_BUFF_SIZE_ 4096*8

#define GYRO_CREATE_STABILIZER \
std::make_shared<ins::Stabilizer::GyroStabilizer>(ins::Stabilizer::GYROTYPE::INS_PRO_GYRO, ins::Stabilizer::FILTERTYPE::INS_AKF_FILTER);\
stabilizer_->setCacheSize(1400);

gyro_mgr::gyro_mgr()
{
    th_ = std::thread(&gyro_mgr::task, this);
}

void gyro_mgr::release()
{
    stop_realtime();
    stop_record();
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

void gyro_mgr::re_init_stab()
{
    mtx_s_.lock();
    stabilizer_ = nullptr;
    stabilizer_ = GYRO_CREATE_STABILIZER;

    if (stabilizer_ == nullptr)
    {
        LOGINFO("gyro_mgr::re_init_stab fail");
    }
    else
    {
        LOGINFO("gyro_mgr::re_init_stab success");
    }
    mtx_s_.unlock();
}

void gyro_mgr::init_mat()
{
    ins::Quaternion q1;
    ins::Vector3d euler1(INSTA_PI, -INSTA_PI/6.0, 0);
    ins::Quaternion::EulrAngle2Quat(&euler1, &q1);
    //ins::Matrix3x3d left_mat;
    ins::Quaternion::Quat2Matrix(&q1, &left_mat_);

    ins::Quaternion q2;
    ins::Vector3d euler2(INSTA_PI, -INSTA_PI/6.0, 0);
    ins::Quaternion::EulrAngle2Quat(&euler2, &q2);
    //ins::Matrix3x3d right_mat;
    ins::Quaternion::Quat2Matrix(&q2, &right_mat_);

    xml_config::get_value(INS_CONFIG_GYRO, INS_CONFIG_GYRO_X_OFFSET, gyro_x_offset_);
    xml_config::get_value(INS_CONFIG_GYRO, INS_CONFIG_GYRO_Y_OFFSET, gyro_y_offset_);
    xml_config::get_value(INS_CONFIG_GYRO, INS_CONFIG_GYRO_Z_OFFSET, gyro_z_offset_);
    xml_config::get_value(INS_CONFIG_GYRO, INS_CONFIG_ACCEL_X_OFFSET, accel_x_offset_);
    xml_config::get_value(INS_CONFIG_GYRO, INS_CONFIG_ACCEL_Y_OFFSET, accel_y_offset_);
    xml_config::get_value(INS_CONFIG_GYRO, INS_CONFIG_ACCEL_Z_OFFSET, accel_z_offset_);

    // int gyro_gravatiy_cfg = 1;
    // xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_GYRO_GRAVITY, gyro_gravatiy_cfg);
    // if (gyro_gravatiy_cfg != 0)
    // {
    //     LOGINFO("gyro gravity calibration on");
    // }

    ins::Vector3d gravity_offset(accel_x_offset_, accel_y_offset_, accel_z_offset_);
    ins::Vector3d gravity_flat(0, 0, 1);
    
    gravity_mat_ = ins::Stabilizer::GyroStabilizer::getInitCorrectionMatrix(gravity_flat, gravity_offset);

    LOGINFO("gyro offset x:%lf y:%lf z:%lf accel offset x:%lf y:%lf z:%lf", 
        gyro_x_offset_, 
        gyro_y_offset_, 
        gyro_z_offset_, 
        accel_x_offset_, 
        accel_y_offset_, 
        accel_z_offset_);
}

void gyro_mgr::task()
{
    gyro_reader_ = std::make_shared<gyro_wrapper>();

    mtx_s_.lock();
    stabilizer_ = GYRO_CREATE_STABILIZER;
    mtx_s_.unlock();

    GYRO_DAT r_data;
    ins::Stabilizer::GyroData si_data;

    init_mat();
    
    while(!quit_)
    {
        if(gyro_reader_->read_dat(&r_data))  continue;

        struct timeval tm;
        gettimeofday(&tm, nullptr);
        long long pts = tm.tv_sec*1000000 + tm.tv_usec;
        long long pts_amba = pts - cam_manager::nv_amba_delta_usec();

        if (b_calibration) queue_calibration(r_data);

        si_data.gyro.x = (r_data.gyro_x - gyro_x_offset_)/180.0*3.14159;
        si_data.gyro.y = (r_data.gyro_y - gyro_y_offset_)/180.0*3.14159;
        si_data.gyro.z = (r_data.gyro_z - gyro_z_offset_)/180.0*3.14159;
        si_data.accel.x = r_data.acc_x;
        si_data.accel.y = r_data.acc_y;
        si_data.accel.z = r_data.acc_z;
        si_data.timestamp = (double)pts_amba/1000000.0;

        mtx_s_.lock();
        stabilizer_->inputGyroData(si_data);
        //stabilizer_->getPredictedMatrix(&so_data, 0);
        mtx_s_.unlock();

        ins_g_a_data g_a;
        g_a.pts = pts_amba;
        g_a.gyro_x = si_data.gyro.x;
        g_a.gyro_y = si_data.gyro.y;
        g_a.gyro_z = si_data.gyro.z;
        g_a.accel_x = si_data.accel.x;
        g_a.accel_y = si_data.accel.y;
        g_a.accel_z = si_data.accel.z;

        queue_realtime_gyro(g_a);
        queue_rec_gyro(g_a);

        usleep(2000); //500HZ
    }

    mtx_s_.lock();
    stabilizer_ = nullptr;
    mtx_s_.unlock();

    LOGINFO("gyro read task over");
}

void gyro_mgr::queue_calibration(const GYRO_DAT& gyro_data)
{
    if (calibration_cnt_ == -1)
    {
        //for driver calibration, not use now
        calibration_cnt_ = 0;
        return;
    }

    printf("%lf %lf %lf %lf %lf %lf\n", 
        gyro_data.gyro_x,
        gyro_data.gyro_y,
        gyro_data.gyro_z,
        gyro_data.acc_x,
        gyro_data.acc_y,
        gyro_data.acc_z);

    gyro_x_ += gyro_data.gyro_x;
    gyro_y_ += gyro_data.gyro_y;
    gyro_z_ += gyro_data.gyro_z;
    accel_x_ += gyro_data.acc_x;
    accel_y_ += gyro_data.acc_y;
    accel_z_ += gyro_data.acc_z;

    if (++calibration_cnt_ >= _GYRO_CALIBRATION_CNT_)
    {
        b_calibration = false;
        gyro_x_ = gyro_x_/_GYRO_CALIBRATION_CNT_;
        gyro_y_ = gyro_y_/_GYRO_CALIBRATION_CNT_;
        gyro_z_ = gyro_z_/_GYRO_CALIBRATION_CNT_;
        accel_x_ = accel_x_/_GYRO_CALIBRATION_CNT_;
        accel_y_ = accel_y_/_GYRO_CALIBRATION_CNT_;
        accel_z_ = accel_z_/_GYRO_CALIBRATION_CNT_;

        gyro_x_offset_ = gyro_x_;
        gyro_y_offset_ = gyro_y_;
        gyro_z_offset_ = gyro_z_;
        accel_x_offset_ = accel_x_;
        accel_y_offset_ = accel_y_;
        accel_z_offset_ = accel_z_;

        ins::Vector3d gravity_offset(accel_x_offset_, accel_y_offset_, accel_z_offset_);
        ins::Vector3d gravity_flat(0, 0, 1);
        gravity_mat_ = ins::Stabilizer::GyroStabilizer::getInitCorrectionMatrix(gravity_flat, gravity_offset);
    }
}

int gyro_mgr::calibration()
{
    LOGINFO("gyro calibration begin");

	gyro_x_ = 0.0;
	gyro_y_ = 0.0;
	gyro_z_ = 0.0;
	accel_x_ = 0.0;
	accel_y_ = 0.0;
	accel_z_ = 0.0;
    calibration_cnt_ = -1;
    b_calibration = true;

    while (b_calibration)
    {
        usleep(20*1000);
    }

	xml_config::set_value(INS_CONFIG_GYRO, INS_CONFIG_GYRO_X_OFFSET, gyro_x_);
	xml_config::set_value(INS_CONFIG_GYRO, INS_CONFIG_GYRO_Y_OFFSET, gyro_y_);
	xml_config::set_value(INS_CONFIG_GYRO, INS_CONFIG_GYRO_Z_OFFSET, gyro_z_);
	xml_config::set_value(INS_CONFIG_GYRO, INS_CONFIG_ACCEL_X_OFFSET, accel_x_);
	xml_config::set_value(INS_CONFIG_GYRO, INS_CONFIG_ACCEL_Y_OFFSET, accel_y_);
	xml_config::set_value(INS_CONFIG_GYRO, INS_CONFIG_ACCEL_Z_OFFSET, accel_z_);

    LOGINFO("gyro calibration result: gyro_x:%lf gyro_y:%lf  gyro_z:%lf accel_x:%lf accel_y:%lf accel_z:%lf", 
		gyro_x_, 
        gyro_y_, 
        gyro_z_, 
        accel_x_, 
        accel_y_, 
        accel_z_);

    gyro_x_ = 0.0;
	gyro_y_ = 0.0;
	gyro_z_ = 0.0;
	accel_x_ = 0.0;
	accel_y_ = 0.0;
	accel_z_ = 0.0;
    calibration_cnt_ = -1;

    return INS_OK;
}

int gyro_mgr::start_record(std::string file_name)
{
    INS_ASSERT(!b_record_);
    
    b_record_ = true;
    std::string tmp = file_name + "/gyro.dat";
    fd_ = ::open(tmp.c_str(), O_WRONLY|O_CREAT, 0666);
    if (fd_ < 0)
    {
        LOGERR("file:%s open fail", tmp.c_str());
        return INS_ERR_FILE_OPEN;
    }

    LOGINFO("------gyro rec begin size:%lu", rt_queue_.size());

    mtx_.lock();
    for (auto& data : rt_queue_)
    {
        queue_rec_gyro(data);
    }
    mtx_.unlock();

    LOGINFO("------gyro rec end");

    th_rec_ = std::thread(&gyro_mgr::rec_task, this);

    LOGINFO("gyro record start");

    return INS_OK;
}

int gyro_mgr::stop_record()
{
    if (!b_record_) return INS_OK; 

    mtx_rec_.lock();
    sink_.clear();
    mtx_rec_.unlock();

    b_record_ = false;
    INS_THREAD_JOIN(th_rec_);

    if (fd_ > 0)
    {
        ::close(fd_);
        fd_ = -1;
    }

    LOGINFO("gyro record stop");

    return INS_OK;
}

void gyro_mgr::add_sink(const std::shared_ptr<sink_interface> sink)
{
    std::lock_guard<std::mutex> lock(mtx_rec_);
    sink_.push_back(sink);
}

int gyro_mgr::start_realtime()
{
    if (b_realtime_) return INS_OK;

    b_realtime_ = true;
    LOGINFO("gyro realtime start");
    return INS_OK;
}

int gyro_mgr::stop_realtime()
{
    if (!b_realtime_) return INS_OK;

    b_realtime_ = false;
    //rt_queue_.clear();
    LOGINFO("gyro realtime stop");
    return INS_OK;
}

int gyro_mgr::start_euler_angle()
{
    if (b_euler_angle_) return INS_OK;

    b_euler_angle_ = true;
    LOGINFO("gyro euler angle start");
    return INS_OK;
}

int gyro_mgr::stop_euler_angle()
{
    if (!b_euler_angle_) return INS_OK;

    b_euler_angle_ = false;
    LOGINFO("gyro euler angle stop");
    return INS_OK;
}

std::shared_ptr<ins_gyro_info> gyro_mgr::get_gyro_info(long long pts)
{
    if (!b_realtime_) return nullptr; 

    //从队列获取原始数据陀螺仪数据
    bool success = false;
    ins_g_a_data origin_data;
    mtx_.lock();
    for (auto it = rt_queue_.rbegin(); it != rt_queue_.rend(); it++)
    {
        if ((*it).pts < pts)
        {
            if (it != rt_queue_.rbegin())
            {
                success = true;
                origin_data = *(--it);
            }
            break;
        }
    }
    mtx_.unlock();

    if (!success)
    {
        LOGERR("timestamp:%lld get rt rotation mat, but origin gyro not fond", pts);
        return nullptr;
    }

    //从算法库获取旋转矩阵
    ins::Matrix3x3d mat;
    mtx_s_.lock();
    if (!stabilizer_)
    {
        LOGERR("timestamp:%lld get rt rotation mat, but stabilizer is null", pts);
        return nullptr;
    }
    stabilizer_->getSmoothedMatrix(&mat, (double)pts/1000000.0);
    mtx_s_.unlock();

    //旋转矩阵进行转换
    auto o_data = std::make_shared<ins_gyro_info>();

    ins::Matrix3x3d::mult(&gravity_mat_, &mat, &mat); //重力加速度校准

    memcpy(o_data->raw_mat, mat.m, sizeof(o_data->raw_mat));

    ins::Matrix3x3d::mult(&left_mat_, &mat, &mat); //左乘
    ins::Matrix3x3d::mult(&mat, &right_mat_, &mat); //右乘

    //转置
    o_data->pts = origin_data.pts;
    o_data->mat[0] = mat.m[0];
    o_data->mat[1] = mat.m[3];
    o_data->mat[2] = mat.m[6];
    o_data->mat[3] = 0;
    o_data->mat[4] = mat.m[1];
    o_data->mat[5] = mat.m[4];
    o_data->mat[6] = mat.m[7];
    o_data->mat[7] = 0;
    o_data->mat[8] = mat.m[2];
    o_data->mat[9] = mat.m[5];
    o_data->mat[10] = mat.m[8];
    o_data->mat[11] = 0;
    o_data->mat[12] = 0;
    o_data->mat[13] = 0;
    o_data->mat[14] = 0;
    o_data->mat[15] = 1;
    o_data->gyro_x = origin_data.gyro_x;
    o_data->gyro_y = origin_data.gyro_y;
    o_data->gyro_z = origin_data.gyro_z;
    o_data->accel_x = origin_data.accel_x;
    o_data->accel_y = origin_data.accel_y;
    o_data->accel_z = origin_data.accel_z;

    return o_data;
}

// std::shared_ptr<ins_gyro_info> gyro_mgr::get_gyro_info(long long pts)
// {
//     if (!b_realtime_) return nullptr; 
//     std::lock_guard<std::mutex> lock(mtx_);

//     long long latest_pts = 0;;

//     while (!rt_queue_.empty())
//     {
//         auto info = rt_queue_.front();
//         rt_queue_.pop_front();
//         latest_pts = info->pts;
//         if (info->pts > pts) return info;
//     }

//     LOGERR("req pts:%lld  lastest pts %lld no gyro mat now", pts, latest_pts);

//     return nullptr;
// }

void gyro_mgr::queue_realtime_gyro(const ins_g_a_data& i_data)
{
    std::lock_guard<std::mutex> lock(mtx_);

    while (rt_queue_.size() >= 1500) rt_queue_.pop_front();

    rt_queue_.push_back(i_data);
}

std::shared_ptr<ins_euler_angle_data> gyro_mgr::get_euler_angle(long long pts)
{
    if (!b_euler_angle_) return nullptr; 
    std::lock_guard<std::mutex> lock(mtx_euler_);

    long long latest_pts = 0;;

    while (!euler_queue_.empty())
    {
        auto info = euler_queue_.front();
        euler_queue_.pop_front();
        latest_pts = info->pts;
        if (info->pts > pts) return info;
    }

    LOGERR("pts:%lld  %lld no euler angle data now", pts, latest_pts);

    return nullptr;
}

void gyro_mgr::queue_euler_angle(long long pts, const ins::Vector3d& angle)
{
    std::lock_guard<std::mutex> lock(mtx_euler_);
    if (!b_euler_angle_)
    {
        if (!euler_queue_.empty()) euler_queue_.clear();
        return;
    }

    if(euler_queue_.size() > 1500)
    {
        LOGINFO("gyro euler queue size:%d, drop 1000", euler_queue_.size());
        while (euler_queue_.size() > 500) euler_queue_.pop_front();
    }

    auto info = std::make_shared<ins_euler_angle_data>();
    info->pts = pts;
    info->x = angle.x;
    info->y = angle.y;
    info->z = angle.z;

    euler_queue_.push_back(info);
}

void gyro_mgr::queue_rec_gyro(const ins_g_a_data& i_data)
{
    if (!b_record_) return;
    std::lock_guard<std::mutex> lock(mtx_rec_);

    for (auto it = sink_.begin(); it != sink_.end(); it++)
    {
        (*it)->queue_gyro(i_data);
    }

    if (buff_ == nullptr)
    {
        buff_ = std::make_shared<page_buffer>(_GYRO_FILE_BUFF_SIZE_);
        buff_->set_offset(0);
    }  

    double data_buff[8];
    memset(data_buff, 0xff, sizeof(double));//前8个字节全ff,标示开始
    data_buff[1] = i_data.pts/1000000.0;
    data_buff[2] = i_data.gyro_x;
    data_buff[3] = i_data.gyro_y;
    data_buff[4] = i_data.gyro_z;
    data_buff[5] = i_data.accel_x;
    data_buff[6] = i_data.accel_y;
    data_buff[7] = i_data.accel_z;
    char* p = (char*)data_buff;

    unsigned int size = std::min(buff_->size()-buff_->offset(), (unsigned)sizeof(data_buff));
    memcpy(buff_->data() + buff_->offset(), p, size);
    buff_->set_offset(buff_->offset()+size);
    p += size;
    if (size < sizeof(data_buff))
    {
        if(rec_queue_.size() > 50)
        {
            int drop_cnt = 20; 
            LOGINFO("gyro file queue size:%d, drop:%d", rec_queue_.size(), drop_cnt);
            while (drop_cnt-- > 0) rec_queue_.pop_front();
        }

        rec_queue_.push_back(buff_);
        buff_ = std::make_shared<page_buffer>(_GYRO_FILE_BUFF_SIZE_);
        buff_->set_offset(0);

        size = sizeof(data_buff) - size;
        memcpy(buff_->data() + buff_->offset(), p, size);
        buff_->set_offset(buff_->offset()+size);
    }
}

std::shared_ptr<page_buffer> gyro_mgr::dequeue_rec_gyro()
{
    std::lock_guard<std::mutex> lock(mtx_rec_);
    if (rec_queue_.empty()) return nullptr;

    auto data = rec_queue_.front();
    rec_queue_.pop_front();
    return data;
}

void gyro_mgr::rec_task()
{
    std::shared_ptr<page_buffer> buff;

    while (1)
    {
        buff = dequeue_rec_gyro();
        if (buff != nullptr)
        {
            write_rec_gyro(buff);
            usleep(10*1000);
        }
        else if (b_record_)
        {
            usleep(50*1000);
        }
        else
        {
            if (buff_) write_rec_gyro(buff_);
            break;
        }
    } 
}

void gyro_mgr::write_rec_gyro(const std::shared_ptr<page_buffer>& buff)
{
    if (fd_ < 0) return;
    if (buff->offset() <= 0) return;
    
    int ret = write(fd_, buff->data(), buff->offset());
    if (ret < 0)
    {
        LOGERR("gyro write fail:%d", ret);
    }
    else
    {
        //LOGINFO("write gyro size:%d", buff->size());
    }
}


