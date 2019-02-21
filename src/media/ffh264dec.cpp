
#include <string>
#include "ffh264dec.h"
#include "common.h"
#include "inslog.h"

int ffh264dec::open(ffh264dec_param& param)
{
	LOGINFO("ffh264dec w:%u h:%u outpixfmt:%d spslen:%u ppslen:%u buffcnt:%d", 
        param.width, param.height, param.o_pix_fmt, param.sps_len, param.pps_len, param.buff_cnt);
    
    width_ = param.width;
    height_ = param.height;

    avpicture_alloc(&pic_, param.o_pix_fmt, width_, height_);

	codec_ = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec_)
	{
		LOGERR("avcodec_find_decoder fail");
		return INS_ERR;
	}

	codec_ctx_ = avcodec_alloc_context3(codec_);
	if (!codec_ctx_)
	{
		LOGERR("avcodec_alloc_context fail");
		return INS_ERR;
	}

	codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
	codec_ctx_->codec_id = AV_CODEC_ID_H264;
	codec_ctx_->thread_count = param.threads;
	//codec_ctx_->refcounted_frames = 1;
    codec_ctx_->extradata_size = param.sps_len + param.pps_len + 5 + 3 * 2;
    codec_ctx_->extradata = new unsigned char[codec_ctx_->extradata_size]();
    
    codec_ctx_->extradata[0] = 1;//config version
    //codec_ctx_->extradata[1] = 0x66;  // profile
    //codec_ctx_->extradata[2] = 00; //profile compatibility
    //codec_ctx_->extradata[3] = 0x1e; //level
    codec_ctx_->extradata[4] = 0xff; //must be ff
    codec_ctx_->extradata[5] = 0xe1; //&0x1f:sps num
    codec_ctx_->extradata[6] = 0;  //sps len
    codec_ctx_->extradata[7] = param.sps_len; //sps len
    memcpy(codec_ctx_->extradata+8, param.sps, param.sps_len);
    codec_ctx_->extradata[8+param.sps_len] = 1;
    codec_ctx_->extradata[8+param.sps_len+2] = param.pps_len;
    memcpy(codec_ctx_->extradata+param.sps_len+8+3, param.pps, param.pps_len);
    
	int ret = avcodec_open2(codec_ctx_, codec_, nullptr);
	if (ret < 0)
	{
        LOGERR("avcodec_open fail ret:%d", ret);
		return INS_ERR;
	}

    sw_ctx_ = sws_getContext(width_, height_, AV_PIX_FMT_YUV420P, width_, height_, param.o_pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if(!sw_ctx_)
    {
        LOGERR("sws_getContext fail");
        return INS_ERR;
    }

    frame_ = av_frame_alloc();
    if (frame_ == nullptr)
    {
        LOGERR("av_frame_alloc fail");
        return INS_ERR;
    }
    
    // if (param.buff_cnt > 0)
    // {
    //     mem_pool_ = std::make_shared<ins_mem_pool>("ffh264dec");
    //     mem_pool_->alloc(false, param.buff_cnt, param.width*param.height*4);
    // }

	return INS_OK;
}

void ffh264dec::close()
{	
	if (codec_ctx_)
	{
        if (codec_ctx_->extradata)
        {
            delete[] codec_ctx_->extradata;
            codec_ctx_->extradata = nullptr;
        }
        
		avcodec_close(codec_ctx_);
    	av_free(codec_ctx_);
        codec_ctx_ = nullptr;
	}

	if (sw_ctx_)
	{
		sws_freeContext(sw_ctx_);
        sw_ctx_ = nullptr;
	}

    if (frame_)
    {
        av_frame_free(&frame_);
        frame_ = nullptr;
    }

    avpicture_free(&pic_);
}

//flush: data = nullptr
std::shared_ptr<ins_img_frame> ffh264dec::decode(unsigned char* data, unsigned int size, long long pts, long long dts)
{   
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.pts = pts;
	pkt.dts = dts;
    pkt.size = size;
    pkt.data = data;

    int got_frame;
	int ret = avcodec_decode_video2(codec_ctx_, frame_, &got_frame, &pkt);
	if (ret < 0)
	{
        LOGERR("ff decode error ret:%d", ret);
		return nullptr;
	}

	if (!got_frame)
	{
        LOGINFO("not got frame");
		return nullptr;
	}
    
    sws_scale(sw_ctx_, frame_->data,frame_->linesize, 0, height_, pic_.data, pic_.linesize);

    auto dec_frame = std::make_shared<ins_img_frame>();
    dec_frame->w = width_;
    dec_frame->h = height_;
    dec_frame->buff = std::make_shared<insbuff>(pic_.linesize[0]*height_);
    memcpy(dec_frame->buff->data(), pic_.data[0], dec_frame->buff->size());
    
    return dec_frame;
}

std::shared_ptr<ffdec_frame> ffh264dec::decode2(unsigned char* data, unsigned int size, long long pts, long long dts)
{   
    // if (mem_pool_ == nullptr) return nullptr;

    // if (decode(data, size, pts, dts) == nullptr) return nullptr;

    // int loop_cnt = 10;
    // insbuff2* buff = nullptr;
    // while (loop_cnt-- > 0)
    // {
    //     buff = mem_pool_->dequeue_buff(pic_.linesize[0]*height_);
    //     if (buff == nullptr)
    //     {
    //         usleep(20*1000);
    //         continue;
    //     }
    //     else
    //     {
    //         break;
    //     }
    // }

    // if (buff == nullptr) 
    // {
    //     LOGINFO("dec cann't get mem pool");
    //     return nullptr;
    // }

    // auto frame = std::make_shared<ffdec_frame>();
    // frame->pts = frame_->pkt_pts;
    // frame->buff = buff;
    // frame->mem_pool = mem_pool_;
    // memcpy(frame->buff->data(), pic_.data[0], frame->buff->size());
    
    // return frame;

    return nullptr;
}


