#include "temp_monitor.h"
#include "hw_util.h"
#include "common.h"
#include "inslog.h"
#include "access_msg_center.h"
#include <sstream>
#include <unistd.h>
//#include "ins_battery.h"

#define CPU_GPU_TEMP_THRESHOLD 85
#define BATTERY_TEMP_THRESHOLD 73

temp_monitor::temp_monitor(std::function<bool()>& cb)
{
    need_report_temp_cb_ = cb;
    //battery_ = std::make_shared<ins_battery>();
    //if (battery_->open() != INS_OK) battery_ = nullptr;
    th_ = std::thread(&temp_monitor::task, this);
}  

temp_monitor::~temp_monitor()
{ 
    quit_ = true;
    cv_.notify_all();
    INS_THREAD_JOIN(th_);
}

void temp_monitor::task()
{
    int32_t hight_cnt = 0;
    double battery_temp = 0;
    int32_t cpu_temp, gpu_temp;

    LOGINFO("temp monitor task run");
     
    while (!quit_)
    {
        do
        {
            if (need_report_temp_cb_ && !need_report_temp_cb_()) break;

            cpu_temp = hw_util::get_temp(SYS_PROPERTY_CPU_TEMP);
            if (cpu_temp >= CPU_GPU_TEMP_THRESHOLD)
            {
                hight_cnt++; break;
            }

            gpu_temp = hw_util::get_temp(SYS_PROPERTY_GPU_TEMP);
            if (gpu_temp >= CPU_GPU_TEMP_THRESHOLD)
            {
                hight_cnt++; break;
            }

            battery_temp = hw_util::get_temp(SYS_PROPERTY_BATTERY_TEMP);
            if (battery_temp >= BATTERY_TEMP_THRESHOLD)
            {
                hight_cnt++; break;
            }

            // if (battery_)
            // {
            //     auto ret = battery_->read_temp(battery_temp);
            //     if (ret == INS_OK && battery_temp >= BATTERY_TEMP_THRESHOLD)
            //     {
            //         hight_cnt++; break;
            //     }
            // }

            hight_cnt = 0;
        } while (0);

        if (hight_cnt >= 3)
        {
            LOGINFO("temperature high cpu:%d gpu:%d battery:%lf", cpu_temp, gpu_temp, battery_temp);

            hight_cnt = 0;
            json_obj obj;
            obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
            obj.set_int("code", INS_ERR_TEMPERATURE_HIGH);
            access_msg_center::queue_msg(0, obj.to_string());
            
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait_for(lock, std::chrono::seconds(8)); //等待结束后再重新检测温度
        }
        else
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait_for(lock, std::chrono::seconds(2));
        }
    }

    LOGINFO("temp monitor task exit");
}