
#include <stdlib.h>
#include "inslog.h"
#include "ffaacenc.h"
#include "ffutil.h"
#include "common.h"

ffaacenc::~ffaacenc()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);

    if (ctx_)
    {
        avcodec_close(ctx_);
        av_free(ctx_);
        ctx_ = nullptr;
    }

    if (av_frame_)
    {
        av_frame_free(&av_frame_);
        av_frame_ = nullptr;
    }
}

int32_t ffaacenc::open(ffaacenc_param& param, bool sync)
{   
    sync_ = sync;
    codec_ = avcodec_find_encoder_by_name("libfdk_aac");
    RETURN_IF_TRUE(!codec_, "find aac encoder fail", INS_ERR);
    
    ctx_ = avcodec_alloc_context3(codec_);
    RETURN_IF_TRUE(!ctx_, "avcodec alloc context fail", INS_ERR);
    
    ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
    ctx_->codec_id = AV_CODEC_ID_AAC;
    ctx_->sample_rate = param.samplerate;
    ctx_->channel_layout =av_get_default_channel_layout(param.channel);//convert_ch_layout(param.ch_layout);
    ctx_->channels = param.channel; //av_get_channel_layout_nb_channels(codec_ctx_->channel_layout);
    ctx_->sample_fmt = (AVSampleFormat)param.samplefmt; // fdk-aac just support s16
    ctx_->frame_size = 1024;
    ctx_->bit_rate = param.bitrate;
    ctx_->time_base.den = param.timescale.den;
    ctx_->time_base.num = param.timescale.num;
    ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    spatial_ = param.spatial;
    
    int32_t ret = avcodec_open2(ctx_, codec_, nullptr);
    if (ret < 0)
    {
        LOGERR("avcodec_open fail ret:%d %s", ret, FFERR2STR(ret));
        return INS_ERR;
    }

    av_frame_ = av_frame_alloc();
    RETURN_IF_TRUE(!av_frame_, "av frame alloc fail", INS_ERR);
    av_frame_->nb_samples = ctx_->frame_size;
    av_frame_->format = ctx_->sample_fmt;
    av_frame_->channel_layout = ctx_->channel_layout;

    frame_pool_ = std::make_shared<obj_pool<ins_frame>>(-1, "aac frame");
    frame_pool_->alloc(64);
    buff_pool_ = std::make_shared<obj_pool<insbuff>>(-1, "aac buff");
    buff_pool_->alloc(64);
    uint32_t size = param.bitrate*ctx_->frame_size*2/param.samplerate/8;
    if (size%4096) size = (size/4096 + 1)*4096; //4096整数倍
    buff_size_ = size;

    if (!sync_)
    {
        th_ = std::thread(&ffaacenc::task, this);
    }

    LOGINFO("ffaac encode samplerate:%d channle:%d bitrate:%d buffsize:%d", ctx_->sample_rate, ctx_->channels, ctx_->bit_rate, buff_size_);
    
    return INS_OK;
}

void ffaacenc::task()
{
    ins_pcm_frame frame;

    while(!quit_)
    {
        if (dequeue_frame(frame))
        {
            do_encode(frame);
        }
        else
        {
            usleep(20*1000);
        }
    }
}

int32_t ffaacenc::encode(const ins_pcm_frame& frame)
{
    if (sync_)
    {
        return do_encode(frame);
    }  
    else
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.size() > 200)
        {
            LOGERR("aac enc queue size:%d discard 10 frame", queue_.size());
            for (int32_t i = 0; i < 10; i++) queue_.pop_front();
        }
        queue_.push_back(frame);
        return INS_OK;
    }
}

int32_t ffaacenc::do_encode(const ins_pcm_frame& frame)
{
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;
    
    av_frame_->pts = frame.pts;
    av_frame_->data[0] = frame.buff->data();
    av_frame_->linesize[0] = frame.buff->size();

    int32_t got_pkt;
    int32_t ret = avcodec_encode_audio2(ctx_, &pkt, av_frame_, &got_pkt);
    if (ret < 0)
    {
        LOGERR("avcodec_encode_audio2 fail:%d %s", ret, FFERR2STR(ret));
        return INS_ERR;
    }
    
    if (!got_pkt)
    {
        LOGINFO("avcodec_encode_audio2 not got pkt");
        return INS_ERR;
    }

    output(pkt);

    av_free_packet(&pkt);

    return INS_OK;
}

void ffaacenc::output(AVPacket& pkt)
{
    if (pkt.size > buff_size_)
    {
        LOGERR("aac pkt size:%d > buff size:%d", pkt.size, buff_size_);
        return;
    }

    auto frame = frame_pool_->pop();
    //auto frame = std::make_shared<ins_frame>();
    frame->buf = buff_pool_->pop();
    if (!frame->buf->data()) frame->buf->alloc(buff_size_);
    frame->buf->set_offset(pkt.size);
    frame->media_type = INS_MEDIA_AUDIO;
    frame->pts = pkt.pts;
    frame->dts = pkt.pts;
    frame->duration = pkt.duration;
    memcpy(frame->buf->data(), pkt.data, pkt.size);

    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& it : sinks_)
    {
        if (!it.second->is_audio_param_set())
        {
            ins_audio_param param;
            param.mime = INS_AAC_MIME;
            param.b_spatial = spatial_;
            param.samplerate = ctx_->sample_rate;
            param.channels = ctx_->channels;
            param.bitrate = ctx_->bit_rate;
            param.a_timebase = ctx_->time_base.den;
            param.config = std::make_shared<insbuff>(ctx_->extradata_size);
            memcpy(param.config->data(), ctx_->extradata, ctx_->extradata_size);
            it.second->set_audio_param(param);
        }

        it.second->queue_frame(frame);
    }
}

bool ffaacenc::dequeue_frame(ins_pcm_frame& frame)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return false;
    frame = queue_.front();
    queue_.pop_front();
    return true;
}

// void ffaacenc::get_config(unsigned char*& config, unsigned int& config_len)
// {
//     config = codec_ctx_->extradata;
//     config_len = codec_ctx_->extradata_size;
// }

// std::shared_ptr<EncodeFrame> ffaacenc::encode_internal(AVFrame* frame)
// {
//     AVPacket* pkt = new AVPacket;
//     av_init_packet(pkt);
//     pkt->data = nullptr;
//     pkt->size = 0;
 
//     int got_pkt;
//     int ret = avcodec_encode_audio2(codec_ctx_, pkt, frame, &got_pkt);
//     if (ret < 0)
//     {
//         LOGERR("avcodec_encode_audio2 fail:%d %s", ret, FFERR2STR(ret));
//         delete pkt;
//         return nullptr;
//     }
    
//     if (!got_pkt)
//     {
//         LOGINFO("avcodec_encode_audio2 not got pkt");
//         delete pkt;
//         return nullptr;
//     }
    
//     auto enc_frame = std::make_shared<EncodeFrame>();
//     enc_frame->data = pkt->data;
//     enc_frame->len = pkt->size;
//     enc_frame->pts = pkt->pts;
//     enc_frame->dts = pkt->dts;
//     enc_frame->private_data = pkt;

//     return enc_frame;
// }


// AVSampleFormat ffaacenc::convert_sample_fmt(int sample_fmt)
// {
//     switch (sample_fmt)
//     {
//         case CM_SAMPLE_FMT_s16:
//             return AV_SAMPLE_FMT_S16;
//         case CM_SAMPLE_FMT_s16p:
//             return AV_SAMPLE_FMT_S16P;
//         case CM_SAMPLE_FMT_f32:
//             return AV_SAMPLE_FMT_FLT;
//         case CM_SAMPLE_FMT_f32p:
//             return AV_SAMPLE_FMT_FLTP;
//         default:
//             return AV_SAMPLE_FMT_NONE;
//     }
// }

// int ffaacenc::convert_ch_layout(int ch_layout)
// {
//     switch (ch_layout)
//     {
//         case CM_CH_LAYOUT_MONO:
//             return AV_CH_LAYOUT_MONO;
//         case CM_CH_LAYOUT_STEREO:
//             return AV_CH_LAYOUT_STEREO;
//         default:
//             return CM_CH_LAYOUT_NONE;
//     }
// }