
#include <string>
#include "ffaacdec.h"
#include "inslog.h"
#include "ffutil.h"
#include "common.h"

ffaacdec::~ffaacdec()
{
    if (codec_ctx_)
    {
        if (codec_ctx_->extradata)
        {
            delete [] codec_ctx_->extradata;
            codec_ctx_->extradata = nullptr;
        }
        
        avcodec_close(codec_ctx_);
        av_free(codec_ctx_);
        codec_ctx_ = nullptr;
    }
}

int ffaacdec::open(const unsigned char* config, unsigned config_size)
{   
    codec_ = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec_)
    {
        LOGERR("cann't find aac docoder");
        return INS_ERR;
    }
    
    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (!codec_ctx_)
    {
        LOGERR("avcodec_alloc_context fail");
        return INS_ERR;
    }

    codec_ctx_->codec_type = AVMEDIA_TYPE_AUDIO;
    codec_ctx_->codec_id = AV_CODEC_ID_AAC;
    codec_ctx_->thread_count = 1;
    codec_ctx_->refcounted_frames = 1;
    
    codec_ctx_->extradata = new unsigned char[config_size]();
    codec_ctx_->extradata_size = config_size;
    memcpy(codec_ctx_->extradata, config, config_size);
    
    int ret = avcodec_open2(codec_ctx_, codec_, 0);
    if (ret < 0)
    {
        LOGERR("avcodec_open fail ret:%d", ret);
        return INS_ERR;
    }

    return INS_OK;
}

std::shared_ptr<insbuff> ffaacdec::decode(unsigned char* data, unsigned size, long long pts, long long dts)
{  
    int got_frame = 0;
    AVPacket pkt;
    av_init_packet(&pkt);
    
    pkt.data = data;
    pkt.size = size;
    pkt.pts = pts;
    pkt.dts = dts;
    
    AVFrame* frame = av_frame_alloc();
    int ret = avcodec_decode_audio4(codec_ctx_, frame, &got_frame, &pkt);
    if (ret < 0)
    {
        av_frame_free(&frame);
        LOGERR("aac decode fail, ret:%d %s", ret, FFERR2STR(ret));
        return nullptr;
    }
    
    if (!got_frame)
    {
        av_frame_free(&frame);
        //LOGINFO("not got frame");
        return nullptr;
    }

    // LOGINFO("aacframe pts:%lld size:%d %d ch:%d format:%d samplerate:%d samples:%d",
    //     frame->pkt_pts, frame->linesize[0], frame->linesize[1], 
    //     frame->channel_layout, frame->format, frame->sample_rate, frame->nb_samples);
    
    // auto dec_frame = std::make_shared<DecodeFrame>();
    // memcpy(dec_frame->data, frame->data, sizeof(frame->data));
    // memcpy(dec_frame->linesize, frame->linesize, sizeof(frame->linesize));
    // dec_frame->pts = frame->pkt_pts;
    // dec_frame->dts = frame->pkt_dts;

    //auto dec_frame = std::make_shared<ff_frame>(frame);
    
    std::shared_ptr<insbuff> buff;
    if (frame->format == AV_SAMPLE_FMT_FLT)
    {
        buff = std::make_shared<insbuff>(frame->linesize[0]);
        memcpy(buff->data(), frame->data[0], buff->size());
    }
    else if (frame->format == AV_SAMPLE_FMT_FLTP)
    {
        buff = std::make_shared<insbuff>(frame->linesize[0]);
        float* out = (float*)buff->data();
        float* in = (float*)frame->data[0];
        for (int i = 0; i < frame->nb_samples; i++)
        {
            out[2*i] = in[i];
            out[2*i+1] = in[i+frame->nb_samples];
        }
    }
    else
    {
        LOGERR("aac dec format:%d not support now", frame->format);
    }   

    av_frame_unref(frame);
    av_frame_free(&frame);

    return buff;
}
