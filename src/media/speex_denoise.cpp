
#include "speex_preprocess.h"
#include "common.h"
#include "inslog.h"
#include "speex_denoise.h"

int speex_denoise::init(int samplerate, int framesize)
{
    handle_ = speex_preprocess_state_init(framesize, samplerate);
	if (handle_ == nullptr)
	{
        LOGERR("speex_preprocess_state_init fail");
		return INS_ERR;
	}
	
	int i=1;
	speex_preprocess_ctl(handle_, SPEEX_PREPROCESS_SET_DENOISE, &i);
	i = 0;
	speex_preprocess_ctl(handle_, SPEEX_PREPROCESS_SET_AGC, &i);
	i = 80000;
	speex_preprocess_ctl(handle_, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);
	// i = -40;
    // speex_preprocess_ctl(handle_, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &i);

    i = 0;
    speex_preprocess_ctl(handle_, SPEEX_PREPROCESS_SET_DEREVERB, &i);
    float f = .0;
    speex_preprocess_ctl(handle_, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
    f = .0;
    speex_preprocess_ctl(handle_, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);

    LOGINFO("------speex denoise init samplerate:%d framesize:%d", samplerate, framesize);

    return INS_OK;
}

void speex_denoise::deinit()
{
    if (handle_)
    {
        speex_preprocess_state_destroy(handle_);
        handle_ = nullptr;
    }
}

void speex_denoise::process(std::shared_ptr<insbuff>& pcm)
{
    if (handle_ == nullptr) return;

    speex_preprocess_run(handle_, (short*)pcm->data());
}

#if 0
/* frame size:����һ�δ�����PCM sample��, ����Ϊ20ms������
   ����, ������44100, ����������ôframesie = 44100*1*20/1000(20ms) */
SpeexPreprocessState* SpeexDnInit(unsigned int samplerate, unsigned int framesize)
{
	SpeexPreprocessState *st;
	int i = 0;
	
	st = speex_preprocess_state_init(framesize, samplerate);
	if (!st)
	{
		return NULL;
	}
	
	i=1;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &i);
	i=0;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &i);
	i=80000;
	speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);
	i = -40;
        speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &i);
	//i = 1;
   	//speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_VAD, &i);
	return st;
}

void SpeexDnDeInit(SpeexPreprocessState* st)
{
	if (!st)
	{
		return;
	}
	
	speex_preprocess_state_destroy(st);
}

/* pIoPcmΪ������PCM����,��СΪframesize, ���봦����������Ҳ�����ڸ�λ�� */
int SpeexDnProcess(SpeexPreprocessState* st, short* pIoPcm)
{
	if (!st || !pIoPcm)
	{
		return -1;
	}
	
	return speex_preprocess_run(st, pIoPcm);
}

void Put32(unsigned int v, FILE* fp)
{	
		fputc( v        & 0xff, fp);
    fputc((v >>  8) & 0xff, fp);
    fputc((v >> 16) & 0xff, fp);
    fputc((v >> 24) & 0xff, fp);
}

void Put16(short v, FILE* fp)
{
		fputc( v       & 0xff, fp);
    fputc((v >> 8) & 0xff, fp);
}

void wav_head(FILE* fp)
{	
    fputs("RIFF", fp);
    Put32(0, fp);                /* headlen+datalen */
    fputs("WAVEfmt ", fp);
    Put32(18, fp);               /* fmtsize */
    Put16(0x0001, fp);           /* pcm */
    Put16(1, fp);     /* channel */
    Put32(44100, fp);   /* samplerate */
    Put32(2 * 44100 * 1, fp);
    Put16(2 * 1, fp);
    Put16(2 * 8, fp);
    Put16(0, fp);
    fputs("data", fp);
    Put32(0, fp);                /* datalen */
}

viod wav_tail(FILE* fp, unsigned int len)
{
	fflush(fp);
	fseek(fp, 4, SEEK_SET);
	Put32(46 + len, fp);

	
	/* ��ͷ��д�����ݵĳ��� */
	fseek(fp, 42, SEEK_SET);
	Put32(len, fp);
}

#define FRAEME_SIZE 44100*1*20/1000

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("no enough param");
		return -1;
	}	

	short in[FRAEME_SIZE];
	SpeexPreprocessState *st;
	int count=0;
	unsigned int len = 0;

	FILE* fpIn = fopen(argv[1], "rb");
	if (!fpIn)
	{
		printf("open in.pcm fail\r\n");
		return -1;
	}
	
	fseek(fpIn,44,SEEK_SET);

	FILE* fpOut = fopen(argv[2], "wb");
	if (!fpOut)
	{
		printf("open out.pcm fail\r\n");
		fclose(fpIn);
		return -1;
	}
	
	wav_head(fpOut);

	/* init */
	st = SpeexDnInit(44100, FRAEME_SIZE);

	while (1)
	{
		if (fread(in, sizeof(short), FRAEME_SIZE, fpIn) <= 0)
		{
			break;
		}

		/* process */
		SpeexDnProcess(st, in);

		fwrite(in, sizeof(short), FRAEME_SIZE, fpOut);

		count++;
		len += FRAEME_SIZE;
	}

	/* deinit */
	SpeexDnDeInit(st);

	printf("count:%d\r\n", count);
	
	wav_tail(fpOut, len);

	fclose(fpIn);
	fclose(fpOut);

	return 0;
}
#endif


