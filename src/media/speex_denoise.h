#ifndef _SPEEX_DE_NOISE_H_
#define _SPEEX_DE_NOISE_H_

#include "speex_preprocess.h"
#include "insbuff.h"

class speex_denoise
{
public:
    ~speex_denoise() { deinit(); };
    int init(int samplerate, int framesize);
    void deinit();
    void process(std::shared_ptr<insbuff>& pcm);

private:
    SpeexPreprocessState* handle_ = nullptr;
};

#endif