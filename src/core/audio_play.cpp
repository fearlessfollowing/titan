#include "audio_play.h"
#include "common.h"
#include "inslog.h"
#include <limits.h>
#include "audio_fmt_conv.h"
#include "audio_ace.h"
#include <sys/time.h>
#include "cam_manager.h"
#include "hdmi_monitor.h"

std::atomic_llong audio_play::video_timestamp_(LLONG_MAX);

audio_play::~audio_play()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);

    aec_ = nullptr;

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
    LOGINFO("audio play destroy");
}

int32_t audio_play::open(uint32_t samplerate, uint32_t channel)
{
    if (channel != 1 && channel != 2 && channel != 4)
    {
        LOGERR("audio play not support channel:%d", channel);
        return INS_ERR;
    }

    video_timestamp_ = LLONG_MAX;
    channel_ = channel;
    samplerate_ = samplerate;

    th_ = std::thread(&audio_play::task, this);

    return INS_OK;
}

int32_t audio_play::do_open(uint32_t samplerate, uint32_t channel)
{   
	auto ret = snd_pcm_open(&handle_, "hw:0,7", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) 
    {
        LOGERR("open fail:%d %s", ret, snd_strerror(ret));
        return INS_ERR;
    }

    snd_pcm_hw_params_malloc(&params_);
    snd_pcm_hw_params_any(handle_, params_);

    uint32_t val;
    snd_pcm_uframes_t buffer_size;

	ret = snd_pcm_hw_params_set_access(handle_, params_, SND_PCM_ACCESS_RW_INTERLEAVED);
    RETURN_IF_TRUE(ret < 0, "set access fail", INS_ERR);

    snd_pcm_hw_params_set_format(handle_, params_, SND_PCM_FORMAT_S16_LE); 
    RETURN_IF_TRUE(ret < 0, "set format fail", INS_ERR);

    //只支持2声道
    snd_pcm_hw_params_set_channels(handle_, params_, 2);
    RETURN_IF_TRUE(ret < 0, "set channel fail", INS_ERR);

    // snd_pcm_hw_params_get_rate_max(params_, &val, &dir);
    // LOGINFO("-----rate max:%d dir:%d", val, dir);
    // snd_pcm_hw_params_get_rate_min(params_, &val, &dir);
    // LOGINFO("-----rate min:%d dir:%d", val, dir);

    val = samplerate;
    snd_pcm_hw_params_set_rate_near(handle_, params_, &val, nullptr);
    RETURN_IF_TRUE(ret < 0, "set samplerate fail", INS_ERR);
    LOGINFO("set samplerate:%d", val);

	// snd_pcm_hw_params_get_buffer_size_max(params_, &buffer_size);
	// LOGINFO("max buff size:%d", buffer_size);
    // snd_pcm_hw_params_get_buffer_size_min(params_, &buffer_size);
	// LOGINFO("min buff size:%d", buffer_size);
    buff_cnt_ = 3;
    buffer_size = INS_AUDIO_FRAME_SIZE*buff_cnt_;
    ret = snd_pcm_hw_params_set_buffer_size_near(handle_, params_, &buffer_size);
	RETURN_IF_TRUE(ret < 0, "set buffer size fail", INS_ERR);
	//LOGINFO("set buff size:%d", buffer_size);

    // snd_pcm_hw_params_get_period_size_min(params_, &period_size_, nullptr);
	// LOGINFO("min period size:%d", period_size_);
	period_size_ = INS_AUDIO_FRAME_SIZE;
    ret = snd_pcm_hw_params_set_period_size_near(handle_, params_, &period_size_, nullptr);
	RETURN_IF_TRUE(ret < 0, "set period size fail", INS_ERR);
	//LOGINFO("set period size:%d", period_size_);

    frame_size_ = snd_pcm_format_size(SND_PCM_FORMAT_S16_LE, period_size_*channel);

    ret = snd_pcm_hw_params(handle_, params_);
    RETURN_IF_TRUE(ret < 0, "set hw params fail", INS_ERR);

    aec_ = std::make_shared<audio_ace>();
    aec_->set_frame_cb([this](ins_pcm_frame& frame){ on_frame(frame); });
    aec_->setup(samplerate);

    init_ = true;

    LOGINFO("audio play open samplerate:%u channel:%u period_size:%d buffer_size:%d", samplerate, channel, period_size_, buffer_size);

    return INS_OK;
}

int32_t audio_play::play(std::shared_ptr<insbuff>& buff)
{
    auto ret = snd_pcm_writei(handle_, buff->data(), period_size_);
    if (ret == -EPIPE) 
    {
        LOGINFO("audio play EPIPE------");
        snd_pcm_prepare(handle_);
        return INS_ERR_RETRY;
    } 
    else if (ret < 0) 
    {
        LOGINFO("audio play write error:%d %s", ret, snd_strerror(ret));
        return INS_ERR;
    } 
    else if (ret != period_size_)
    {
        LOGINFO("audio play write frames:%d < req:%d", ret, period_size_);
        return INS_OK;
    }
    else
    {
        return INS_OK;
    }
}

void audio_play::queue_frame(const ins_pcm_frame& frame)
{
    if (!init_) return;

    ins_pcm_frame frame_1ch;
    frame_1ch.pts = frame.pts;
    if (channel_ == 1)
    {
        frame_1ch.buff = frame.buff;
    }
    else if (channel_ == 2)
    {
        frame_1ch.buff = std::make_shared<insbuff>(frame.buff->size()/2);
	    stereo_to_mono_s16(frame.buff, frame_1ch.buff);
    }
    else if (channel_ == 4)
    {
        frame_1ch.buff = std::make_shared<insbuff>(frame.buff->size()/4);
	    ch4_to_mono_s16(frame.buff, frame_1ch.buff);
    }
    else
    {
        return;
    }

    aec_->queue_mic_frame(frame_1ch);
}

void audio_play::on_frame(ins_pcm_frame& frame)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.size() > 500) 
    {
        LOGERR("hdmi audio queue:%d discard", queue_.size());
        return;
    }
    queue_.push(frame);
}

std::shared_ptr<insbuff> audio_play::dequeue_frame()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return nullptr;
    if (video_timestamp_ == LLONG_MAX) return nullptr; 

    int64_t pts = video_timestamp_;
    while (!queue_.empty())
    {
        auto tmp = queue_.front();
        if (tmp.pts > pts + 150000)
        {
            //LOGINFO("audio pts:%ld > video pts:%ld", tmp.pts, pts); 
            return tmp.buff; //不pop,留最后一帧
        }
        else if (tmp.pts + 150000 < pts)
        {
            //LOGINFO("audio pts:%ld < video pts:%ld", tmp.pts, pts);
            if (queue_.size() == 1)
            {
                return tmp.buff;//不pop,留最后一帧
            }
            else
            {
                queue_.pop();
                continue;
            }
        }
        else
        {
            if (queue_.size() > 1) queue_.pop();
            //LOGINFO("audio pts:%ld video pts:%ld", tmp.pts, pts);
            return tmp.buff;
        }
    }

    return nullptr;
}

void audio_play::task()
{
    while (!quit_)
    {
        if (hdmi_monitor::audio_init_)
        {
            if (do_open(samplerate_, channel_) != INS_OK)
            {
                LOGERR("audio play open fail, play task exit");
                return;
            }
            else
            {
                break;
            }
        }
        else
        {
            usleep(20*1000);
        }
    }

    if (quit_) return;

    LOGINFO("audio play task run");

    auto silent_buff = std::make_shared<insbuff>(frame_size_);
    memset(silent_buff->data(), 0x00, silent_buff->size());
    auto ch2_buff = std::make_shared<insbuff>(frame_size_);

    int64_t spk_start_ts = -1;
    int64_t spk_seq = 0;
    int32_t no_frame_cnt = -1;
    int64_t cnt = 0, deleta_total = 0, delta = 0;
    int64_t base_delta = -(int64_t)(buff_cnt_-1)*1000000*INS_AUDIO_FRAME_SIZE/samplerate_;
    uint32_t invalid_cnt = 0;
    while (!quit_)
    {
        auto buff = dequeue_frame();
        if (buff == nullptr)
        {
            //还没开始播放不用插帧
            if (no_frame_cnt != -1) no_frame_cnt++;
            if (no_frame_cnt >= (int32_t)buff_cnt_-1)
            {
                LOGINFO("---insert silent audio frame:%d", no_frame_cnt);
                play(silent_buff); //插入静音帧
                no_frame_cnt = 0;
            }
            else
            {
                usleep(20*1000);
                continue;
            }
        }
        else
        {
            mono_to_stereo_s16(buff, ch2_buff);

            if (spk_start_ts == -1) 
            {
                struct timeval tm;
                gettimeofday(&tm, nullptr);
                spk_start_ts = (int64_t)tm.tv_sec*1000000 + tm.tv_usec;
                //LOGINFO("----------spk start timestamp:%ld", spk_start_ts);
            }
            ins_pcm_frame spk_frame;
            spk_frame.pts = spk_start_ts + period_size_*1000000*spk_seq/samplerate_;
            spk_frame.buff = buff;
            spk_seq++;

            auto r = cnt++%2000;
            if (r > 10 && r <= 20)
            {
                struct timeval tm;
                gettimeofday(&tm, nullptr);
                deleta_total += (int64_t)tm.tv_sec*1000000 + tm.tv_usec - spk_frame.pts;
                if (r == 20)
                {
                    auto tmp = base_delta - deleta_total/10;
                    if (abs(tmp-delta) < (invalid_cnt+2)*1000)
                    {
                        invalid_cnt = 0;
                        delta = tmp;
                        auto real = (int64_t)tm.tv_sec*1000000 + tm.tv_usec - spk_frame.pts + delta;
                        LOGINFO("audio play delte:%ld real:%ld", delta, real);
                    }
                    else
                    {
                        invalid_cnt++;
                        auto real = (int64_t)tm.tv_sec*1000000 + tm.tv_usec - spk_frame.pts + delta;
                        LOGINFO("!!!!!!!audio play invalid delta:%ld last:%ld real:%ld cnt:%u", tmp, delta, real, invalid_cnt);
                    }
                    deleta_total = 0;
                }
            }

            spk_frame.pts -= cam_manager::nv_amba_delta_usec() + delta; 
            play(ch2_buff);
            aec_->queue_spk_frame(spk_frame);
            no_frame_cnt = 0;
        }
    }

    LOGINFO("audio play task exit");
}

