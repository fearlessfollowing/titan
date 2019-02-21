#include "spatial_audio_composer.h"
#include "TwirlingCapture.h"
#include "inslog.h"
#include "common.h"
#include "ffaacdec.h"
#include "mcencode.h"

spatial_audio_composer::~spatial_audio_composer()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int spatial_audio_composer::open(int samplerate, int channels, unsigned bitrate, const unsigned char* config, unsigned config_size)
{
    tl_cap_handle_ = captureInit(channels, 512, channels, samplerate, true);
    if (tl_cap_handle_ == nullptr)
    {
        LOGERR("Twirling capture init fail");
        return INS_ERR;
    }
    captureSet(tl_cap_handle_, true, 1.0);	

    //decoder
    for (int i = 0; i < 2; i++)
    {
        dec_[i] = std::make_shared<ffaacdec>();
        int ret = dec_[i]->open(config, config_size);
        RETURN_IF_NOT_OK(ret);
    }

    //encoder
    sp < AMessage > format = new AMessage();
	format->setString(INS_MC_KEY_MIME, INS_AAC_MIME);
	format->setInt32(INS_MC_KEY_AAC_PROFILE, INS_AAC_OBJ_LC);
	format->setInt32(INS_MC_KEY_SAMPLE_RATE, samplerate);
    format->setInt32(INS_MC_KEY_CHANNEL_CNT, channels);
    format->setInt32(INS_MC_KEY_BITRATE, bitrate);//kb

    enc_ = std::make_shared<mc_encode>();
    enc_->set_spatial_audio(true);
    enc_->set_buff_type(BUFF_TYPE_NORMAL);
    int ret = enc_->open(format);
    RETURN_IF_NOT_OK(ret);
    enc_->start();

    f32_buff_ = std::make_shared<insbuff>(1024*channels*sizeof(float));
    s16_buff_ = std::make_shared<insbuff>(1024*channels*sizeof(short));

    th_ = std::thread(&spatial_audio_composer::task, this);

    LOGINFO("spatial audio open samplerate:%d channel:%d bitrate:%d", samplerate, channels, bitrate);

    return INS_OK;
}

void spatial_audio_composer::set_sink(const std::shared_ptr<sink_interface>& sink)
{
    if (enc_) enc_->add_stream_sink(0, sink);
}

void spatial_audio_composer::task()
{
    while (!quit_)
    {
        std::vector<std::shared_ptr<ins_demux_frame>> v_frame;
        if (!get_frame(v_frame))
        {
            usleep(30*1000);
            continue;
        }
        else
        {
            compose(v_frame);
        }
    }

    //if (enc_) enc_->drain_encode(true, 1000);
}

bool spatial_audio_composer::get_frame(std::vector<std::shared_ptr<ins_demux_frame>>& frame)
{
    if (queue_[0].empty() || queue_[1].empty()) return false;

    for (int i = 0; i < 2; i++)
    {
        std::shared_ptr<ins_demux_frame> frame_tmp;
        queue_[i].dequeue(frame_tmp);
        assert(frame_tmp != nullptr);
        frame.push_back(frame_tmp);
        //LOGINFO("i:%d audio pts:%lld", i, frame_tmp->pts);
    }

    return true;
}

int spatial_audio_composer::compose(const std::vector<std::shared_ptr<ins_demux_frame>>& frame)
{
    assert(frame.size() == 2);

    std::shared_ptr<insbuff> dec_frame[2];
    for (int i = 0; i < 2; i++)
    {
        dec_frame[i] = dec_[i]->decode(frame[i]->data, frame[i]->len, frame[i]->pts, frame[i]->dts);
        if (dec_frame[i] == nullptr)
        {
            LOGERR("aac frame pts:%lld fail", frame[i]->pts);
            return INS_ERR;
        }
    }

    auto frame_4channel = mix_4channel(dec_frame[0]->data(), dec_frame[1]->data(), dec_frame[0]->size());

    int size = f32_buff_->size()/2;
    captureProcess(tl_cap_handle_, 0, 0, 0, (float*)frame_4channel->data(), (float*)f32_buff_->data(), nullptr);
    captureProcess(tl_cap_handle_, 0, 0, 0, (float*)(frame_4channel->data()+size), (float*)(f32_buff_->data()+size), nullptr);

    f32_to_s16(f32_buff_, s16_buff_);

    int ret = enc_->feed_data(s16_buff_->data(), s16_buff_->size(), frame[0]->pts, 1000);
    RETURN_IF_NOT_OK(ret);

    ret = enc_->drain_encode(false, 1000);
    RETURN_IF_NOT_OK(ret);

    return INS_OK;
}

std::shared_ptr<insbuff> spatial_audio_composer::mix_4channel(const unsigned char* in1, const unsigned char* in2, unsigned size)
{
    int frame_size = size/2/sizeof(float);
	auto out = std::make_shared<insbuff>(frame_size*4*sizeof(float));

	float* in1_data = (float*)in1;
	float* in2_data = (float*)in2;
	float* out_data = (float*)out->data();

	for (int i = 0; i < frame_size*2; i+=2)
	{
        out_data[2*i] = in2_data[i+1];  //1号mic
		out_data[2*i+1] = in2_data[i];  //2号mic
		out_data[2*i+2] = in1_data[i+1];//3号mic
		out_data[2*i+3] = in1_data[i];  //4号mic
	}

    return out;
}

void spatial_audio_composer::f32_to_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
    int sample_size = in->size()/sizeof(float);

    float* in_data = (float*)in->data();
    short* out_data = (short*)out->data();

    for (int i = 0; i < sample_size; i++)
    {
        out_data[i] = in_data[i]*32768.0;
    }
}