
#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <thread>
#include "inslog.h"
#include "audio_fmt_conv.h"
#include <mutex>
#include <queue>
#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

#define FRAME_SIZE 512

std::mutex mtx_capture_;
std::mutex mtx_play_;
std::mutex mtx_aec_;
std::queue<std::shared_ptr<insbuff>> capture_queue_;
std::queue<std::shared_ptr<insbuff>> play_queue_;
std::queue<std::shared_ptr<insbuff>> aec_queue_;
bool aec_run_ = false;
SpeexEchoState* st_ = nullptr;

void queue_frame(const std::shared_ptr<insbuff>& buff, std::string type)
{
	if (type == "capture")
	{
		std::lock_guard<std::mutex> lock(mtx_capture_);
		auto buff_1ch = std::make_shared<insbuff>(buff->size()/2);
		stereo_to_mono_s16(buff, buff_1ch);
		capture_queue_.push(buff_1ch);
	}
	else if (type == "play")
	{
		std::lock_guard<std::mutex> lock(mtx_play_);
		play_queue_.push(buff);
	}
	else if (type == "aec")
	{
		std::lock_guard<std::mutex> lock(mtx_aec_);
		aec_queue_.push(buff);
	}
}

std::shared_ptr<insbuff> dequeue_frame(std::string type)
{
	if (type == "capture")
	{
		std::lock_guard<std::mutex> lock(mtx_capture_);
		if (capture_queue_.empty()) return nullptr;
		auto buff = capture_queue_.front();
		capture_queue_.pop();
		return buff;
	}
	else if (type == "play")
	{
		std::lock_guard<std::mutex> lock(mtx_play_);
		if (play_queue_.empty()) return nullptr;
		auto buff = play_queue_.front();
		play_queue_.pop();
		return buff;
	}
	else if (type == "aec")
	{
		std::lock_guard<std::mutex> lock(mtx_aec_);
		if (aec_queue_.empty()) return nullptr;
		auto buff = aec_queue_.front();
		aec_queue_.pop();
		return buff;
	}
	else
	{
		return nullptr;
	}
}

int capture_task()
{
	snd_pcm_t* handle_ = nullptr;
    auto ret = snd_pcm_open(&handle_, "hw:1,0", SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0) 
    {
        LOGERR("capture dev open fail:%d %s", ret, snd_strerror(ret));
        return -1;
    }

	snd_pcm_hw_params_t *params_ = nullptr;
    snd_pcm_hw_params_malloc(&params_);
    snd_pcm_hw_params_any(handle_, params_);

    uint32_t val;
    snd_pcm_uframes_t frames;

    ret = snd_pcm_hw_params_set_access(handle_, params_, SND_PCM_ACCESS_RW_INTERLEAVED);
    RETURN_IF_TRUE(ret < 0, "set access fail", -1);

    snd_pcm_hw_params_set_format(handle_, params_, SND_PCM_FORMAT_S16_LE); 
    RETURN_IF_TRUE(ret < 0, "set format fail", -1);

    snd_pcm_hw_params_set_channels(handle_, params_, 2);
    RETURN_IF_TRUE(ret < 0, "set channel fail", -1);

    val = 48000;
    snd_pcm_hw_params_set_rate_near(handle_, params_, &val, nullptr);
    RETURN_IF_TRUE(ret < 0, "set samplerate fail", -1);
	LOGINFO("set rate:%d", val);

    frames = FRAME_SIZE;
    snd_pcm_hw_params_set_period_size_near(handle_, params_, &frames, nullptr);
    RETURN_IF_TRUE(ret < 0, "set period size fail", -1);
	LOGINFO("set period size:%d", frames);

    ret = snd_pcm_hw_params(handle_, params_);
    RETURN_IF_TRUE(ret < 0, "set hw params fail", -1);

	LOGINFO("capture dev open success");

	while (1)
	{
		auto buff = std::make_shared<insbuff>(FRAME_SIZE*2*2);
		auto ret = snd_pcm_readi(handle_, buff->data(), FRAME_SIZE);
		if (ret == -EPIPE) 
		{
			LOGINFO("read EPIPE------");
			snd_pcm_prepare(handle_);
			continue;
		} 
		else if (ret < 0) 
		{
			LOGINFO("read error:%d %s", ret, snd_strerror(ret));
			break;
		} 
		else if (ret != FRAME_SIZE)
		{
			LOGINFO("read frame:%d < req:FRAME_SIZE", ret);
			break;
		}
		else
		{
			queue_frame(buff, "capture");
			continue;
		}
	}

	snd_pcm_hw_params_free(params_);
	snd_pcm_close(handle_);

	LOGINFO("capture task exit");
}

int play_task()
{
	usleep(300*1000);

	snd_pcm_t* handle_ = nullptr;

	int32_t ret = snd_pcm_open(&handle_, "hw:0,7", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) 
    {
        printf("open fail:%d %s\n", ret, snd_strerror(ret));
        return -1;
    }

	snd_pcm_hw_params_t *params_ = nullptr;
    snd_pcm_hw_params_malloc(&params_);
    snd_pcm_hw_params_any(handle_, params_);

    uint32_t val;
    snd_pcm_uframes_t buffer_size, period_size;

	ret = snd_pcm_hw_params_set_access(handle_, params_, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0)
	{
		printf("set access fail:%d\n", ret);
		return -1;
	}

    snd_pcm_hw_params_set_format(handle_, params_, SND_PCM_FORMAT_S16_LE); 
    if (ret < 0)
	{
		printf("set format fail:%d\n", ret);
		return -1;
	}

    snd_pcm_hw_params_set_channels(handle_, params_, 2);
    if (ret < 0)
	{
		printf("set channel fail:%d\n", ret);
		return -1;
	}

    val = 48000;
    snd_pcm_hw_params_set_rate_near(handle_, params_, &val, nullptr);
    if (ret < 0)
	{
		printf("set samplerate fail:%d\n", ret);
		return -1;
	}

	//snd_pcm_hw_params_get_buffer_size_max(params_, &buffer_size);
	//printf("max buff size:%lu\n", buffer_size);
	buffer_size = FRAME_SIZE*3;
    ret = snd_pcm_hw_params_set_buffer_size_near(handle_, params_, &buffer_size);
	if (ret < 0)
	{
		printf("set buff size fail:%d\n", ret);
		return -1;
	}
	printf("set buff size:%lu\n", buffer_size);

    snd_pcm_hw_params_get_period_size_min(params_, &period_size, NULL);
	printf("min period size:%lu\n", period_size);
	period_size = FRAME_SIZE;
    ret = snd_pcm_hw_params_set_period_size_near(handle_, params_, &period_size, NULL);
	if (ret < 0)
	{
		printf("set period size fail:%d\n", ret);
		return -1;
	}
	printf("set period size:%lu\n", period_size);

    ret = snd_pcm_hw_params(handle_, params_);
    if (ret < 0)
	{
		printf("snd_pcm_hw_params fail:%d\n", ret);
		return ret;
	}

	LOGINFO("play dev open success");

	int32_t delay_cnt = 500;
	int32_t cnt = 0;
	std::shared_ptr<insbuff> s16_2ch_buff = std::make_shared<insbuff>(FRAME_SIZE*2*2);
	while (1)
	{
		std::shared_ptr<insbuff> buff;
		if (cnt < delay_cnt)
		{
			if (cnt == 0) 
			{
				delay_cnt = capture_queue_.size() + 2;
				LOGINFO("-----------------delay cnt:%d", delay_cnt);
			}
			if ((cnt >= 1) || (capture_queue_.size() >= 10))
			{
				LOGINFO("play no aec frame cnt:%d", cnt);
				buff = dequeue_frame("capture");
				cnt++;
			}
			else
			{
				LOGINFO("play no frame");
			}
		}
		else
		{
			buff = dequeue_frame("aec");
		}
 
		if (buff == nullptr)
		{
			usleep(5*1000);
			continue;
		}

		mono_to_stereo_s16(buff, s16_2ch_buff);

		while (1)
		{
			ret = snd_pcm_writei(handle_, s16_2ch_buff->data(), period_size);
			if (ret == -EPIPE) 
			{
				LOGINFO("write EPIPE------");
				snd_pcm_prepare(handle_);
			}
			else if (ret < 0) 
			{
				printf("write error:%d %s\n", ret, snd_strerror(ret));
				return -1;
			} 
			else if (ret != period_size)
			{
				printf("write frames:%d < req:%lu\n", ret, period_size);
			}
			else
			{
				queue_frame(buff, "play");
				if (cnt == delay_cnt) aec_run_ = true;
				break;
			}
		}
	}

	snd_pcm_hw_params_free(params_);
	snd_pcm_close(handle_);

	printf("play task exit\n");
}

int main(int argc, char* argv[])
{  
	ins_log::init("./", "log");

	int32_t samplerate = 48000;
    st_ = speex_echo_state_init(FRAME_SIZE, FRAME_SIZE*10);
	//st_ = speex_echo_state_init_mc(FRAME_SIZE, FRAME_SIZE*6, 2, 2);
    speex_echo_ctl(st_, SPEEX_ECHO_SET_SAMPLING_RATE, &samplerate);

    auto den_ = speex_preprocess_state_init(FRAME_SIZE, samplerate);
    speex_preprocess_ctl(den_, SPEEX_PREPROCESS_SET_ECHO_STATE, st_);

	std::thread capture_th(capture_task);
	std::thread play_th(play_task);

	int32_t cnt = 0;
	while (1)
	{
		if (!aec_run_) 
		{
			usleep(10*1000);
			continue;
		}

		auto buff = dequeue_frame("capture");
		if (buff == nullptr)
		{
			usleep(5*1000);
			continue;
		}

		std::shared_ptr<insbuff> play_buff; 
		while (1)
		{
			play_buff = dequeue_frame("play");
			if (play_buff == nullptr)
			{
				usleep(1000*2);
				continue;
			}
			else
			{
				break;
			}
		}

		LOGINFO("aec process aec queue:%d cap queue:%d play queue:%d", aec_queue_.size(), capture_queue_.size(), play_queue_.size());
		auto out_buff = std::make_shared<insbuff>(play_buff->size());
		speex_echo_cancellation(st_, (spx_int16_t*)buff->data(), (spx_int16_t*)play_buff->data(), (spx_int16_t*)out_buff->data());
      	speex_preprocess_run(den_, (spx_int16_t*)out_buff->data());
		//queue_frame(out_buff, "aec");
		queue_frame(buff, "aec");
	}

	speex_echo_state_destroy(st_);
	speex_preprocess_state_destroy(den_);

    return 0;
}
