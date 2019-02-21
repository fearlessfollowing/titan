
#include "stab_mgr.h"
//#include "gyro_mgr.h"
#include "inslog.h"

stab_mgr* stab_mgr::instance_ = nullptr;

stab_mgr::stab_mgr()
{
    //gyro_ = std::make_shared<gyro_mgr>();
}

std::shared_ptr<ins_gyro_info> stab_mgr::get_gyro_info(long long pts)
{
    // if (gyro_)
    // {
    //     return gyro_->get_gyro_info(pts);
    // }
    // else
    {
        return nullptr;
    }
}

int stab_mgr::start_record(std::string file_name)
{
    // if (gyro_)
    // {
    //     return gyro_->start_record(file_name);
    // }
    // else
    {
        return INS_ERR;
    }
}

int stab_mgr::stop_record()
{
    // if (gyro_)
    // {
    //     return gyro_->stop_record();
    // }
    // else
    {
        return INS_ERR;
    }
}

int stab_mgr::start_realtime()
{
    // if (gyro_)
    // {
    //     return gyro_->start_realtime();
    // }
    // else
    {
        return INS_ERR;
    }
}

int stab_mgr::stop_realtime()
{
//     if (gyro_)
//     {
//         return gyro_->stop_realtime();
//     }
//     else
    {
        return INS_ERR;
    }
}

int stab_mgr::calibration()
{
    // if (gyro_)
    // {
    //     return gyro_->calibration();
    // }
    // else
    {
        return INS_ERR;
    }
}

bool stab_mgr::is_realtime()
{   
    // if (gyro_)
    // {
    //     return gyro_->is_realtime();
    // }
    // else
    {
        return false;
    }
}

void stab_mgr::re_init_stab()
{
    // if (gyro_)
    // {
    //     return gyro_->re_init_stab();
    // }
}

std::shared_ptr<ins_euler_angle_data> stab_mgr::get_euler_angle(long long pts)
{
    // if (gyro_)
    // {
    //     return gyro_->get_euler_angle(pts);
    // }
    // else
    {
        return nullptr;
    }
}

int stab_mgr::start_euler_angle()
{
    // if (gyro_)
    // {
    //     return gyro_->start_euler_angle();
    // }
    // else
    {
        return INS_ERR;
    }
}
    
int stab_mgr::stop_euler_angle()
{
    // if (gyro_)
    // {
    //     return gyro_->stop_euler_angle();
    // }
    // else
    {
        return INS_ERR;
    }
}

void stab_mgr::add_sink(const std::shared_ptr<sink_interface> sink)
{
    // if (gyro_)
    // {
    //     return gyro_->add_sink(sink);
    // }
}

