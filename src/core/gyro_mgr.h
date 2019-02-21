
#ifndef _GYRO_MANAGER_H_
#define _GYRO_MANAGER_H_

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include "gyro_data.h"
#include "gyro_wrapper.h"
#include "GyroStabilizer.h"
#include "insbuff.h"
#include "metadata.h"
#include "sink_interface.h"

class gyro_mgr
{
public:
    gyro_mgr();
    ~gyro_mgr() { release(); };
    std::shared_ptr<ins_gyro_info> get_gyro_info(long long pts);
    int start_record(std::string file_name);
    int stop_record();
    int start_realtime();
    int stop_realtime();
    int calibration();
    bool is_realtime() { return b_realtime_; }
    void re_init_stab();
    void add_sink(const std::shared_ptr<sink_interface> sink);

    int start_euler_angle();
    int stop_euler_angle();
    std::shared_ptr<ins_euler_angle_data> get_euler_angle(long long pts);

private:
    void release();
    void task();
    void preview_read_task();
    void rec_task();
    void queue_realtime_gyro(const ins_g_a_data& i_data);
    void queue_rec_gyro(const ins_g_a_data& i_data);
    std::shared_ptr<page_buffer> dequeue_rec_gyro();
    void write_rec_gyro(const std::shared_ptr<page_buffer>& buff);
    void stop_read_task();
    void start_read_task();
    void queue_calibration(const GYRO_DAT& gyro_data);
    void queue_euler_angle(long long pts, const ins::Vector3d& angle);
    void init_mat();

    std::shared_ptr<gyro_wrapper> gyro_reader_;
    std::shared_ptr<ins::Stabilizer::GyroStabilizer> stabilizer_;
    std::thread th_;
    std::mutex mtx_;
    std::mutex mtx_euler_;
    std::thread th_rec_;
    std::mutex mtx_rec_;
    int fd_ = -1;
    std::shared_ptr<page_buffer> buff_;
    std::deque<ins_g_a_data> rt_queue_;
    std::deque<std::shared_ptr<ins_euler_angle_data>> euler_queue_;
    std::deque<std::shared_ptr<page_buffer>> rec_queue_;
    bool quit_ = false;
    bool b_record_  = false; 
    bool b_realtime_ = false;
    bool b_euler_angle_ = false;
    // bool b_init_ = false;
    // FILE* fp = nullptr;
    std::mutex mtx_s_;

    double gyro_x_offset_ = 0.0;
    double gyro_y_offset_ = 0.0;
    double gyro_z_offset_ = 0.0; 
    double accel_x_offset_ = 0.0;
    double accel_y_offset_ = 0.0;
    double accel_z_offset_ = 0.0;

    double gyro_x_ = 0.0;
	double gyro_y_ = 0.0;
	double gyro_z_ = 0.0;
	double accel_x_ = 0.0;
	double accel_y_ = 0.0;
	double accel_z_ = 0.0;
    bool b_calibration = false;
    int calibration_cnt_ = -1;

    std::vector<std::shared_ptr<sink_interface>> sink_;
};

#endif