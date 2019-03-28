#include "audio_fmt_conv.h"

void stereo_to_mono_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out, bool b_first_channle)
{
	int size = in->size()/2;

	short* in_data = (short*)in->data();
	short* out_data = (short*)out->data();

	int j = 0;
	if (!b_first_channle) j = 1;

	for (int i = 0; i < size/2; i++) {
		out_data[i] = in_data[2*i+j];
	}
}

void mono_to_stereo_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
	int32_t samples = in->size()/2;

	short* in_data = (short*)in->data();
	short* out_data = (short*)out->data();

	for (int32_t i = 0; i < samples; i++) {
		out_data[2*i] = in_data[i];
        out_data[2*i+1] = in_data[i];
	}
}

void ch4_to_stereo_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
    int32_t samples = in->size()/2/4;

	short* in_data = (short*)in->data();
	short* out_data = (short*)out->data();

	for (int32_t i = 0; i < samples; i++) {
		out_data[2*i] = in_data[4*i];
        out_data[2*i+1] = in_data[4*i+1];
	}
}

void ch4_to_mono_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
	int samples = in->size()/4/2;

	short* in_data = (short*)in->data();
	short* out_data = (short*)out->data();

	for (int i = 0; i < samples; i++) {
		out_data[i] = in_data[4*i];
	}
}

void mix_4channel_s16(const std::shared_ptr<insbuff>& in1, const std::shared_ptr<insbuff>& in2, std::shared_ptr<insbuff>& out)
{
    int sample_cnt = in1->size()/sizeof(short);

	short* in1_data = (short*)in1->data();
	short* in2_data = (short*)in2->data();
	short* out_data = (short*)out->data();

	for (int i = 0; i < sample_cnt; i+=2)
	{
        out_data[2*i] =   in2_data[i+1];
		out_data[2*i+1] = in2_data[i];
		out_data[2*i+2] = in1_data[i+1];
		out_data[2*i+3] = in1_data[i];
	}
}

void mix_4channel_f32(const std::vector<std::shared_ptr<insbuff>>& in, std::shared_ptr<insbuff>& out)
{
    int sample_cnt = in[0]->size()/sizeof(float);

	float* in1_data = (float*)in[0]->data();
	float* in2_data = (float*)in[1]->data();
	float* out_data = (float*)out->data();

	for (int i = 0; i < sample_cnt; i+=2)
	{
        out_data[2*i] =   in2_data[i+1]; //1号mic
		out_data[2*i+1] = in2_data[i];   //2号mic
		out_data[2*i+2] = in1_data[i+1]; //3号mic
		out_data[2*i+3] = in1_data[i];   //4号mic
	}
}

void s24_to_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
    int sample_size = in->size()/3;

    unsigned char* in_data = in->data();
	unsigned char* out_data = out->data();

    //24位转成16位：去掉最低位
    for (int i = 0; i < sample_size; i++)
    {
        out_data[i*2] = in_data[i*3+1];
        out_data[i*2+1] = in_data[i*3+2];
    }
}

void f32_to_s16(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
    int sample_cnt = in->size()/sizeof(float);

    float* in_data = (float*)in->data();
    short* out_data = (short*)out->data();
    
    for (int i = 0; i < sample_cnt; i++)
    {
        out_data[i] = (float)in_data[i]*32768.0;
    }
}

void s16_to_f32(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
    int sample_cnt = in->size()/sizeof(short);

    short* in_data = (short*)in->data();
    float* out_data = (float*)out->data();
    
    for (int i = 0; i < sample_cnt; i++) {
        out_data[i] = (float)in_data[i]/32768.0;
    }
}


void f32_to_s16(int16_t *pOut, const float *pIn, size_t sampleCount) 
{
    if (pOut == NULL || pIn == NULL) {
        return;
    }

#if 0	
    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = (short) pIn[i];
    }
#else 
    for (int i = 0; i < sampleCount; i++) {
        pOut[i] = (float)pIn[i]*32768.0;
    }
#endif
	
}

void s16_to_f32(float *pOut, const int16_t *pIn, size_t sampleCount) 
{
    if (pOut == NULL || pIn == NULL) {
        return;
    }

#if 0	
    for (size_t i = 0; i < sampleCount; ++i) {
        *pOut++ = pIn[i];
    }
#else 
    for (int i = 0; i < sampleCount; i++) {
        pOut[i] = (float)pIn[i]/32768.0;
    }
#endif
}


void s16_2ch_to_f32_1ch(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
    int sample_cnt = in->size()/sizeof(short)/2;

    short* in_data = (short*)in->data();
    float* out_data = (float*)out->data();
    
    for (int i = 0; i < sample_cnt; i++) {
        out_data[i] = (float)in_data[2*i]/32768.0;
    }
}

void f32_1ch_to_s16_2ch(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
{
    int sample_cnt = out->size()/sizeof(short)/2;

    float* in_data = (float*)in->data();
    short* out_data = (short*)out->data();
    
    for (int i = 0; i < sample_cnt; i++)
    {
        out_data[2*i] = (float)in_data[i]*32768.0;
		out_data[2*i+1] = out_data[2*i];
    }
}

// void audio_mgr::s16_stero_to_f32_mono(const std::shared_ptr<insbuff>& in, std::shared_ptr<insbuff>& out)
// {
//     int sample_size = in->size()/sizeof(short)/2;

//     short* in_data = (short*)in->data();
//     float* out_data = (float*)out->data();
    
//     for (int i = 0; i < sample_size; i++)
//     {
//         out_data[i] = (float)in_data[2*i]/32768.0;
//     }
// }

// //待测试
// std::shared_ptr<insbuff> audio_mgr::samplefmt32_samplefmt16(const std::shared_ptr<insbuff>& in)
// {
//     int sample_size = in->size()/4;
// 	auto out = std::make_shared<insbuff>(sample_size*2);

//     int* in_data = (int*)in->data();
// 	short* out_data = (short*)out->data();

//     for (int i = 0; i < sample_size; i++)
//     {
//         out_data[i] = in_data[i] >> 16;
//     }

//     return out;
// }