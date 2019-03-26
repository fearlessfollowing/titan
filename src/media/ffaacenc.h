

#ifndef _FF_AAC_ENC_H_
#define _FF_AAC_ENC_H_

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
}

#include <memory>
#include <mutex>
#include <map>
#include <deque>
#include <thread>
#include "Arational.h"
#include "ins_frame.h"
#include "sink_interface.h"

struct ffaacenc_param {
    uint32_t samplefmt = AV_SAMPLE_FMT_S16;
    uint32_t samplerate = 0;
    uint32_t channel = 1;
    uint32_t bitrate = 0;
    bool spatial = false;
    Arational timescale;
};

class ffaacenc {
public:
			~ffaacenc();
    int32_t open(ffaacenc_param& param, bool sync = true);
    int32_t encode(const ins_pcm_frame& frame);
    void 	add_output(uint32_t index, std::shared_ptr<sink_interface> sink)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        sinks_.insert(std::make_pair(index, sink));
    }
    void 	del_output(uint32_t index)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sinks_.find(index);
        if (it != sinks_.end()) sinks_.erase(it);
    }

private:
    void 	task();
    int32_t do_encode(const ins_pcm_frame& frame);
    bool 	dequeue_frame(ins_pcm_frame& frame);
    void 	output(AVPacket& pkt);
    bool 	sync_ = true;
	AVCodec* 			codec_ = nullptr;
	AVCodecContext* 	ctx_ = nullptr;
    AVFrame* 			av_frame_ = nullptr;
    std::mutex 			mtx_;
    std::thread 		th_;
    bool 				quit_ = false;
    std::map<uint32_t, std::shared_ptr<sink_interface>> sinks_;
    std::deque<ins_pcm_frame> queue_;
    bool 				spatial_ = false;
    uint32_t 			buff_size_ = 0;
	
    std::shared_ptr<obj_pool<ins_frame>>	frame_pool_;   
    std::shared_ptr<obj_pool<insbuff>> 		buff_pool_;
};

#endif

