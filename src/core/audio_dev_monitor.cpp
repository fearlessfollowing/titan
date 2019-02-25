#include "audio_dev_monitor.h"
#include "common.h"
#include "inslog.h"
#include "access_msg_center.h"
#include "json_obj.h"
#include "hw_util.h"
#include <chrono>
#include <alsa/asoundlib.h>



/**********************************************************************************************
 * 函数名称: ~audio_dev_monitor
 * 功能描述: 析构(停止监听线程)
 * 参   数: 
 * 返 回 值: 
 *********************************************************************************************/
audio_dev_monitor::~audio_dev_monitor()
{
    quit_ = true;
    cv_.notify_all();
    INS_THREAD_JOIN(th_);
}


/**********************************************************************************************
 * 函数名称: start
 * 功能描述: 启动音频设备监听线程
 * 参   数: 
 * 返 回 值: 
 *********************************************************************************************/
int32_t audio_dev_monitor::start()
{   
    th_ = std::thread(&audio_dev_monitor::task, this);
    return INS_OK;
}



/**********************************************************************************************
 * 函数名称: task
 * 功能描述: 音频设备监听任务
 * 参   数: 
 * 返 回 值: 
 *********************************************************************************************/
void audio_dev_monitor::task()
{
    dev_last_ = INS_SND_TYPE_INNER;

    LOGINFO("snd monitor task start");

    while (!quit_) {
        auto dev =  check_snd_dev();
        if (dev != dev_last_) {
            dev_last_ = dev;
            send_snd_dev_change_msg(dev);
        }

        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, std::chrono::seconds(2));
    }

    LOGINFO("snd monitor task exit");
}


/**********************************************************************************************
 * 函数名称: check_snd_dev
 * 功能描述: 检测当前系统中存在的优先级最高的音频设备
 * 参   数: 
 * 返 回 值: 
 *      INS_SND_TYPE_35MM   - 外置3.5mm MIC
 *      INS_SND_TYPE_USB    - USB接口的音频设备
 *      INS_SND_TYPE_INNER  - 机内的音频设备
 *********************************************************************************************/
int32_t audio_dev_monitor::check_snd_dev()
{
    if (hw_util::check_35mm_mic_on()) {
        return INS_SND_TYPE_35MM;
    }
    
    if (hw_util::check_usb_mic_on()) {   
        if (dev_last_ != INS_SND_TYPE_USB) {
            snd_pcm_t* handle = nullptr;
            int32_t ret = snd_pcm_open(&handle, "hw:2,0", SND_PCM_STREAM_CAPTURE, 0);
            if (ret >= 0) {
                snd_pcm_close(handle);
                return INS_SND_TYPE_USB;
            } else {
                LOGERR("usb audio open fail:%d %s", ret, snd_strerror(ret));
            }
        } else {
            return INS_SND_TYPE_USB;
        }
    }

    return INS_SND_TYPE_INNER;
}



// int32_t audio_dev_monitor::check_usb_dev_ok()
// {
//     snd_pcm_t* handle = nullptr;
//     int32_t retry = 10;
//     int32_t ret = -1;
//     while (retry-- > 0)
//     {
//         ret = snd_pcm_open(&handle, "hw:2,0", SND_PCM_STREAM_CAPTURE, 0);
//         if (ret < 0) 
//         {
//             LOGERR("usb audio open fail:%d %s", ret, snd_strerror(ret));
//             usleep(1000*1000);
//         }
//         else
//         {
//             break;
//         }
//     }

//     return ret;
// }



/**********************************************************************************************
 * 函数名称: send_snd_dev_change_msg
 * 功能描述: 发送音频设备改变通知(core)
 * 参   数: 
 *      dev - 当前的音频设备类型
 * 返 回 值: 无
 *********************************************************************************************/
void audio_dev_monitor::send_snd_dev_change_msg(int dev)
{
    json_obj obj;
	obj.set_string("name", INTERNAL_CMD_SND_DEV_CHANGE);
	obj.set_int("dev", dev);
	access_msg_center::queue_msg(0, obj.to_string());
}



