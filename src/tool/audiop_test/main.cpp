
#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <thread>
#include "inslog.h"

int test_alsa_play()
{
	ins_log::init("./", "log");

	snd_pcm_t* handle_ = NULL;

	int32_t ret = snd_pcm_open(&handle_, "hw:0,7", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) 
    {
        printf("open fail:%d %s\n", ret, snd_strerror(ret));
        return -1;
    }

	snd_pcm_hw_params_t *params_ = NULL;
    snd_pcm_hw_params_malloc(&params_);
    snd_pcm_hw_params_any(handle_, params_);

	//int32_t dir = 0;
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

	snd_pcm_hw_params_get_buffer_size_max(params_, &buffer_size);
	printf("max buff size:%lu\n", buffer_size);
    ret = snd_pcm_hw_params_set_buffer_size_near(handle_, params_, &buffer_size);
	if (ret < 0)
	{
		printf("set buff size fail:%d\n", ret);
		return -1;
	}
	printf("set buff size:%lu\n", buffer_size);

    snd_pcm_hw_params_get_period_size_min(params_, &period_size, NULL);
	printf("min period size:%lu\n", period_size);
	period_size = 1024;
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

	// snd_pcm_sw_params_t *swparams = NULL;
	// snd_pcm_sw_params_alloca(&swparams);
	// snd_pcm_sw_params_current(handle_, swparams);
	// ret = snd_pcm_sw_params_set_avail_min(handle_, swparams, period_size);
	// ret = snd_pcm_sw_params_set_start_threshold(handle_, swparams, 24000);
	// ret = snd_pcm_sw_params_set_stop_threshold(handle_, swparams, 24000);
	// ret = snd_pcm_sw_params(handle_, swparams);

	FILE* fp = fopen("1.pcm", "rb");
	if (!fp)
	{
		printf("file open fail\n");
		return -1;
	} 

	uint32_t size = period_size*2*2;
	uint8_t* buff = (uint8_t*)malloc(size);
	while (1)
	{
		LOGINFO("1");
		int32_t ret = fread(buff, 1, size, fp);
		if (ret != size)
		{
			printf("read over\n");
			break;
		}

		ret = snd_pcm_writei(handle_, buff, period_size);
		if (ret == -EPIPE) 
		{
			LOGINFO("EPIPE------");
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
			LOGINFO("2");
		}
	}

	free(buff);
	fclose(fp);
	snd_pcm_hw_params_free(params_);
	snd_pcm_close(handle_);

	printf("audio play exit\n");

	return 0;
}


int main(int argc, char* argv[])
{   

    return 0;
}
