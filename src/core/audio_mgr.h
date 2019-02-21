
#ifndef _AUDIO_ENCODE_MANAGER_H_
#define _AUDIO_ENCODE_MANAGER_H_

#include <vector>
#include <memory>
#include <string>
#include <deque>
#include <thread>
#include <mutex>
#include "sink_interface.h"
#include "obj_pool.h"
#include "ins_frame.h"

class audio_reader;
class ffaacenc;
class audio_spatial;
class audio_play;

struct audio_param
{
    // unsigned int samplerate;
    // std::string sample_fmt;
    // std::string ch_layout;
    // unsigned int bitrate;
    int32_t type = 0;
    bool fanless = false; 
    bool hdmi_audio = false;
};

class audio_mgr
{
public:
    ~audio_mgr();
    int32_t open(const audio_param& param);
    void add_output(uint32_t index, const std::shared_ptr<sink_interface>& sink, bool aux = false);
    void del_output(uint32_t index);
    uint8_t dev_type() { return dev_type_; };
    std::string dev_name() { return dev_name_; };
    bool is_spatial() { return spatial_; };

private:
    void process_task();
    int32_t open_inner_mic(int32_t type, bool fanless, bool hdmi_audio);
    int32_t open_outer_35mm_mic(bool hdmi_audio);
    int32_t open_outer_usb_mic(bool hdmi_audio);
    int32_t open_audio_enc(uint32_t samplerate, uint32_t channel, uint32_t bitrate);
    void on_pcm_data(uint32_t index, ins_pcm_frame& frame);
    int32_t align_first_frame();
    void process(std::vector<ins_pcm_frame>& v_frame);
    void queue_pcm_frame(uint32_t index, ins_pcm_frame& frame);
    bool deque_pcm_frame(uint32_t index, ins_pcm_frame& frame);

    std::vector<std::shared_ptr<audio_reader>> dev_;
    std::vector<std::shared_ptr<ffaacenc>> enc_; //只有在内置双Mic的时候才有两路流，第二路为第一个mic的双声道流
    std::vector<std::deque<ins_pcm_frame>> queue_;
    std::shared_ptr<obj_pool<insbuff>> pool_;
    std::shared_ptr<audio_spatial> spatial_handle_;
    std::shared_ptr<audio_play> play_;

    bool quit_ = false;
    std::thread th_;
    std::mutex mtx_[2];

    uint32_t buff_size_ = 0;
    uint32_t fmt_size_ = 0;
    uint8_t dev_type_ = 0;
    bool spatial_ = false;
    std::string dev_name_;
    uint32_t samplerate_;
    uint32_t channel_;
};

#endif