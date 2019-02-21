#ifndef _FF_PNG_DEC_H_
#define _FF_PNG_DEC_H_

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <memory>
#include <string>
#include "insbuff.h"
#include "ins_frame.h"

class ffpngdec
{
public:
    ~ffpngdec();
    std::shared_ptr<ins_img_frame> decode(std::string filename);

private:
    int open(std::string filename);
    AVFormatContext* ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    int stream_index_ = -1;
};

#endif