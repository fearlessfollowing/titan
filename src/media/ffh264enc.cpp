
#include <stdlib.h>
#include "ffh264enc.h"
#include "inslog.h"
#include "ffutil.h"
#include "common.h"

void ffh264enc::close()
{	
    if (codec_ctx_)
    {
        avcodec_close(codec_ctx_);
        av_free(codec_ctx_);
        codec_ctx_ = nullptr;
    }

    free_sw_context();
}

int ffh264enc::open(const ins_enc_option& option)
{   
    codec_ = avcodec_find_encoder_by_name("libx264");
    if (!codec_)
    {
        LOGERR("find libx264 encoder fail");
        return INS_ERR;
    }
    
    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (!codec_ctx_)
    {
        LOGERR("avcodec_alloc_context fail");
        return INS_ERR;
    }
    
    codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx_->codec_id = AV_CODEC_ID_H264;
    codec_ctx_->width = option.width;
    codec_ctx_->height = option.height;
    codec_ctx_->pix_fmt = AV_PIX_FMT_NV12;
    codec_ctx_->max_b_frames = 0;
    codec_ctx_->time_base.den = (int)(option.framerate*1000);
    codec_ctx_->time_base.num = 1000;
    codec_ctx_->ticks_per_frame = 1;
    codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codec_ctx_->gop_size = (int)option.framerate;
    codec_ctx_->thread_count = option.thread_cnt; //default
    //codec_ctx_->level = GetLevel(param.width, param.height);//default
    //codec_ctx_->thread_type = FF_THREAD_FRAME;
    //codec_ctx_->color_range = AVCOL_RANGE_MPEG;
    
    av_opt_set(codec_ctx_->priv_data, "preset", option.preset.c_str(), 0);
    av_opt_set(codec_ctx_->priv_data, "profile", option.profile.c_str(), 0);
    av_opt_set(codec_ctx_->priv_data, "x264-params", "force-cfr=1", 0);
    //av_opt_set(codec_ctx_->priv_data, "tune", "grain", 0);
    
    //if (option.subpel_mb <= 0) codec_ctx_->me_subpel_quality = 0;
    // av_opt_set(codec_ctx_->priv_data, "8x8dct", "0", 0);
    // av_opt_set(codec_ctx_->priv_data, "weightp", "0", 0);
    // av_opt_set(codec_ctx_->priv_data, "aq_mode", "0", 0);
    //av_opt_set(codec_ctx_->priv_data, "rc-lookahead", "2", 0); //不要开启这个7680x7680的时候内存会不够
    //av_opt_set_double(codec_ctx_->priv_data, "crf", 23, 0);

    //ABR
    codec_ctx_->bit_rate = option.bitrate*1000;//bit/s
    codec_ctx_->rc_buffer_size = (int)(codec_ctx_->bit_rate*1.2);
    codec_ctx_->rc_max_rate = codec_ctx_->rc_buffer_size;
    
    int ret = avcodec_open2(codec_ctx_, codec_, nullptr);
    if (ret < 0)
    {
        LOGERR("avcodec_open fail ret:%d %s", ret, FFERR2STR(ret));
        return INS_ERR;
    }
    
    frame_cb_(codec_ctx_->extradata, codec_ctx_->extradata_size, 0, FRAME_FLAG_CONFIG);

    LOGINFO("ffh264enc width:%u height:%u fps:%lf bitrate:%d thread:%d preset:%s profile:%s",
        option.width, 
        option.height, 
        option.framerate, 
        option.bitrate, 
        option.thread_cnt, 
        option.preset.c_str(), 
        option.profile.c_str());
    
	return INS_OK;
}

int ffh264enc::encode(unsigned char* data, long long pts)
{
    AVFrame* frame = nullptr;
    if (data)
    {
        frame = av_frame_alloc();
        frame->pts = pts;
        frame->format = AV_PIX_FMT_NV12;
        frame->width = codec_ctx_->width;
        frame->height = codec_ctx_->height;
        frame->data[0] = data;
        frame->data[1] = data + codec_ctx_->width*codec_ctx_->height;
        //frame->data[2] = data + codec_ctx_->width*codec_ctx_->height*5/4;
        frame->linesize[0] = codec_ctx_->width;
        frame->linesize[1] = codec_ctx_->width;
        //frame->linesize[2] = codec_ctx_->width/2;
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    int got_pkt = 0;
    int ret  = avcodec_encode_video2(codec_ctx_, &pkt, frame, &got_pkt);
    if (ret < 0)
    {
        LOGERR("avcodec_encode_video2 err:%d %s", ret, FFERR2STR(ret));
    }
    else if (got_pkt)
    {
        //LOGINFO("ffenc pts:%lld size:%d", pkt.pts, pkt.size);
        //write_frame(pkt);
        int flag = 0;
        if (pkt.flags & AV_PKT_FLAG_KEY) flag = FRAME_FLAG_KEY;
        frame_cb_(pkt.data, pkt.size, pkt.pts, flag);
    }
    else
    {
        //LOGINFO("not got packet");
    }

    av_free_packet(&pkt);
    if (frame)
    {
        av_frame_free(&frame);
    }

    return got_pkt?INS_OK:INS_ERR;
}

int ffh264enc::encode_rgba(unsigned char* data, long long pts)
{
    if (alloc_sw_context(codec_ctx_->width, codec_ctx_->height) != INS_OK) return INS_ERR;

    AVFrame* frame = nullptr;
    if (data)
    {
        frame = av_frame_alloc();
        frame->pts = pts;
        frame->format = AV_PIX_FMT_YUV420P;
        frame->width = codec_ctx_->width;
        frame->height = codec_ctx_->height;

        unsigned char* data_line[8] = {nullptr};
        int linesize[8] = {0};
        data_line[0] = data;
        linesize[0] = codec_ctx_->width*4;
        av_frame_get_buffer(frame, 32);
        sws_scale(sw_ctx_, data_line, linesize, 0, codec_ctx_->height, frame->data, frame->linesize);
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    int got_pkt;
    int ret  = avcodec_encode_video2(codec_ctx_, &pkt, frame, &got_pkt);
    if (ret < 0)
    {
        LOGERR("avcodec_encode_video2 err:%d %s", ret, FFERR2STR(ret));
    }
    else if (got_pkt)
    {
        LOGINFO("enc out pts:%lld size:%d %x %x", pkt.pts, pkt.size, pkt.data[3], pkt.data[4]);
    }
    else
    {
        LOGINFO("not got packet");
    }

    av_free_packet(&pkt);
    if (frame)
    {
        av_frame_free(&frame);
    }

    return ret;
}

int ffh264enc::alloc_sw_context(int width, int height)
{
    if (sw_ctx_) return INS_OK;

    sw_ctx_ = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if(!sw_ctx_)
    {
        LOGERR("sws_getContext fail");
        return INS_ERR;
    }
    else
    {
        return INS_OK;
    }
}

void ffh264enc::free_sw_context()
{
    if (sw_ctx_)
	{
		sws_freeContext(sw_ctx_);
        sw_ctx_ = nullptr;
	}
}

// void ffh264enc::add_sink(unsigned int index, const std::shared_ptr<sink_interface>& sink)
// {
//     ins_video_param param;
//     param.fps = fps_; 
//     param.bitrate = codec_ctx_->bit_rate/1000;
//     param.width = codec_ctx_->width;
//     param.height = codec_ctx_->height;
//     param.config = new android::ABuffer(codec_ctx_->extradata_size);
// 	memcpy(param.config->data(), codec_ctx_->extradata, codec_ctx_->extradata_size);

//     sink->set_video_param(param);

//     std::lock_guard<std::mutex> lock(mtx_);
//     sink_.insert(std::make_pair(index, sink));

//     //LOGINFO("---- h264enc add sink index:%d", index);
// }


// void ffh264enc::del_sink(unsigned int index)
// {
//     std::lock_guard<std::mutex> lock(mtx_);
//     auto it = sink_.find(index);
//     if (it != sink_.end()) sink_.erase(it);
//     //LOGINFO("---- h264enc del sink index:%d", index);
// }

// void ffh264enc::write_frame(const AVPacket& pkt)
// {
//     auto frame = std::make_shared<ins_frame>();
//     frame->buff = std::make_shared<page_buffer>(pkt.size);
// 	frame->pts = pkt.pts;
// 	frame->dts = frame->pts;
// 	frame->duration = 1000000.0/fps_;
// 	frame->is_key_frame = pkt.flags & AV_PKT_FLAG_KEY;
// 	frame->media_type = INS_MEDIA_VIDEO;
//     memcpy(frame->buff->data(), pkt.data, pkt.size);

//     //LOGINFO("enc out pts:%lld size:%d keyframe:%d", pkt.pts, pkt.size, pkt.flags & AV_PKT_FLAG_KEY);

// 	std::lock_guard<std::mutex> lock(mtx_);
// 	for (auto it = sink_.begin(); it != sink_.end(); it++)
// 	{
// 		it->second->queue_frame(frame);
//     }
// }

