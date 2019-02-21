
extern "C" 
{
#include "libavutil/intreadwrite.h"
}

#include "insdemux.h"
#include "ffutil.h"
#include "inserr.h"
#include <unistd.h>
#include <arpa/inet.h>

#define INS_DEMUX_PRI_NONE     0
#define INS_DEMUX_PRI_FRAME    1
#define INS_DEMUX_PRI_PIC      2
#define INS_DEMUX_PRI_PKT      3
#define INS_DEMUX_PRI_ARRY     4
#define INS_DEMUX_PRI_AVFREE   5

ins_demux_frame::~ins_demux_frame()
{
	if (!private_data) return;
    
    switch (private_type)
    {
        case INS_DEMUX_PRI_PKT:
		{
            AVPacket* pkt = (AVPacket*)private_data;
            av_free_packet(pkt);
			delete pkt;
            break;
		}
        case INS_DEMUX_PRI_AVFREE:
        {
		    unsigned char* data = (unsigned char*)private_data;
            av_free(data);
            break;
		}
        default:
            break;
    }
    
    private_data = nullptr;
}

static int read_cb(int err, const char* filename)
{
	LOGINFO("error %d endpoint not connect", err);
	usleep(15000*1000);
	return 0;
}

void ins_demux::close()
{
	//LOGINFO("DEMUX close");
	
	if (ctx_)
	{
		avformat_close_input(&ctx_);
		ctx_ = nullptr;
	}

	if (video_bfs_)
	{
		av_bitstream_filter_close(video_bfs_);
		video_bfs_ = nullptr;
	}

	if (audio_bfs_)
	{
		av_bitstream_filter_close(audio_bfs_);
		audio_bfs_ = nullptr;
	}
	
	if (sps_)
	{
		delete[] sps_;
		sps_ = nullptr;
        sps_len_ = 0;
	}
    
    if (pps_)
    {
        delete[] pps_;
        pps_ = nullptr;
        pps_len_ = 0;
    }
}

int ins_demux::open(const char* url, bool annexb, std::map<std::string, std::string>* options)
{
	if (!url)
	{
		LOGERR("demux url is null");
		return INS_DEMUX_ERR;
	}

	int ret = 0;
	is_out_annexb_ = annexb;
	video_bfs_ = av_bitstream_filter_init("h264_mp4toannexb");
	audio_bfs_ = av_bitstream_filter_init("aac_adtstoasc");

	ctx_ = avformat_alloc_context();
	if (ctx_ == nullptr)
	{
		LOGERR("avformat_alloc_context fail");
		return INS_DEMUX_ERR;
	}

	AVDictionary* opt = nullptr;
	if (options)
	{
		for (auto it = options->begin(); it != options->end(); it++)
		{
			av_dict_set(&opt, it->first.c_str(), it->second.c_str(), 0);
		}
	}

	//av_dict_set(&opt, "multiple_requests", "0", 0);
	// ctx_->interrupt_callback.callback = interruptCallBack;
	// ctx_->interrupt_callback.opaque = this;

	ret = avformat_open_input(&ctx_, url, nullptr, &opt);
	if (ret < 0)
	{
		LOGERR("avformat_open_input fail, ret:%d %s", ret, FFERR2STR(ret));
		return INS_DEMUX_ERR;
	}
    
	// for (unsigned int i = 0; i < ctx_->nb_streams; i++)
	// {
	// 	AVStream *st = ctx_->streams[i];
	// 	if ((st->codec->codec_type != AVMEDIA_TYPE_VIDEO) && (st->codec->codec_type != AVMEDIA_TYPE_AUDIO))
	// 	{
	// 		st->discard = AVDISCARD_ALL;
	// 	}
	// }
    
	ret = avformat_find_stream_info(ctx_, nullptr);
	if (ret < 0)
	{
		LOGERR("avformat_find_stream_info fail, ret:%d %s", ret, FFERR2STR(ret));
		return INS_DEMUX_ERR;
	}

	ret = av_find_best_stream(ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (ret >= 0)
	{
		video_stream_index_ = ret;
	}

	ret = av_find_best_stream(ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (ret >= 0)
	{
		audio_stream_index_ = ret;
	}

	ret = av_find_best_stream(ctx_, AVMEDIA_TYPE_DATA, -1, -1, nullptr, 0);
	if (ret >= 0)
	{
		auto codec = ctx_->streams[ret]->codec;
		if (codec->codec_id == AV_CODEC_ID_CAMERA_MOTION_METADATA && codec->codec_tag == MKTAG('c','a','m','m'))
		{
			camm_stream_index_ = ret;	
		}
	}

	parse_sps_pps();
	parse_audio_config();

	return INS_DEMUX_OK;
}

void ins_demux::get_param(ins_demux_param& param)
{
	LOGINFO("demux url:%s",ctx_->filename);

	if (video_stream_index_ != -1)
	{
		AVCodecContext* codec_ctx = ctx_->streams[video_stream_index_]->codec;

		param.has_video = true;
		param.width = codec_ctx->width;
		param.height = codec_ctx->height;
		param.fps = (double)ctx_->streams[video_stream_index_]->avg_frame_rate.num/(double)ctx_->streams[video_stream_index_]->avg_frame_rate.den;
		param.v_biterate = codec_ctx->bit_rate;
		param.sps = sps_;
		param.sps_len = sps_len_;
        param.pps = pps_;
		param.pps_len = pps_len_;
		param.v_frames = ctx_->streams[video_stream_index_]->nb_frames;
        
        if (ctx_->streams[video_stream_index_]->duration)
        {
            param.v_duration = (double) ctx_->streams[video_stream_index_]->duration * ctx_->streams[video_stream_index_]->time_base.num / (double)ctx_->streams[video_stream_index_]->time_base.den;
        }
        else
        {
            param.v_duration = (double)ctx_->duration/(double)AV_TIME_BASE;
        }

		LOGINFO("video: width:%u height:%u fps:%lf biterate:%u spslen:%u ppslen:%u duration:%lf frames:%lld",
			param.width, 
			param.height, 
			param.fps, 
			param.v_biterate, 
			param.sps_len, 
			param.pps_len,
			param.v_duration, 
			param.v_frames);
	}

	if (audio_stream_index_ != -1)
	{
		AVCodecContext* codec_ctx = ctx_->streams[audio_stream_index_]->codec;

		param.has_audio = true;
		param.samplerate = codec_ctx->sample_rate;
		param.channels = codec_ctx->channels;
		param.a_biterate = codec_ctx->bit_rate;
		param.config = ctx_->streams[audio_stream_index_]->codec->extradata;
		param.config_len = ctx_->streams[audio_stream_index_]->codec->extradata_size;
		param.a_frames = ctx_->streams[audio_stream_index_]->nb_frames;
        
        if (ctx_->streams[audio_stream_index_]->duration)
        {
            param.a_duration = (double) ctx_->streams[audio_stream_index_]->duration * ctx_->streams[audio_stream_index_]->time_base.num / (double)ctx_->streams[audio_stream_index_]->time_base.den;
        }
        else
        {
            param.a_duration = (double)ctx_->duration/(double)AV_TIME_BASE;
        }

		LOGINFO("audio: samplerate:%u, channels:%u, bitrate:%u configlen:%u duration:%lf chlayout:%ld samplefmt:%d frames:%d",
			param.samplerate, 
			param.channels, 
			param.a_biterate, 
			param.config_len, 
			param.a_duration, 
			codec_ctx->channel_layout, 
			codec_ctx->sample_fmt, 
			param.a_frames);
	}

	if (camm_stream_index_ != -1)
	{
		param.has_camm = true;
		LOGINFO("camm track");
	}
}

int ins_demux::seek_frame(double time, unsigned char media_type)
{
	if (!ctx_->iformat->read_seek && !ctx_->iformat->read_seek2)
	{
		LOGERR("stream unseekable");
		return INS_DEMUX_ERR;
	}

	//seek default
    int stream_index = -1;

    if (media_type == INS_DEMUX_AUDIO)
    {
    	stream_index = audio_stream_index_;
    }
    else if (media_type == INS_DEMUX_VIDEO)
    {
    	stream_index = video_stream_index_;
    }
    // else if (media_type == INS_DEMUX_SUBTITLE)
    // {
    // 	stream_index = subtitle_stream_index;
    // }

    std::lock_guard<std::mutex> lock(mutex_);
	long long timestamp = (long long)(time * ctx_->streams[stream_index]->time_base.den/ctx_->streams[stream_index]->time_base.num);
	LOGINFO("seek to time:%lf timestamp:%lld", time, timestamp);

	int ret = av_seek_frame(ctx_, stream_index, timestamp, AVSEEK_FLAG_BACKWARD);
	if (ret < 0)
	{	
		LOGERR("seek fail, ret:%d %s", ret, FFERR2STR(ret));
		return INS_DEMUX_ERR;
	}
	else
	{
		return INS_DEMUX_OK;
	}
}

char* ins_demux::get_metadata(const char* key)
{
    AVDictionaryEntry* entry = av_dict_get(ctx_->metadata, key, NULL, 0);
    if (!entry || !entry->value)
    {
        LOGERR("cann't get metadata key:%s", key);
        return nullptr;
    }

	LOGINFO("metadata %s:%s", key, entry->value);
    
    return entry->value;
}

int ins_demux::get_next_frame(std::shared_ptr<ins_demux_frame>& frame)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    //pkt 是否释放 都在 parseAudioFrame parseVideoFrame里处理
	AVPacket* pkt = new AVPacket;
	av_init_packet(pkt);
	
	while (1)
	{
        int ret = av_read_frame(ctx_, pkt);
		if (ret < 0)
		{
			av_free_packet(pkt);
			delete pkt;
            if (ret == AVERROR_EOF)
            {
                LOGINFO("DEMUX frame read over");
                return INS_ERR_OVER;
            }
            else
            {
                LOGINFO("DEMUX frame read error:%d %s", ret, FFERR2STR(ret));
                return INS_ERR_FILE_IO;//暂时只用这一个错误代替
            }
		}

		if (audio_stream_index_ == pkt->stream_index)
		{
			AVRational dst_tb = {1, 1000000}; //转换成微秒时间单位
			av_packet_rescale_ts(pkt, ctx_->streams[audio_stream_index_]->time_base, dst_tb);
			std::shared_ptr<ins_demux_frame> demux_frame = parse_audio_frame(pkt);
			if (demux_frame == nullptr)
			{
				continue;
			}
			else
			{
                frame = demux_frame;
				return INS_DEMUX_OK;
			}
		}
		else if (video_stream_index_ == pkt->stream_index)
		{
			AVRational dst_tb = {1, 1000000}; //转换成微秒时间单位
			av_packet_rescale_ts(pkt, ctx_->streams[video_stream_index_]->time_base, dst_tb);
			std::shared_ptr<ins_demux_frame> demux_frame = parse_video_frame(pkt);
			if (demux_frame == nullptr)
			{
				continue;
			}
			else
			{
                frame = demux_frame;
                return INS_DEMUX_OK;
			}
		}
		else if (camm_stream_index_ == pkt->stream_index)
		{
			// int pkt_type = AV_RL16(((uint16_t*)pkt->data) + 1);
			AVRational dst_tb = {1, 1000000}; //转换成微秒时间单位
			av_packet_rescale_ts(pkt, ctx_->streams[camm_stream_index_]->time_base, dst_tb);

			frame = std::make_shared<ins_demux_frame>();
			frame->media_type = INS_MEDIA_CAMM;
			frame->pts = pkt->pts;
			frame->dts = pkt->dts;
			frame->duration = std::max(0, pkt->duration);
			frame->is_key = pkt->flags & AV_PKT_FLAG_KEY;
			frame->data = pkt->data;
			frame->len = pkt->size;
			frame->private_data = (void*)pkt;
			frame->private_type = INS_DEMUX_PRI_PKT;
			return INS_DEMUX_OK;
		}
		else
		{
			//LOGINFO("read frame undeal index:%d", pkt->stream_index, ctx_->streams[pkt->stream_index]);
			av_free_packet(pkt);
			continue;
		}
	}
}

std::shared_ptr<ins_demux_frame> ins_demux::parse_audio_frame(AVPacket* pkt)
{
	auto frame = std::make_shared<ins_demux_frame>();
	
    frame->media_type = INS_MEDIA_AUDIO;
	frame->pts = pkt->pts;
	frame->dts = pkt->dts;
	frame->duration = pkt->duration;
    frame->is_key = pkt->flags & AV_PKT_FLAG_KEY;

	//adts
	if ((pkt->data[0] == 0xff) && ((pkt->data[1] & 0xf0) == 0xf0))
	{
		int ret = av_bitstream_filter_filter(audio_bfs_, ctx_->streams[audio_stream_index_]->codec, nullptr, &frame->data, (int*)&frame->len, pkt->data, pkt->size, pkt->flags & AV_PKT_FLAG_KEY);
        
        //error
        if (ret < 0)
        {
            av_free_packet(pkt);
            LOGERR("audio bit stream filter fail, err:%d %s", ret, FFERR2STR(ret));
            return nullptr;
        }
        //out buffer point to in buffer
        else if (ret == 0)
        {
            frame->private_data = (void*)pkt;
            frame->private_type = INS_DEMUX_PRI_PKT;
        }
        // out buffer allocted by filter
        else
        {
            av_free_packet(pkt);
            delete pkt;
            frame->private_data = frame->data;
            frame->private_type = INS_DEMUX_PRI_AVFREE;
        }
	}
	else
	{
		frame->data = pkt->data;
		frame->len = pkt->size;
		frame->private_data = (void*)pkt;
        frame->private_type = INS_DEMUX_PRI_PKT;
	}

	//LOGINFO("demux audio len:%u pts:%llu dts:%llu", frame->len, frame->pts, frame->dts);

	frame->duration = std::max(frame->duration, (long long)0);

	return frame;
}

std::shared_ptr<ins_demux_frame> ins_demux::parse_video_frame(AVPacket* pkt)
{
	auto frame = std::make_shared<ins_demux_frame>();
	
	frame->media_type = INS_MEDIA_VIDEO;
	frame->pts = pkt->pts;
	frame->dts = pkt->dts;
	frame->duration = pkt->duration;
    frame->is_key = pkt->flags & AV_PKT_FLAG_KEY;

	unsigned char* data = pkt->data;
	unsigned int len = pkt->size;

	if (INS_DEMUX_OK != parse_nal(data, len))
	{
		av_free_packet(pkt);
		return nullptr;
	}
	
	if (is_in_annexb_)
	{
		if (is_out_annexb_)
		{
			frame->data = data;
			frame->len = len;
			frame->private_data = (void*)pkt;
        	frame->private_type = INS_DEMUX_PRI_PKT;
		}
		else
		{
			frame->data = (unsigned char*)av_malloc(len+100); //多100字节预留保护
			frame->private_data = (void*)frame->data;
        	frame->private_type = INS_DEMUX_PRI_AVFREE;
			annexb_to_mp4(data, len, frame->data, frame->len);
			av_free_packet(pkt);
			delete pkt;
		}
	}
	else
	{
		if (is_out_annexb_)
		{
			int ret = av_bitstream_filter_filter(video_bfs_, ctx_->streams[video_stream_index_]->codec, nullptr, &frame->data, (int*)&frame->len, data, len, pkt->flags & AV_PKT_FLAG_KEY);
            //error
            if (ret < 0)
            {
                av_free_packet(pkt);
                LOGERR("video bit stream filter fail, err:%d %s", ret, FFERR2STR(ret));
                return nullptr;
            }
            //out buffer point to in buffer
            else if (ret == 0)
            {
                frame->private_data = (void*)pkt;
        		frame->private_type = INS_DEMUX_PRI_PKT;
            }
            // out buffer allocted by filter
            else
            {
                av_free_packet(pkt);
                delete pkt;
                frame->private_data = (void*)frame->data;
        		frame->private_type = INS_DEMUX_PRI_AVFREE;
            }
		}
		else
		{
			frame->data = data;
			frame->len = len;
			frame->private_data = (void*)pkt;
        	frame->private_type = INS_DEMUX_PRI_PKT;
		}
	}

	frame->duration = std::max(frame->duration, (long long)0);

	//LOGINFO("demux video len:%u pts:%llu dts:%llu", frame->len, frame->pts, frame->dts);

	return frame;
}

void ins_demux::annexb_to_mp4(unsigned char* annexb, unsigned int annexb_len, unsigned char* mp4, unsigned int& mp4_len)
{
	int start_pos = 4;
	int mp4_pos = 0;
	
	for (unsigned int i = 4; i < annexb_len; i++)
	{
		if (((annexb[i] == 0) && (annexb[i+1] == 0) && (annexb[i+2] == 0) && (annexb[i+3] == 1)) || ((annexb[i] == 0) && (annexb[i+1] == 0) && (annexb[i+2] == 1)))
		{
			int startcode_len = 4;
			
			if ((annexb[i] == 0) && (annexb[i+1] == 0) && (annexb[i+2] == 1))
			{
				startcode_len = 3;
			}
			
			*((unsigned int*)(mp4+mp4_pos)) = htonl(i-start_pos);
			mp4_pos += 4;
			memcpy(mp4+mp4_pos, annexb+start_pos, i-start_pos);
			mp4_pos += i-start_pos;
			i += startcode_len;
			start_pos = i;
		}
	}

	*((unsigned int*)(mp4+mp4_pos)) = htonl(annexb_len-start_pos);
	mp4_pos += 4;
	memcpy(mp4+mp4_pos, annexb+start_pos, annexb_len-start_pos);
	mp4_len = mp4_pos + annexb_len - start_pos;
}

int ins_demux::parse_nal(unsigned char*& data, unsigned int& len)
{
    bool got_gyro = false;
    
	while (1)
	{
        int nal_type = 0;
        int start_code_len = 0;
        if (is_in_annexb_)
        {
            if ((data[0] == 0) && (data[1] == 0) && (data[2] == 1))
            {
                nal_type = data[3] & 0x1f;
                start_code_len = 3;
            }
            else if ((data[0] == 0) && (data[1] == 0) && (data[2] == 0) && (data[3] == 1))
            {
                nal_type = data[4] & 0x1f;
                start_code_len = 4;
            }
            else
            {
                LOGERR("nal not start with 0001 or 001");
                return INS_DEMUX_ERR;
            }
        }
        else
        {
            nal_type = data[4] & 0x1f;
            start_code_len = 4;
        }
        
        if ((nal_type == 5) || (nal_type == 1))
        {
            break;
        }
        
		//strip all sps pps sei frame
        if (is_in_annexb_)
        {
            if (!step_to_next_nal(data, len))
            {
                return INS_DEMUX_ERR;
            }
        }
        else
        {
            unsigned int nal_len = ntohl(*((unsigned int*)data)) + 4;
            
            if (len < nal_len)
            {
                return INS_DEMUX_ERR;
            }
				
            data += nal_len;
            len -= nal_len;
        }
	}

	return INS_DEMUX_OK;
}

bool ins_demux::step_to_next_nal(unsigned char*& data, unsigned int& len)
{
	for (unsigned int i = 3; i < len - 4; i++)
	{
		if (((data[i] == 0) && (data[i+1] == 0) && (data[i+2] == 1)) 
			|| ((data[i] == 0) && (data[i+1] == 0) && (data[i+2] == 1) && (data[i+3] == 1)))
		{
			data += i;
			len -= i;
			return true;
		}
	}

	return false;
}

int ins_demux::nal_startcode_len(unsigned char* data)
{
	if ((data[0] == 0) && (data[1] == 0) && (data[2] == 1)) 
	{
		return 3;
	}
	
	if ((data[0] == 0) && (data[1] == 0) && (data[2] == 0) && (data[3] == 1))
	{
		return 4;
	}

	return 0;
}


void ins_demux::parse_sps_pps()
{ 
	if (video_stream_index_ == -1) return;

	unsigned char* extra_data = ctx_->streams[video_stream_index_]->codec->extradata;
	int extra_data_len = ctx_->streams[video_stream_index_]->codec->extradata_size;
    
	// printf("-------------:");
	// for (int i  = 0; i < extra_data_len; i++)
	// {
	// printf("%x ", extra_data[i]);
	// }
	// printf("\n");
    
    if (!extra_data || !extra_data_len)
    {
        LOGERR("no video extra data, cann't get sps pps");
        return;
    }
    
    if ((extra_data[0] == 0 && extra_data[1] == 0 && extra_data[2] == 0 && extra_data[3] == 1)
        || (extra_data[0] == 0 && extra_data[1] == 0 && extra_data[2] == 1))
    {
        int i =  0;
        int sps_start_pos = 0;
        int sps_end_pos = 0;
        int pps_start_pos = 0;
        int pps_end_pos = 0;
        int startcode_len = 0;
        
        //sps
        for (i = 0; i < extra_data_len - 4; i++)
        {
            if (extra_data[i] == 0 && extra_data[i+1] == 0 && extra_data[i+2] == 0 && extra_data[i+3] == 1)
            {
                startcode_len = 4;
            }
            else if (extra_data[i] == 0 && extra_data[i+1] == 0 && extra_data[i+2] == 1)
            {
                startcode_len = 3;
            }
            else
            {
                continue;
            }
            
            if (sps_start_pos)
            {
                sps_end_pos = i;
                break;
            }
            
            if ((extra_data[i+startcode_len] & 0x1f) == 7)
            {
                sps_start_pos = i + startcode_len;
            }
            
            if (startcode_len == 4)
            {
                i++;
            }
        }
        
        //pps
        for (i = 0; i < extra_data_len - 4; i++)
        {
            if (extra_data[i] == 0 && extra_data[i+1] == 0 && extra_data[i+2] == 0 && extra_data[i+3] == 1)
            {
                startcode_len = 4;
            }
            else if (extra_data[i] == 0 && extra_data[i+1] == 0 && extra_data[i+2] == 1)
            {
                startcode_len = 3;
            }
            else
            {
                continue;
            }
            
            if (pps_start_pos)
            {
                pps_end_pos = i;
                break;
            }
            
            if ((extra_data[i+startcode_len] & 0x1f) == 8)
            {
                pps_start_pos = i + startcode_len;
            }
            
            if (startcode_len == 4)
            {
                i++;
            }
        }
        
        if (!pps_end_pos)
        {
            pps_end_pos = extra_data_len;
        }
        
        if (sps_end_pos > sps_start_pos)
        {
            sps_len_ = sps_end_pos - sps_start_pos;
            sps_ = new unsigned char[sps_len_]();
            memcpy(sps_, extra_data+sps_start_pos, sps_len_);
        }
        
        if (pps_end_pos > pps_start_pos)
        {
            pps_len_ = pps_end_pos - pps_start_pos;
            pps_ = new unsigned char[pps_len_]();
            memcpy(pps_, extra_data+pps_start_pos, pps_len_);
        }
        
        is_in_annexb_ = true;
        
        //LOGINFO("-------------------video extradata is annexb");
    }
    else
    {
        /* extra data ∏Ò Ω: 6th bit&&1f: number of sps unit  7 8 bit: sps len  */
        unsigned char* pos = extra_data;
        
        sps_len_ = ntohs(*((unsigned short*)(pos+6)));
        pps_len_ = ntohs(*((unsigned short*)(pos+6+2+sps_len_+1)));
        
        sps_ = new unsigned char[sps_len_]();
        pps_ = new unsigned char[pps_len_]();
        
        memcpy(sps_, pos+6+2, sps_len_);
        memcpy(pps_, pos+6+2+sps_len_+3, pps_len_);
        
        is_in_annexb_ = false;
        
        //LOGINFO("---------------------video extradata is not annexb");
    }
}
 
void ins_demux::parse_audio_config()
{
	if (audio_stream_index_ == -1) return;
	
	AVPacket pkt;
	av_init_packet(&pkt);
	unsigned char* data = nullptr;
	int len = 0;

	while (1)
	{
        if (ctx_->streams[audio_stream_index_]->codec->extradata) break;
        
		int ret = av_read_frame(ctx_, &pkt);
		if (ret < 0)
		{
			LOGERR("cann't read frame");
			av_free_packet(&pkt);
			return;
		}

		if (audio_stream_index_ != pkt.stream_index)
		{
			av_free_packet(&pkt);
			continue;
		}

		//adts
		if ((pkt.data[0] == 0xff) && ((pkt.data[1] & 0xf0) == 0xf0))
		{
			av_bitstream_filter_filter(audio_bfs_, ctx_->streams[audio_stream_index_]->codec, nullptr, &data, &len, pkt.data, pkt.size, pkt.flags & AV_PKT_FLAG_KEY);
		}

		av_free_packet(&pkt);
	}

	if (!ctx_->streams[audio_stream_index_]->codec->extradata)
	{
		LOGERR("no audio extra data, cann't get audio config");
	}
}


