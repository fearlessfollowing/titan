

#ifndef _FF_AAC_DEC_H_
#define _FF_AAC_DEC_H_

#include <memory>
extern "C"
{
#include "libavcodec/avcodec.h"
}
#include "insbuff.h"

// struct ff_frame
// {
//     ff_frame(AVFrame* av_frame) : frame(av_frame) {}
//     ~ff_frame()
//     {
//         if (frame)
//         {
//             av_frame_unref(frame);
//             av_frame_free(&frame);
//             frame = nullptr;
//         }
//     }
//     AVFrame* frame = nullptr;
// };

class ffaacdec
{
public:
    ~ffaacdec();
    int open(const unsigned char* config, unsigned config_size);
    std::shared_ptr<insbuff> decode(unsigned char* data, unsigned size, long long pts, long long dts);
    
private:
    AVCodec* codec_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
};

#endif
