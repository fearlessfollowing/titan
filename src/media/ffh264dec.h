 
#ifndef _FF_H264_DEC_H_
#define _FF_H264_DEC_H_

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

#include <memory>
#include "ins_frame.h"

struct ffh264dec_param
{
	int width = 0;
	int height = 0;
	unsigned char* sps = nullptr;
	unsigned char* pps = nullptr;
	unsigned int sps_len = 0;
	unsigned int pps_len = 0;
	unsigned int threads = 1;
	int buff_cnt = 0;
	AVPixelFormat o_pix_fmt = AV_PIX_FMT_RGBA;
};

struct ffdec_frame
{
	long long pts = 0;
	insbuff2* buff = nullptr;
	// std::shared_ptr<ins_mem_pool> mem_pool;

	// ~ffdec_frame()
	// {
	// 	if (mem_pool && buff)
	// 	{
	// 		mem_pool->queue_buff(buff);
	// 		buff = nullptr;
	// 		mem_pool = nullptr;
	// 	}
	// }
};

class ffh264dec
{
public:
	~ffh264dec() { close(); };
	int open(ffh264dec_param& param);
	//以下都是暂时只支持非plannar的fmt
    std::shared_ptr<ins_img_frame> decode(unsigned char* data, unsigned int size, long long pts, long long dts);
	std::shared_ptr<ffdec_frame> decode2(unsigned char* data, unsigned int size, long long pts, long long dts);
private:
	void close();
	unsigned int width_ = 0;
	unsigned int height_ = 0;
	AVCodec* codec_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
	SwsContext* sw_ctx_ = nullptr;
	AVPicture pic_; 
	AVFrame* frame_ = nullptr;
};

#endif

