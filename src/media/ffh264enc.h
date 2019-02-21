
#ifndef _FF_H264_ENC_H_
#define _FF_H264_ENC_H_

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
}

#include <map>
#include <mutex>
#include "ins_enc_option.h"

class ffh264enc
{
public:
    ~ffh264enc() { close(); };
	int open(const ins_enc_option& option);
    int encode(unsigned char* data, long long pts);
    int encode_rgba(unsigned char* data, long long pts);
    void set_frame_cb(std::function<void(const unsigned char* data, unsigned size, long long pts, int flag)> frame_cb)
    {
        frame_cb_ = frame_cb;
    }

    enum 
    {
        FRAME_FLAG_CONFIG = 0x01,
        FRAME_FLAG_KEY = 0x02,
    };

private:
    void close();
    int alloc_sw_context(int width, int height);
    void free_sw_context();
    AVCodec* codec_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    SwsContext* sw_ctx_ = nullptr;
    std::function<void(const unsigned char* data, unsigned size, long long pts, int flag)> frame_cb_;
};

#endif

