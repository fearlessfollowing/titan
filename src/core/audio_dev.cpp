#include "audio_dev.h"
#include "common.h"
#include "inslog.h"
#include <memory>
#include <sstream>

//#define FRAME_SIZE 1024

audio_dev::~audio_dev()
{
    if (params_)
    {
        snd_pcm_hw_params_free(params_);
        params_ = nullptr;
    }

    if (handle_)
    {
        snd_pcm_close(handle_);
        handle_ = nullptr;
    }
}

int32_t audio_dev::open(uint32_t card, uint32_t dev)
{
    card_ = card;
    std::stringstream ss;
    ss << "hw:" << card << "," << dev;
    hw_name_ = ss.str();
    auto ret = snd_pcm_open(&handle_, hw_name_.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0) 
    {
        LOGERR("%s open fail:%d %s", hw_name_.c_str(), ret, snd_strerror(ret));
        return INS_ERR;
    }

    snd_pcm_hw_params_malloc(&params_);
    snd_pcm_hw_params_any(handle_, params_);

    //LOGINFO("%s open success", hw_name_.c_str());

    return INS_OK;
}

int32_t audio_dev::set_param(uint32_t samplerate, uint32_t channel, snd_pcm_format_t fmt)
{
    uint32_t val;
    snd_pcm_uframes_t frames;

    auto ret = snd_pcm_hw_params_set_access(handle_, params_, SND_PCM_ACCESS_RW_INTERLEAVED);
    RETURN_IF_TRUE(ret < 0, "set access fail", INS_ERR);

    snd_pcm_hw_params_set_format(handle_, params_, fmt); //SND_PCM_FORMAT_S16_LE
    RETURN_IF_TRUE(ret < 0, "set format fail", INS_ERR);

    snd_pcm_hw_params_set_channels(handle_, params_, channel);
    RETURN_IF_TRUE(ret < 0, "set channel fail", INS_ERR);

    val = samplerate;
    snd_pcm_hw_params_set_rate_near(handle_, params_, &val, nullptr);
    RETURN_IF_TRUE(ret < 0, "set samplerate fail", INS_ERR);

    frames = INS_AUDIO_FRAME_SIZE;
    snd_pcm_hw_params_set_period_size_near(handle_, params_, &frames, nullptr);
    RETURN_IF_TRUE(ret < 0, "set period size fail", INS_ERR);

    // val = 3;
    // snd_pcm_hw_params_set_periods(handle_, params_, val, dir);
    // RETURN_IF_TRUE(ret < 0, "set period fail", INS_ERR);

    ret = snd_pcm_hw_params(handle_, params_);
    RETURN_IF_TRUE(ret < 0, "set hw params fail", INS_ERR);

    frame_bytes_ = snd_pcm_format_size(fmt, INS_AUDIO_FRAME_SIZE*channel);

    //LOGINFO("-----%s fmt width:%d size:%d", hw_name_.c_str(), snd_pcm_format_width(fmt), frame_size_);

    // snd_pcm_hw_params_get_period_size(params_,&frames, &dir);
    // LOGINFO("period size:%d", frames);
    // snd_pcm_hw_params_get_period_time(params_,&val, &dir);
    // LOGINFO("period time:%d", val);
    // snd_pcm_hw_params_get_periods(params_, &val, &dir);
    // LOGINFO("periods:%d", val);
    // snd_pcm_hw_params_get_buffer_time(params_, &val, &dir);
    // LOGINFO("buffer time:%d", val);

    // snd_pcm_hw_params_get_rate(params_, &val, &dir);
    // LOGINFO("------samplereate:%d", val);
    // snd_pcm_hw_params_get_channels(params_, &val);
    // LOGINFO("------channel:%d", val);

    return INS_OK;
}

int32_t audio_dev::get_param(uint32_t& samplerate, uint32_t& channel, snd_pcm_format_t& fmt)
{
    uint32_t val;

    auto ret = snd_pcm_hw_params_get_channels_max(params_, &val);
    RETURN_IF_TRUE(ret < 0, "get channel", INS_ERR);
    channel = val;

    snd_pcm_hw_params_get_rate_max(params_, &val, nullptr);
    RETURN_IF_TRUE(ret < 0, "get samplerate", INS_ERR);
    samplerate = val;

    ret = snd_pcm_hw_params_test_format(handle_, params_, SND_PCM_FORMAT_S24_3LE);
    if (ret < 0) {
        ret = snd_pcm_hw_params_test_format(handle_, params_, SND_PCM_FORMAT_S16_LE);
        RETURN_IF_TRUE(ret < 0, "fmts16le not support", INS_ERR);
        fmt = SND_PCM_FORMAT_S16_LE;
    } else {
        fmt = SND_PCM_FORMAT_S24_3LE;
    }

    return INS_OK;
}

int32_t audio_dev::read(std::shared_ptr<insbuff>& buff)
{
    auto ret = snd_pcm_readi(handle_, buff->data(), INS_AUDIO_FRAME_SIZE);
    if (ret == -EPIPE) {
        LOGINFO("%s EPIPE------", hw_name_.c_str());
        snd_pcm_prepare(handle_);
        return INS_ERR_RETRY;
    } else if (ret < 0) {
        LOGINFO("%s read error:%d %s", hw_name_.c_str(), ret, snd_strerror(ret));
        return INS_ERR;
    } else if (ret != INS_AUDIO_FRAME_SIZE) {
        LOGINFO("%s read frame:%d < req:%d", hw_name_.c_str(), ret, INS_AUDIO_FRAME_SIZE);
        return INS_OK;
    } else {
        return INS_OK;
    }
}

bool audio_dev::is_spatial()
{
    // //H2N 才是全景声
    char* name = nullptr;
    snd_card_get_name(card_, &name);
    std::string s_name = name;
    if (s_name != INS_H2N_AUDIO_DEV_NAME && s_name != INS_H3VR_AUDIO_DEV_NAME) {
        LOGINFO("-----name:%s not spatial audio dev", name);
        return false;
    }

    //H2N全景声是4通道
    // uint32_t val;
    // snd_pcm_hw_params_get_channels_max(params_, &val);
    // if (val != 4) {
    //     LOGINFO("-----channle:%d not 4", val);
    //     return false;
    // }

    // //H2N全景声是48000采样
    // int32_t dir;
    // snd_pcm_hw_params_get_rate_max(params_, &val, &dir);
    // if (val != 48000) 
    // {
    //     LOGINFO("-----samplerate:%d not 48000", val);
    //     return false;
    // }

    //H2N全景声都是S24LE
    // ret = snd_pcm_hw_params_test_format(handle_, params_, SND_PCM_FORMAT_S24_3LE);
    // if (ret < 0) 
    // {
    //     LOGINFO("-----samplfmt not s24");
    //     return false;
    // }

    uint32_t samplerate;
    uint32_t channel;
    snd_pcm_format_t fmt;
    auto ret = get_param(samplerate, channel, fmt);
    if (ret != INS_OK || samplerate != 48000 || channel != 4 || fmt != SND_PCM_FORMAT_S24_3LE)
    {
        LOGINFO("snd:%s samplerate:%u channle:%u not spatial", name, samplerate, channel);
        return false;
    }

    if (s_name == INS_H3VR_AUDIO_DEV_NAME)  return true; //H3-VR 4声道都是全景声

    set_param(samplerate , channel, fmt);

    if (is_channel_3_silence()) {
        //LOGINFO("%s is spatial audio", hw_name_.c_str());
        return true;
    } else {
        //LOGINFO("%s is not spatial", hw_name_.c_str());
        return false;
    }
}

bool audio_dev::is_channel_3_silence()
{
    auto buff = std::make_shared<insbuff>(frame_bytes_);

	for (int i = 0; i < 2; i++) {
		auto ret = read(buff);
		RETURN_IF_NOT_OK(ret);

		//现在只判断H2n的全景声：如果第三个声道也就是z：全为0， 那就可以判断为全景声
		uint8_t* p = buff->data();
		for (unsigned j = 6; j < buff->size(); j += 12) {
			if (p[j] != 0 || p[j+1] != 0 || p[j+2] != 0) {
                //LOGINFO("i:%d j:%d value:%x %x %x", i, j, p[j], p[j+1], p[j+2]);
                return false;
            }
		}
	}

    return true;
}