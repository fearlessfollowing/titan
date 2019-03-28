#ifndef _AUDIO_READER_H_
#define _AUDIO_READER_H_

#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <alsa/asoundlib.h>
#include "obj_pool.h"
#include "ins_frame.h"

class audio_dev;

class audio_reader {
public:
    			~audio_reader();
    int32_t 	open(int32_t card, int32_t dev, int32_t dev_type);
    void 		set_offset(int64_t offset_time); //如果不需要两个设备同步，设置为0
    bool 		deque_frame(ins_pcm_frame& frame);
	
    void 		get_param(uint32_t& samplerate, uint32_t& channel, uint32_t& fmt_size) {
        channel = channel_;
        samplerate = samplerate_;
        fmt_size = fmt_size_;
    }

	void set_frame_cb(uint32_t index, std::function<void(uint32_t, ins_pcm_frame&)> cb) {
        index_ = index;
        frame_cb_ = cb;
    }
	
    uint64_t base_time_ = -1;
    
private:
    void 		task();
    void 		aliagn_frame(const std::shared_ptr<insbuff>& buff);
    void 		output(const std::shared_ptr<insbuff>& buff);
    int32_t 	calc_delta_sample();
    void 		set_base_time();
    void 		send_rec_over_msg(int32_t errcode) const;
    std::shared_ptr<audio_dev> dev_;
    std::string 			name_;
    uint32_t 				samplerate_ = 0;
    uint32_t 				channel_ = 0;
    snd_pcm_format_t 		fmt_;
    uint32_t 				frame_size_ = 0;
    uint32_t 				fmt_size_ = 0;
    int64_t 				offset_time_ = -1;
    int64_t 				frame_seq_ = 0;
	bool 					quit_  = false;
	std::thread 				th_;
    std::shared_ptr<insbuff> 	pre_buff_;
    std::deque<ins_pcm_frame> 	queue_;
	
    std::shared_ptr<obj_pool<insbuff>> 				pool_;
    std::function<void(uint32_t, ins_pcm_frame&)> 	frame_cb_;
    uint32_t 		index_ = 0;
    int32_t 		dev_type_ = 0;

    int32_t 		read_cnt_ = 0;
	int64_t 		min_delta_ = 0x7fffffff;
};

#endif