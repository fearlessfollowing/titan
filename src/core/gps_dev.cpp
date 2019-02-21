#include "gps_dev.h"
#include "common.h"
#include "inslog.h"
#include <unistd.h>

//#define GPS_TEST

gps_dev* dev_ = nullptr; 

void data_cb(GoogleGpsData data)
{
    if (!dev_) return;

    // if (dev_->no_cb_cnt_ == -1)
    // {
    //     system("stty -F /dev/ttyTHS1 115200");
    //     LOGINFO("stty -F /dev/ttyTHS1 115200");
    // }
    dev_->no_cb_cnt_ = 0;
    if (dev_->data_cb_) dev_->data_cb_(data);

    if (data.fix_type < 0)
    {
        dev_->dev_error_ = true;
        LOGERR("the GPS device called soybean is out of service now");
    }

    #ifndef GPS_TEST
    if (dev_->cnt_++%900 == 0) //90s
    #endif
    {
        LOGINFO("time:%lf fixtype:%d latitude:%lf longitude:%lf altitude:%f speed:%f svnum:%d", 
            data.timestamp, 
            data.fix_type, 
            data.latitude, 
            data.longitude, 
            data.altitude,
            data.speed_accuracy, 
            data.status.num_svs);
    }
}

gps_dev::~gps_dev()
{
    quit_ = true;
    cv_.notify_all();
    INS_THREAD_JOIN(th_);
    dev_ = nullptr;
}

void gps_dev::open()
{
    // system("echo 303 > /sys/class/gpio/export");
    // system("echo out > /sys/class/gpio/gpio303/direction");
    // system("echo 0 > /sys/class/gpio/gpio303/value");
    
    no_cb_cnt_ = 0;
    dev_error_ = false;
    dev_ = this;
    th_ = std::thread(&gps_dev::task, this);
}

void gps_dev::task()
{
    bool first_open = true;

    LOGINFO("GPS task running");

    while (!quit_)
    {
        if (dev_error_ || first_open)
        {
            if (dev_error_)
            {
                ins_gpsRelease();
                dev_error_ = false;
            }

            while (!quit_)
            {
                if (ins_gpsInit("/dev/ttyTHS1"))
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    cv_.wait_for(lock, std::chrono::seconds(2));
                    continue;
                }
                else
                {
                    first_open = false;
                    ins_gpsSetCallback(data_cb);
                    LOGINFO("the GPS device called soybean is in service now");
                    break;
                }
            }
        }
        else
        {
            if (no_cb_cnt_ >= 0) no_cb_cnt_++;
            if (no_cb_cnt_ > 50)//5s没有回调当成异常
            {
                no_cb_cnt_ = -1; //表示已处于读不到数据状态,不用重复上报
                if (data_cb_) //伪造数据上报设备异常
                {
                    GoogleGpsData data;
                    data.fix_type = -1; //表示设备异常
                    data_cb_(data);
                }
                dev_error_ = true;
                system("stty -F /dev/ttyTHS1 9600");
                LOGINFO("stty -F /dev/ttyTHS1 9600");
                continue;
            }
            usleep(100*1000);
        }
    }

    ins_gpsRelease();

    LOGINFO("GPS task exit");
}
