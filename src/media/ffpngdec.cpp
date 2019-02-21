
#include "ffpngdec.h"
#include "inslog.h"
#include "common.h"
#include "ffutil.h"

ffpngdec::~ffpngdec()
{
	if (codec_ctx_)
	{
		avcodec_close(codec_ctx_);
		codec_ctx_ = nullptr;
	}

	if (ctx_)
	{
		avformat_close_input(&ctx_);
		ctx_ = nullptr;
	}
}

int ffpngdec::open(std::string filename)
{
	ctx_ = avformat_alloc_context();
	if (!ctx_)
	{
		LOGERR("avformat_alloc_context fail");
		return INS_ERR;
	}

    int ret = avformat_open_input(&ctx_, filename.c_str(),  nullptr,  nullptr);
	if (ret < 0)
	{
		LOGERR("avformat_open_input fail, url:%s ret:%d %s", filename.c_str(), ret, FFERR2STR(ret));
		return INS_ERR;
	}

	ret = avformat_find_stream_info(ctx_, nullptr);
	if (ret < 0)
	{
		LOGERR("avformat_find_stream_info fail");
		return INS_ERR;
	}

	stream_index_ = -1;
	for(unsigned int i = 0; i < ctx_->nb_streams; i++) 
	{
		if(ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
		{
			stream_index_ = i; break;
		}
	}

	if (stream_index_ == -1)
	{
		LOGERR("no video stream");
		return INS_ERR;
	}

	codec_ctx_ = ctx_->streams[stream_index_]->codec;
	codec_ctx_->thread_count = 1;

	AVCodec* codec = avcodec_find_decoder(codec_ctx_->codec_id);
	if (!codec)
	{
		LOGERR("cann't find codecid:0x%x", codec_ctx_->codec_id);
		return INS_ERR;
	}

	if (avcodec_open2(codec_ctx_, codec, nullptr) < 0)
	{
		LOGERR("avcodec_open fail");
		return INS_ERR;
	}

	return INS_OK;
}

std::shared_ptr<ins_img_frame> ffpngdec::decode(std::string filename)
{
	if (INS_OK != open(filename)) return nullptr;

	AVPacket pkt;
	AVFrame* frame = nullptr;
	std::shared_ptr<ins_img_frame> o_frame;
	while(1) 
	{
		av_init_packet(&pkt);

		if (av_read_frame(ctx_, &pkt) < 0) break;

		if (pkt.stream_index != stream_index_ || pkt.size <= 0)
		{
			av_free_packet(&pkt);
			continue;
		}

		int got_frame = false;
		frame = av_frame_alloc();
		if (avcodec_decode_video2(codec_ctx_, frame, &got_frame, &pkt) < 0)
		{
			LOGERR("avcodec_decode_video2 fail");
			break;
		}

		if (!got_frame) 
		{
			LOGERR("png decode not got frame");
			break;
		}

		int plans;
		if ((AVPixelFormat)frame->format == AV_PIX_FMT_RGBA)
		{
			plans = 4;
		}
		else if ((AVPixelFormat)frame->format == AV_PIX_FMT_RGB24)
		{
			plans = 3;
		}
		else
		{
			LOGERR("invalid pixfmt:%d", frame->format);
			break;
		}

		o_frame = std::make_shared<ins_img_frame>();
		o_frame->buff = std::make_shared<insbuff>(frame->height*frame->width*plans);
		o_frame->w = frame->width;
		o_frame->h = frame->height;
		o_frame->fmt = (AVPixelFormat)frame->format;
		//memcpy(o_frame->buff->data(), frame->data[0], o_frame->buff->size());
		for (int i = 0; i < frame->height; i++)
		{
			memcpy(o_frame->buff->data()+frame->width*i*plans, (unsigned char*)frame->data[0]+frame->linesize[0]*i, frame->width*plans);
		}
		
		LOGINFO("png decode width:%d height:%d fmt:%d", codec_ctx_->width, codec_ctx_->height, frame->format);

		break;
	}

	av_free_packet(&pkt);
	if (frame) av_frame_free(&frame);

    return o_frame;
}
