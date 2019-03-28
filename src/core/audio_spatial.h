#ifndef _AUDIO_SPATIAL_H_
#define _AUDIO_SPATIAL_H_

#include <memory>
#include <vector>
#include <stdint.h>
#include "insbuff.h"


enum {
	DENOISE_MODE_TL,
	DENOISE_MODE_RNNOISE,
	DENOISE_MODE_MX
};

#define ENABLE_DENOISE_MODE_RNNOISE
//#define ENABLE_DENOISE_MODE_TL

class audio_spatial {

public:
            ~audio_spatial();
    int32_t open(uint32_t samplerate, uint32_t channel, bool fanless);

#ifdef ENABLE_DENOISE_MODE_TL
    int32_t compose(const std::vector<std::shared_ptr<insbuff>>& v_in, std::shared_ptr<insbuff>& out);
    std::vector<std::shared_ptr<insbuff>> denoise(const std::vector<std::shared_ptr<insbuff>>& v_in); //双路(4ch)降噪
    std::shared_ptr<insbuff> denoise(const std::shared_ptr<insbuff> in); //一路(2ch)降噪
#endif

    bool b_denoise() { return !fanless_; }; //无风扇模式不降噪


#ifdef ENABLE_DENOISE_MODE_RNNOISE
	std::shared_ptr<insbuff> rnnoise_denoise(const std::shared_ptr<insbuff> in); //一路(2ch)降噪
#endif


private:

#ifdef ENABLE_DENOISE_MODE_TL
    std::shared_ptr<insbuff>    get_noise_sample(bool fanless, int index);
    int32_t                     get_ansdb_by_volume(uint32_t volume);
#endif

    void*               					tl_cap_ = nullptr;
    std::vector<void*>  					tl_ans_;
    std::vector<std::shared_ptr<insbuff>> 	ans_in_buff_;
    std::vector<std::shared_ptr<insbuff>> 	ans_out_buff_;
    std::shared_ptr<insbuff> 				cap_in_buff_;
    std::shared_ptr<insbuff> 				cap_out_buff_;
    uint32_t    							frame_size_ = 0;
    bool        							fanless_ = false;	/* 是否为无风扇模式 */
    int32_t     							cur_dn_vol_ = 0;	/* 当前的降噪音量 */

	int32_t									denoise_mode_ = DENOISE_MODE_RNNOISE;		/* 降噪模式:   */
	DenoiseState*							rnnoise_st_;

	
	
};

#endif
