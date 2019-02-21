#ifndef _SPATIAL_AUDIO_COMPOSER_H_
#define _SPATIAL_AUDIO_COMPOSER_H_

#include <memory>
#include <thread>
#include <vector>
#include <assert.h>
#include "insbuff.h"
#include "sink_interface.h"
#include "ins_queue.h"
#include "insdemux.h"

class ffaacdec;
class mc_encode;

class spatial_audio_composer
{
public:
    ~spatial_audio_composer();
    int open(int samplerate, int channels, unsigned bitrate, const unsigned char* config, unsigned config_size);
    void set_sink(const std::shared_ptr<sink_interface>& sink);
    void queue_frame(int index, const std::shared_ptr<ins_demux_frame>& frame)
    {
        assert(index == 0 || index == 1);
        queue_[index].enqueue(frame);
    }

private:
    void task();
    int compose(const std::vector<std::shared_ptr<ins_demux_frame>>& frame);
    bool get_frame(std::vector<std::shared_ptr<ins_demux_frame>>& frame);
    std::shared_ptr<insbuff> mix_4channel(const unsigned char* in1, const unsigned char* in2, unsigned size);
    void f32_to_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
    ins_queue<std::shared_ptr<ins_demux_frame>> queue_[2];
    std::shared_ptr<ffaacdec> dec_[2];
    std::shared_ptr<mc_encode> enc_;
    void* tl_cap_handle_ = nullptr;
    std::thread th_;
    bool quit_ = false;
    std::shared_ptr<insbuff> f32_buff_;
    std::shared_ptr<insbuff> s16_buff_;
    FILE* fp1_ = nullptr;
    FILE* fp2_ = nullptr;
    FILE* fp3_ = nullptr;
};

#endif