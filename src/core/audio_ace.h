#ifndef __AUDIO_ACE_H__
#define __AUDIO_ACE_H__

#include <stdint.h>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include "insbuff.h"
#include "ins_frame.h"
#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"
#include "obj_pool.h"

#define AEC_MIC 0
#define AEC_SPK 1

enum class AEC_QUEUE {
    MIC = 0,
    SPK,
    CNT,
};


/*
 * 音频的回声消除处理类
 */
class audio_ace {
public:
            ~audio_ace();
    int32_t setup(int32_t samplerate);
    
    void    queue_mic_frame(const ins_pcm_frame& frame) { queue_frame(frame, AEC_QUEUE::MIC); }
    void    queue_spk_frame(const ins_pcm_frame& frame) { queue_frame(frame, AEC_QUEUE::SPK); }
    
    // bool dequeue_aec_frame(ins_pcm_frame& frame) { return dequeue_frame(frame, AEC_QUEUE::AEC);}
    void    set_frame_cb(std::function<void(ins_pcm_frame& frame)> cb) { frame_cb_ = cb; }

private:
    //std::shared_ptr<insbuff> echo_cancellation(std::shared_ptr<insbuff>& mic_buff, std::shared_ptr<insbuff>& spk_buff);
    bool    dequeue_frame(ins_pcm_frame& frame, AEC_QUEUE type);
    void    queue_frame(const ins_pcm_frame& frame, AEC_QUEUE type);
    void    task();

    std::thread                 th_;
    std::mutex                  v_mtx_[(int32_t)AEC_QUEUE::CNT];
    std::queue<ins_pcm_frame>   v_queue_[(int32_t)AEC_QUEUE::CNT];
    SpeexEchoState*             st_ = nullptr;
    SpeexPreprocessState*       den_ = nullptr;
    int32_t                     samplerate_ = 48000;
    bool                        quit_ = false;
    std::function<void(ins_pcm_frame& frame)>   frame_cb_;
    std::shared_ptr<obj_pool<insbuff>>          buff_pool_;
};

#endif