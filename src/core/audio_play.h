#ifndef __AUDIO_PLAY_H__
#define __AUDIO_PLAY_H__

#include <stdint.h>
#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <string>
#include "ins_frame.h"
#include <alsa/asoundlib.h>
#include <atomic>

class audio_ace;

class audio_play
{
public:
    ~audio_play();
    int32_t open(uint32_t samplerate, uint32_t channel);
    void queue_frame(const ins_pcm_frame& frame);
    static std::atomic_llong video_timestamp_;
private:
    int32_t do_open(uint32_t samplerate, uint32_t channel);
    void task();
    std::shared_ptr<insbuff> dequeue_frame();
    int32_t play(std::shared_ptr<insbuff>& buff);
    void on_frame(ins_pcm_frame& frame);
    std::queue<ins_pcm_frame> queue_;
    std::thread th_;
    std::mutex mtx_;
    bool quit_ = false;
    bool init_ = false;

    snd_pcm_t* handle_ = nullptr;
    snd_pcm_hw_params_t *params_ = nullptr;
    snd_pcm_uframes_t period_size_ = 1024;
    uint32_t frame_size_ = 4096;
    uint32_t buff_cnt_ = 3;
    uint32_t channel_  = 2;
    uint32_t samplerate_ = 48000;

    std::shared_ptr<audio_ace> aec_;
};

#endif