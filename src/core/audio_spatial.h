#ifndef _AUDIO_SPATIAL_H_
#define _AUDIO_SPATIAL_H_

#include <memory>
#include <vector>
#include <stdint.h>
#include "insbuff.h"

class audio_spatial {
public:
            ~audio_spatial();
    int32_t open(uint32_t samplerate, uint32_t channel, bool fanless);
    int32_t compose(const std::vector<std::shared_ptr<insbuff>>& v_in, std::shared_ptr<insbuff>& out);
    std::vector<std::shared_ptr<insbuff>> denoise(const std::vector<std::shared_ptr<insbuff>>& v_in); //双路(4ch)降噪
    std::shared_ptr<insbuff> denoise(const std::shared_ptr<insbuff> in); //一路(2ch)降噪
    bool b_denoise() { return !fanless_; }; //无风扇模式不降噪

private:
    std::shared_ptr<insbuff>    get_noise_sample(bool fanless, int index);
    int32_t                     get_ansdb_by_volume(uint32_t volume);
    
    void*               tl_cap_ = nullptr;
    std::vector<void*>  tl_ans_;
    std::vector<std::shared_ptr<insbuff>> ans_in_buff_;
    std::vector<std::shared_ptr<insbuff>> ans_out_buff_;
    std::shared_ptr<insbuff> cap_in_buff_;
    std::shared_ptr<insbuff> cap_out_buff_;
    uint32_t    frame_size_ = 0;
    bool        fanless_ = false;
    int32_t     cur_dn_vol_ = 0;
};

#endif