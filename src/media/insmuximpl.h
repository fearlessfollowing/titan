
#ifndef _INS_MUX_IMPL_H_
#define _INS_MUX_IMPL_H_

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/opt.h"
#include "libavutil/error.h"
#include "libavutil/intreadwrite.h"
}

#include <mutex>
#include "insmux.h"

class ins_mux_impl
{
public:
	int open(const ins_mux_param& param, const char* url);
	void close();
	int write(ins_mux_frame& frame);
	void flush();
	void setIoInterruptResult(int value);
	int io_interrupt_result_ = 0;
private:
	int init_audio(const ins_mux_param& param);
	int init_video(const ins_mux_param& param);
	int init_camm(const ins_mux_param& param);
	std::string guess_format(std::string url);
	
	AVFormatContext* ctx_ = nullptr;
	AVStream* video_stream_ = nullptr;
	AVStream* audio_stream_ = nullptr;
	AVStream* camm_stream_ = nullptr;
	AVRational audio_src_ts_;
	AVRational video_src_ts_;
	AVRational camm_src_ts_;
    bool is_init_ = false;
	bool frame_flush_ = false;
	std::string protocal_; //传输协议
	std::mutex mutex_;
};

#endif

