#ifndef _AUDIO_FMT_CONV_H_
#define _AUDIO_FMT_CONV_H_

#include <memory>
#include <vector>
#include "insbuff.h"

void stereo_to_mono_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out, bool b_first_channle = true);
void mix_4channel_f32(const std::vector<std::shared_ptr<insbuff>>& in, std::shared_ptr<insbuff>& out);
void mix_4channel_s16(const std::shared_ptr<insbuff>& in1, const std::shared_ptr<insbuff>& in2, std::shared_ptr<insbuff>& out);
void s24_to_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
void f32_to_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
void s16_to_f32(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
void s16_2ch_to_f32_1ch(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
void mono_to_stereo_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
void ch4_to_stereo_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
void f32_1ch_to_s16_2ch(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);
void ch4_to_mono_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out);

#endif