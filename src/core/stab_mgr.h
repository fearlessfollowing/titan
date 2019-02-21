#ifndef __STABILIZATION_MANAGER_H__
#define __STABILIZATION_MANAGER_H__

#include "common.h"
#include "metadata.h"
#include "sink_interface.h"

class gyro_mgr;

class stab_mgr
{
public:
    static stab_mgr* create()
    {
        if (instance_ != nullptr) return instance_;
        instance_ = new stab_mgr();
        return instance_;
    };

    static void destroy()
    {
        INS_DELETE(instance_);
    };

    static stab_mgr* get()
    {
        INS_ASSERT(instance_ != nullptr);
        return instance_;
    }

    std::shared_ptr<ins_gyro_info> get_gyro_info(long long pts);
    std::shared_ptr<ins_euler_angle_data> get_euler_angle(long long pts);
    int start_record(std::string file_name);
    int stop_record();
    int start_realtime();
    int stop_realtime();
    int start_euler_angle();
    int stop_euler_angle();
    int calibration();
    bool is_realtime();
    void re_init_stab();
    void add_sink(const std::shared_ptr<sink_interface> sink);

private:
    stab_mgr();
    std::shared_ptr<gyro_mgr> gyro_;
    static stab_mgr* instance_;
};

// #define STAB_GET_GYRO_INFO(pts) stab_mgr::get()->get_gyro_info(pts);
// #define STAB_START_REC(file_name) stab_mgr::get()->start_record(file_name);
// #define STAB_STOP_REC() stab_mgr::get()->stop_record();
// #define STAB_START_REALTIME() stab_mgr::get()->start_realtime();
// #define STAB_STOP_REALTIME() stab_mgr::get()->stop_realtime();

#endif