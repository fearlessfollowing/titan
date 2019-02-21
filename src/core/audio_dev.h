#ifndef _AUDIO_DEV_H_
#define _AUDIO_DEV_H_

#include <alsa/asoundlib.h>
#include <memory>
#include <stdint.h>
#include "insbuff.h"

class audio_dev
{
public:
    ~audio_dev();
    int32_t open(uint32_t card, uint32_t dev);
    int32_t set_param(uint32_t samplerate, uint32_t channel, snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE);
    int32_t get_param(uint32_t& samplerate, uint32_t& channel, snd_pcm_format_t& fmt);
    int32_t read(std::shared_ptr<insbuff>& buff);
    bool is_spatial();
    std::string get_name()
    {
        return hw_name_;
    }
    std::string get_vender_name()
    {
        char* name = nullptr;
        snd_card_get_name(card_, &name);
        return name;
    }
    uint32_t get_frame_size()
    {
        return frame_bytes_;
    }
    uint32_t get_fmt_size(snd_pcm_format_t fmt)
    {
        return snd_pcm_format_size(fmt, 1);
    }

private:
    bool is_channel_3_silence();
    snd_pcm_t* handle_ = nullptr;
    snd_pcm_hw_params_t *params_ = nullptr;
    uint32_t card_ = 0;
    std::string hw_name_;
    uint32_t frame_bytes_ = 0;
};

#endif