#ifndef _GPS_MGR_H_
#define _GPS_MGR_H_

#include <thread>
#include <assert.h>
#include "metadata.h"
#include <mutex>
#include <vector>
#include <deque>
#include "gps_dev.h"
#include "obj_pool.h"
#include "gps_sink.h"

class gps_mgr
{
public:
    static gps_mgr* create()
    {
        if (!mgr_) mgr_ = new gps_mgr();
        return mgr_;
    };

    static void destroy()
    {
        if (mgr_)
        {
            delete mgr_;
            mgr_ = nullptr;
        }
    };

    static gps_mgr* get()
    {
        assert(mgr_);
        return mgr_;
    }

    void add_output(std::shared_ptr<gps_sink> sink)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        sinks_.push_back(sink);
    }

    void del_all_output()
    {
        // std::lock_guard<std::mutex> lock(mtx_);
        // auto it = sinks_.find(index);
        // if (it != sinks_.end()) sinks_.erase(it);
        std::lock_guard<std::mutex> lock(mtx_);
        sinks_.clear();
    }

    std::shared_ptr<ins_gps_data> get_gps(int64_t pts);
    std::shared_ptr<ins_gps_data> get_gps();//获取最近时间的
    int32_t get_state() { return  dev_status_; }; 

private:
    gps_mgr();
    void output(std::shared_ptr<ins_gps_data>& data);
    void on_gps_data(GoogleGpsData& data);
    void send_state_msg(int32_t state);
    void send_system_time_change_msg(int64_t timestamp);
    int32_t transform_state(int32_t fix_type);
    std::shared_ptr<obj_pool<ins_gps_data>> pool_;
    std::mutex mtx_;
    std::unique_ptr<gps_dev> dev_;
    int32_t dev_status_ = 0;
    int64_t last_gps_time_ = std::numeric_limits<int64_t>::min();
    std::deque<std::shared_ptr<ins_gps_data>> queue_;
    std::vector<std::shared_ptr<gps_sink>> sinks_;
    static gps_mgr* mgr_;
};

#endif