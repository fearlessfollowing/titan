#include "audio_ace.h"
#include "inslog.h"
#include "common.h"

#define FRAME_SIZE 512

audio_ace::~audio_ace()
{
    quit_  = true;
    INS_THREAD_JOIN(th_);

    if (st_)
    {
        speex_echo_state_destroy(st_);
        st_ = nullptr;
    }

    if (den_)
    {
        speex_preprocess_state_destroy(den_);
        den_ = nullptr;
    }
}

int32_t audio_ace::setup(int32_t samplerate)
{
    samplerate_ = samplerate;
    st_ = speex_echo_state_init(FRAME_SIZE, FRAME_SIZE*20);
    speex_echo_ctl(st_, SPEEX_ECHO_SET_SAMPLING_RATE, &samplerate);

    den_ = speex_preprocess_state_init(FRAME_SIZE, samplerate);
    speex_preprocess_ctl(den_, SPEEX_PREPROCESS_SET_ECHO_STATE, st_);

    buff_pool_ = std::make_shared<obj_pool<insbuff>>(-1, "aec buff");
    buff_pool_->alloc(32);

    th_ = std::thread(&audio_ace::task, this);

    LOGINFO("aec setup samplerate:%d", samplerate);
}

void audio_ace::queue_frame(const ins_pcm_frame& frame, AEC_QUEUE type)
{
    int32_t index = (int32_t)type;
    std::lock_guard<std::mutex> lock(v_mtx_[index]);
    v_queue_[index].push(frame);
}

bool audio_ace::dequeue_frame(ins_pcm_frame& frame, AEC_QUEUE type)
{
    int32_t index = (int32_t)type;
    std::lock_guard<std::mutex> lock(v_mtx_[index]);
    if (v_queue_[index].empty()) return false;
    frame = v_queue_[index].front();
    v_queue_[index].pop();
    return true;
}

void audio_ace::task()
{
    bool first = true;
    uint64_t frame_duration = 1000000*INS_AUDIO_FRAME_SIZE/samplerate_;

    ins_pcm_frame spk_frame; 
    while (!quit_)
    {
        ins_pcm_frame mic_frame;
		if (!dequeue_frame(mic_frame, AEC_QUEUE::MIC))
		{
			usleep(20*1000);
			continue;
		}

		while (!quit_ && !spk_frame.buff)
		{
			if (!dequeue_frame(spk_frame, AEC_QUEUE::SPK))
			{
                if (first) break;
				usleep(20*1000);
				continue;
			}
			else if (mic_frame.pts > spk_frame.pts + frame_duration)
			{
                spk_frame.buff = nullptr;
                continue;
            }
            else
            {
				break;
			}
		}

        if (quit_) break;

        if (spk_frame.buff == nullptr || mic_frame.pts < spk_frame.pts + 0*frame_duration)
        {
            if (frame_cb_) frame_cb_(mic_frame);
            //LOGINFO("no aec frame pts:%ld", mic_frame.pts);
            continue;
        }

        auto out_buff = buff_pool_->pop();
        if (!out_buff->data()) out_buff->alloc(INS_AUDIO_FRAME_SIZE*sizeof(short));
        //auto out_buff = std::make_shared<insbuff>(mic_frame.buff->size());
        speex_echo_cancellation(st_, (spx_int16_t*)mic_frame.buff->data(), (spx_int16_t*)spk_frame.buff->data(), (spx_int16_t*)out_buff->data());
        speex_preprocess_run(den_, (spx_int16_t*)out_buff->data());
        speex_echo_cancellation(st_, (spx_int16_t*)(mic_frame.buff->data()+mic_frame.buff->size()/2), 
                            (spx_int16_t*)(spk_frame.buff->data()+spk_frame.buff->size()/2), 
                            (spx_int16_t*)(out_buff->data()+out_buff->size()/2));
        speex_preprocess_run(den_, (spx_int16_t*)(out_buff->data()+out_buff->size()/2));

        ins_pcm_frame out_frame;
        out_frame.pts = mic_frame.pts;
        out_frame.buff = out_buff;
        if (frame_cb_) frame_cb_(out_frame);

        //LOGINFO("aec frame pts:%ld", mic_frame.pts);

        first = false;
        spk_frame.buff = nullptr;
    }

    LOGINFO("aec task exit");
}