
#include "gps_mgr.h"
#include "inslog.h"
#include "common.h"
#include "access_msg.h"
#include <sstream>
#include <sys/time.h>
#include <math.h>
#include "cam_manager.h"
#include "access_msg_center.h"
#include "camera_info.h"

gps_mgr* gps_mgr::mgr_ = nullptr;

gps_mgr::gps_mgr()
{
    send_state_msg(dev_status_);	// 第一次上报无设备
    dev_ = std::make_unique<gps_dev>();
    dev_->open();

    pool_ = std::make_shared<obj_pool<ins_gps_data>>(256, "gps");

    std::function<void(GoogleGpsData&)> cb = [this](GoogleGpsData& data)
    {
        on_gps_data(data);
    };

    dev_->set_data_cb(cb);
}

void gps_mgr::on_gps_data(GoogleGpsData& data)
{

    if (data.fix_type >= 2 && data.timestamp > 0 && !camera_info::is_sync_time()) { 
        send_system_time_change_msg(data.timestamp);
    }

    struct timeval tm;
    gettimeofday(&tm, nullptr);
    data.timestamp = tm.tv_sec*1000 + (double)tm.tv_usec/1000.0; //单位毫秒

    int32_t state = transform_state(data.fix_type);
    
    if (dev_status_ != state) {
        dev_status_ = state;
        send_state_msg(state);
    }

    if (dev_status_ == INS_GPS_STATE_NONE) return;

    std::shared_ptr<ins_gps_data> obj;
    for (int32_t i = 0; i < 10; i++) {
        obj = pool_->pop();
        if (obj == nullptr) {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.pop_front();
        } else {
            break;
        }
    }

    if (!obj) {
        LOGERR("gps pool no obj left");
        return;
    }

    obj->pts 			= data.timestamp*1000 - cam_manager::nv_amba_delta_usec(); //trans to us
    obj->fix_type  		= data.fix_type; 
    obj->latitude 		= data.latitude; 
    obj->longitude 		= data.longitude; 
    obj->altitude 		= data.altitude; 
    obj->h_accuracy 	= data.h_accuracy; 
    obj->v_accuracy 	= data.v_accuracy; 
    obj->velocity_east 	= data.velocity_east;
    obj->velocity_north = data.velocity_north; 
    obj->velocity_up 	= data.velocity_up; 
    obj->speed_accuracy = data.speed_accuracy; 
    obj->sv_status.sv_num = std::min(data.status.num_svs, INS_MAX_SV_NUM);
    memcpy(obj->sv_status.sv_list, data.status.sv_list, sizeof(obj->sv_status.sv_list));

    output(obj);

    #if 0
    LOGINFO("time:%lld fixtype:%d latitude:%lf longitude:%lf altitude:%f speed:%f sv num:%d", 
        obj->pts, 
        obj->fix_type, 
        obj->latitude, 
        obj->longitude, 
        obj->altitude,
        obj->speed_accuracy, 
        obj->sv_status.sv_num);

    for (int32_t i = 0; i < obj->sv_status.sv_num; i++) {
        LOGINFO("i:%d prn:%d snr:%f elevation:%f azimuth:%f", 
            i, obj->sv_status.sv_list[i].prn,
            obj->sv_status.sv_list[i].snr,
            obj->sv_status.sv_list[i].elevation,
            obj->sv_status.sv_list[i].azimuth);
    }
    #endif
}


void gps_mgr::output(std::shared_ptr<ins_gps_data>& data)
{
    std::lock_guard<std::mutex> lock(mtx_);

    queue_.push_back(data);

    for (auto& it : sinks_) {
        it->queue_gps(data);
    }
}


int32_t gps_mgr::transform_state(int32_t fix_type)
{
    if (fix_type < 0) {
        return INS_GPS_STATE_NONE; 		/* 没插设备 */
    } else if (fix_type <= INS_GPS_STATE_UNLOCATED) {
        return INS_GPS_STATE_UNLOCATED; /* 无定位 */
    } else {
        return fix_type;
    }
}

//获取最新的数据
std::shared_ptr<ins_gps_data> gps_mgr::get_gps()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return nullptr;
    return queue_.back();
}

std::shared_ptr<ins_gps_data> gps_mgr::get_gps(int64_t pts)
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (queue_.empty()) return nullptr;

    auto front_item = queue_.front();
    auto back_item = queue_.back();

    // reverse search
    for (auto it = queue_.rbegin(); it != queue_.rend(); it++) {
        if (pts >= (*it)->pts) {
            front_item = *it;
            break;
        } else {
            back_item = *it;
        }
    }

    // nearest timestamp
    if ((fabs(pts - front_item->pts)) > (fabs(pts - back_item->pts))) {
        return back_item;
    } else {
        return front_item;
    }
}

void gps_mgr::send_state_msg(int32_t state)
{
    json_obj obj;
    obj.set_int("state", state);
    access_msg_center::send_msg(ACCESS_CMD_GPS_STATE_, &obj);
}

void gps_mgr::send_system_time_change_msg(int64_t timestamp)
{
    if (!access_msg_center::is_idle_state()) return; /* 不是idle不能设置系统时间,所以不发送无谓多的消息 */

    if (last_gps_time_ == timestamp) return;
	
    last_gps_time_ = timestamp;
    
    json_obj obj;
    json_obj param_obj;
    param_obj.set_int64("timestamp", timestamp);
    param_obj.set_string("source", "gps");
    obj.set_string("name", ACCESS_CMD_SYSTEM_TIME_CHANGE);
    obj.set_obj(ACCESS_MSG_PARAM, &param_obj);
    access_msg_center::queue_msg(0, obj.to_string());
}

