#include "audio_record.h"
#include "access_msg_center.h"
#include "inslog.h"
#include "common.h"
#include <sstream>
#include "hw_util.h"
#include <unistd.h>
#include "camera_info.h"

audio_record::~audio_record()
{
    quit_ = true;
    cv_.notify_all();
	INS_THREAD_JOIN(th_);

    hw_util::switch_fan(true);

    //camera_info::set_volume(origin_gain_);

    LOGINFO("capture audio stop");
}

int32_t audio_record::start()
{
    LOGINFO("start capture audio");

    if (access(INS_NOISE_SAMPLE_PATH, 0)) {
		mkdir(INS_NOISE_SAMPLE_PATH, 0755);
	}

    origin_gain_ = camera_info::get_volume();
    //camera_info::set_volume(127);

    th_ = std::thread(&audio_record::task, this);

    return INS_OK;
}


void audio_record::task()
{
    /* 录制有风扇时候的声音 */
    hw_util::switch_fan(true);

    sleep_seconds(3);   //风扇开启到完全转动起来需要时间

    //arecord -Dhw:1,1 -f dat -d ${RECORD_DURATION} ${IN1P_RECORD_FILE}
    
    std::string cmd;
    cmd = std::string("arecord -Dhw:1,0 -f dat ") + INS_NOISE_SAMPLE_PATH + "/0.wav &";
    system(cmd.c_str());

    cmd = std::string("arecord -Dhw:1,1 -f dat ") + INS_NOISE_SAMPLE_PATH + "/1.wav &";
    system(cmd.c_str());

    sleep_seconds(10);

    system("killall arecord");

    LOGINFO("capture sample nosie with fan finish");

    if (quit_) return;

    /* 录制无风扇时候的声音 */
    hw_util::switch_fan(false);

    sleep_seconds(4);   /* 风扇停止转动需要时间 */

    if (quit_) return;

    cmd = std::string("arecord -Dhw:1,0 -f dat ") + INS_NOISE_SAMPLE_PATH + "/fanless_0.wav &";
    system(cmd.c_str());

    cmd = std::string("arecord -Dhw:1,1 -f dat ") + INS_NOISE_SAMPLE_PATH + "/fanless_1.wav &";
    system(cmd.c_str());

    sleep_seconds(5);

    system("killall arecord");

    LOGINFO("capture sample nosie without fan finish");

    json_obj obj;
    obj.set_string("name", INTERNAL_CMD_CAPTURE_AUDIO_F);
    obj.set_int("code", INS_OK);
	access_msg_center::queue_msg(0, obj.to_string());
}

void audio_record::sleep_seconds(uint32_t seconds)
{
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait_for(lock, std::chrono::seconds(seconds));

}