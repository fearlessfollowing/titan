#include "file_audio_composer.h"
#include "spatial_audio_composer.h"
#include "inslog.h"
#include "common.h"

int file_audio_composer::open(std::shared_ptr<sink_interface>& sink, bool b_compose, bool b_in_spatial)
{
    sink_ = sink;
    b_in_spatial_ = b_in_spatial;
    if (b_compose)
    {
        spatial_composer_ = std::make_shared<spatial_audio_composer>();
    }

    return INS_OK;
}

void file_audio_composer::set_param(const ins_demux_param& param)
{
    if (spatial_composer_ == nullptr)
    {
        ins_audio_param audio_param;
        audio_param.mime = INS_AAC_MIME;
        audio_param.samplerate = param.samplerate;
        audio_param.channels = param.channels;
        audio_param.bitrate = param.a_biterate;
        audio_param.a_timebase = 1000000;
        audio_param.b_spatial = b_in_spatial_;
        audio_param.config = new android::ABuffer(param.config_len);
        memcpy(audio_param.config->data(), param.config, param.config_len);
        sink_->set_audio_param(audio_param);
    }
    else 
    {
        spatial_composer_->open(param.samplerate, param.channels*2, param.a_biterate*2/1000, param.config, param.config_len);
        spatial_composer_->set_sink(sink_);
    }
}

void file_audio_composer::queue_frame(int index, const std::shared_ptr<ins_demux_frame>& frame)
{
    if (spatial_composer_)
    {
        spatial_composer_->queue_frame(index, frame);
    }
    else
    {   
        auto audio_frame = std::make_shared<ins_frame>();
        audio_frame->media_type = INS_MEDIA_AUDIO;
        audio_frame->pts = frame->pts;
        audio_frame->dts = frame->dts;
        audio_frame->duration = frame->duration;
        audio_frame->buf = std::make_shared<insbuff>(frame->len);
        memcpy(audio_frame->buf->data(), frame->data, frame->len);
        sink_->queue_frame(audio_frame);
    }
}
