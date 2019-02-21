#ifndef _USB_SINK_H_
#define _USB_SINK_H_

#include "sink_interface.h"
#include <deque>
#include <thread>
#include <mutex>
#include <string>

class cam_manager;

class usb_sink : public sink_interface
{
public:
    int32_t open(cam_manager* camera);
    virtual ~usb_sink();
	virtual void set_video_param(const ins_video_param& param) 
    {
    };
    virtual bool is_video_param_set() 
    { 
        return false; 
    };
    virtual void set_audio_param(const ins_audio_param& param) 
    { 
        audio_param_ = std::make_shared<ins_audio_param>();
        *(audio_param_.get()) = param;
    };
	virtual bool is_audio_param_set() 
    { 
        return (audio_param_!=nullptr); 
    };
	virtual void queue_frame(const std::shared_ptr<ins_frame>& frame);
    virtual void queue_gps(std::shared_ptr<ins_gps_data>& gps);
    void set_audio(bool value)
    {
        b_audio_ = value;
    }
    void set_gps(bool value)
    {
        b_gps_ = value;
    }
    void set_prj_file(std::string filename)
    {
        prj_file_name_ = filename;
    }

private:
    void task();
    std::shared_ptr<insbuff> deque_frame();
    std::shared_ptr<insbuff> deque_audio_frame();
    std::shared_ptr<insbuff> deque_gps_frame();
    std::shared_ptr<insbuff> gen_prj_file_frame();

    bool b_gps_ = false;
    bool b_audio_ = false;
    std::thread th_;
    std::mutex mtx_;
    bool quit_ = false;
    bool audio_param_send_ = false;
    bool prj_file_send_ = false;
    std::shared_ptr<insbuff> buff_;
    std::shared_ptr<ins_audio_param> audio_param_;
	std::deque<std::shared_ptr<ins_frame>> audio_queue_;
	std::deque<std::shared_ptr<ins_gps_data>> gps_queue_;
    cam_manager* camera_;
    std::string prj_file_name_;
    uint32_t audio_seq_ = 0;
    uint32_t gps_seq_ = 0;
};

#endif