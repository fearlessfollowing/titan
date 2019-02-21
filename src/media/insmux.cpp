
#include "insmux.h"
#include "spherical_metadata.h"
#include "ffutil.h"
#include "camera_info.h"

void ins_mux::close()
{	
	if (ctx_ == nullptr) return;

	if (b_head_write_)
	{
		int ret = av_write_trailer(ctx_);
		if (ret < 0) LOGINFO("write tail fail:%d %s", ret, FFERR2STR(ret));
	}

	if (video_stream_ && video_stream_->codec->extradata)
	{
		delete[] video_stream_->codec->extradata;
		video_stream_->codec->extradata = nullptr;
	}

	if (audio_stream_ && audio_stream_->codec->extradata)
	{
		delete[] audio_stream_->codec->extradata;
		audio_stream_->codec->extradata = nullptr;
	}

	if (ctx_->pb != nullptr) //rtsp hls没有pb
	{
		LOGINFO("mux:%s pos:%lld", ctx_->filename, ctx_->pb->pos);
	}

	if (!(ctx_->oformat->flags & AVFMT_NOFILE)) avio_closep(&ctx_->pb);

	if (video_stream_ && video_stream_->codec)
	{
		avcodec_close(video_stream_->codec);
	}

	if (audio_stream_ && audio_stream_->codec)
	{
		avcodec_close(audio_stream_->codec);
	}

	avformat_free_context(ctx_);

	ctx_ = nullptr;

	b_head_write_ = false;
}

int ins_mux::open(const ins_mux_param& param, const char* url)
{
	int ret = 0;

	if (!url)
	{
		LOGERR("mux url is null");
		return INS_ERR;
	}

	if (!param.has_video && !param.has_audio && !param.camm_tracks)
	{
		LOGERR("video audio camm no exist");
		return INS_ERR;
	}

	//av_log_set_level(AV_LOG_TRACE);
	
	std::string format;
	std::string protocal;
	bool flush_packet;
	get_stream_info_from_url(url, format, protocal, flush_packet);
	avformat_alloc_output_context2(&ctx_, nullptr, format.c_str(), url);
	if (!ctx_)
	{
		LOGERR("Could not deduce output format from file extension");
		return INS_ERR;
	}

	//private data
	if (format == "hls")
	{
		av_opt_set(ctx_->priv_data,  "hls_list_size", "10", 0);
		av_opt_set(ctx_->priv_data,  "hls_time", "2", 0);
		av_opt_set(ctx_->priv_data,  "hls_wrap", "30", 0);
	}
	else if (format == "rtsp")
	{
		av_opt_set(ctx_->priv_data,  "rtsp_transport", "tcp", 0);
		av_opt_set(ctx_->priv_data,  "stimeout", "3000000", 0); //io timeout
	}
	else if (format == "dash")
	{
		av_opt_set(ctx_->priv_data,  "window_size", "20", 0);
		av_opt_set(ctx_->priv_data,  "min_seg_duration", "2000", 0); 
		av_opt_set(ctx_->priv_data,  "remove_at_exit", "1", 0);
	}

	//add stream
	if (param.has_video)
	{
		ret = init_video(param);
		if (ret != INS_OK) return ret;
	}

	if (param.has_audio)
	{
		ret = init_audio(param);
		if (ret != INS_OK) return ret;
	}

	for (int32_t i = 0; i < param.camm_tracks; i++)
	{
		ret = init_camm(param);
		if (ret != INS_OK) return ret;
	}

	//set metadata
	if (param.gps_metadata != "")
	{
		av_dict_set(&ctx_->metadata, "xmp", param.gps_metadata.c_str(), 0);
	}

	time_t now = time(0);
	struct tm *now_tm = localtime(&now);
	char now_str[256] = {0};
	strftime(now_str, sizeof(now_str), "%Y-%m-%d %H:%M:%S", now_tm);
	av_dict_set(&ctx_->metadata, "creation_time", now_str, 0);
	av_dict_set(&ctx_->metadata, "make", INS_MAKE, 0);
	av_dict_set(&ctx_->metadata, "model", INS_MODEL, 0);
	av_dict_set(&ctx_->metadata, "encoding_tool", INS_MODEL, 0);

	std::string sf = camera_info::get_sn() + "_" + camera_info::get_fw_ver() + "_" + camera_info::get_ver() + "_" + camera_info::get_m_ver();
	av_dict_set(&ctx_->metadata, "description", sf.c_str(), 0);

	//open file
	if (!(ctx_->oformat->flags & AVFMT_NOFILE))
	{
		AVDictionary* io_opts = nullptr;
		for (auto it = param.io_options.begin(); it != param.io_options.end(); it++)
		{
			av_dict_set(&io_opts, it->first.c_str(), it->second.c_str(), 0);
		}

		//set io timeout
		if (protocal == "rtmp") //rtmp
		{
			av_dict_set(&io_opts, "rwtimeout", "3000000", 0);
		}
		else if (protocal == "tcp")
		{
			av_dict_set(&io_opts, "timeout", "3000000", 0);
		}

		ret = avio_open2(&ctx_->pb, url, AVIO_FLAG_WRITE, nullptr, &io_opts);
		av_dict_free(&io_opts);
		if (ret < 0)
		{
			LOGERR("avio_open fail, ret:%d %s", ret, FFERR2STR(ret));
			return INS_ERR;
		}
	}

	AVDictionary* mux_opts = nullptr;
	for (auto it = param.mux_options.begin(); it != param.mux_options.end(); it++)
	{
		av_dict_set(&mux_opts, it->first.c_str(), it->second.c_str(), 0);
	}

	ret = avformat_write_header(ctx_, &mux_opts);
	av_dict_free(&mux_opts);
	if (ret < 0)
	{
		LOGINFO("avformat_write_header fail:%d %s", ret, FFERR2STR(ret));
		return INS_ERR;
	}

	if (flush_packet) 
	{
		ctx_->flags |= AVFMT_FLAG_FLUSH_PACKETS;
	}
	else 
	{
		ctx_->flags &= ~AVFMT_FLAG_FLUSH_PACKETS;
	}

	b_head_write_ = true;

	return INS_OK;
}

int ins_mux::init_video(const ins_mux_param& param)
{
	LOGINFO("mux:%s video mime:%s width:%u height:%u timescale:%u bitrate:%d configlen:%d", 
		ctx_->filename,
		param.video_mime.c_str(),
		param.width, 
		param.height, 
		param.v_timebase, 
		param.v_bitrate,
		param.v_config_size);

	AVCodecID codec_id;
	if (param.video_mime == INS_H265_MIME)
	{
		codec_id = AV_CODEC_ID_HEVC;
	}
	else 
	{
		codec_id = AV_CODEC_ID_H264;
	}

	AVCodec* video_codec = avcodec_find_decoder(codec_id);
	if (!video_codec)
	{
		LOGERR("avcodec_find_decoder fail");
		return INS_ERR;
	}

	video_stream_ = avformat_new_stream(ctx_, video_codec);
	if (!video_stream_)
	{
		LOGERR("avformat_new_stream video fail");
		return INS_ERR;
	}

	video_stream_->id = ctx_->nb_streams-1;
	video_src_ts_ = AVRational{1, (int)param.v_timebase};
	AVCodecContext* video_ctx = video_stream_->codec;
	video_ctx->bit_rate = param.v_bitrate*1000;
	video_ctx->width = param.width;
	video_ctx->height = param.height;
	video_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	video_ctx->time_base = AVRational{1, 360000}; /* timebase will be overwrite by libavformat */
	video_stream_->time_base = video_ctx->time_base;
	video_stream_->codec->extradata_size = param.v_config_size;
	video_stream_->codec->extradata = new unsigned char[video_stream_->codec->extradata_size]();
	memcpy(video_stream_->codec->extradata, param.v_config, param.v_config_size);

	if (param.spherical_mode.length())
	{
		auto ss = spherical_metadata::video_spherical(param.spherical_mode);
		av_dict_set(&video_stream_->metadata, "spherical_video", ss.c_str(), 0);
	}

	// av_dict_set(&video_stream_->metadata, "language", "eng", 0); //语言码
	// av_dict_set(&video_stream_->metadata, "rotate", "180", 0); //显示旋转
	// av_dict_set(&video_stream_->metadata, "timecode", "1:15:30:1", 0);

	if (ctx_->oformat->flags & AVFMT_GLOBALHEADER)
	{
		video_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	return INS_OK;
}

int ins_mux::init_audio(const ins_mux_param& param)
{
	LOGINFO("mux:%s audio samplerate:%u channels:%u timescale:%u bitrate:%d configlen:%u spatial:%d", 
		ctx_->filename,
		param.samplerate, 
		param.channels, 
		param.a_timebase, 
		param.audio_bitrate,
		param.a_config_size, 
		param.b_spatial_audio);

	AVCodec* audio_codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
	if (!audio_codec)
	{
		LOGERR("avcodec_find_encoder aac fail");
		return INS_ERR;
	}

	audio_stream_ = avformat_new_stream(ctx_, audio_codec);
	if (!audio_stream_)
	{
		LOGERR("avformat_new_stream audio fail");
		return INS_ERR;
	}

	audio_stream_->id = ctx_->nb_streams-1;
	audio_src_ts_ = AVRational{1, (int)param.a_timebase};
	AVCodecContext* audio_ctx = audio_stream_->codec;
	audio_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	audio_ctx->bit_rate = param.audio_bitrate*1000;
	audio_ctx->sample_rate = param.samplerate;
	audio_ctx->channels = param.channels;
	audio_ctx->channel_layout = av_get_default_channel_layout(param.channels);
	audio_ctx->frame_size = 1024;
	audio_ctx->time_base = AVRational{1, (int)param.samplerate};
	audio_stream_->time_base = audio_ctx->time_base;
	audio_stream_->codec->extradata_size = param.a_config_size;
	audio_stream_->codec->extradata = new unsigned char[param.a_config_size];
	memcpy(audio_stream_->codec->extradata, param.a_config, param.a_config_size);

	//暂时通过声道来判断是否全景声
	if (param.b_spatial_audio)
	{
		av_dict_set(&audio_stream_->metadata, "spatial_audio", "1", 0); 
	}

	if (ctx_->oformat->flags & AVFMT_GLOBALHEADER)
	{
		audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	return INS_OK;
}

int ins_mux::init_camm(const ins_mux_param& param)
{
	auto stream = avformat_new_stream(ctx_, nullptr);
	if (!stream)
	{
		LOGERR("avformat_new_stream camm fail");
		return INS_ERR;
	}

	camm_src_ts_ = (AVRational){1, (int)param.c_timebase};
	stream->id = ctx_->nb_streams-1;
	stream->time_base = camm_src_ts_;
	stream->codec->time_base = camm_src_ts_;
	stream->codec->codec_id = AV_CODEC_ID_CAMERA_MOTION_METADATA;
	stream->codec->codec_tag = MKTAG('c','a','m','m');
	stream->codec->codec_type = AVMEDIA_TYPE_DATA;

	if (ctx_->oformat->flags & AVFMT_GLOBALHEADER)
	{
		stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	v_camm_stream_.push_back(stream);

	LOGINFO("mux:%s add camm track timescale:%d", ctx_->filename, param.c_timebase);

	return INS_OK;
}

int ins_mux::write(ins_mux_frame& frame)
{	
	if (!frame.size || !frame.data) return INS_OK;

	std::lock_guard<std::mutex> lock(mutex_);
	
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = frame.data;
	pkt.size = frame.size;
	pkt.pts = frame.pts; 
	pkt.dts = frame.dts;
	pkt.duration = frame.duration;

	if (frame.media_type == INS_MEDIA_AUDIO)
	{
		if (!audio_stream_) return INS_OK;
		pkt.stream_index = audio_stream_->index;
		av_packet_rescale_ts(&pkt, audio_src_ts_, audio_stream_->time_base);
		//LOGINFO("write audio pts:%lld dts:%lld duration:%lld", frame.pts, frame.dts, frame.duration);
	}
	else if (frame.media_type == INS_MEDIA_VIDEO)
	{
		if (!video_stream_) return INS_OK;
		pkt.stream_index = video_stream_->index;
		if (frame.b_key_frame) pkt.flags |= AV_PKT_FLAG_KEY;
		av_packet_rescale_ts(&pkt, video_src_ts_, video_stream_->time_base);
		//LOGINFO("write video pts:%lld dts:%lld duration:%lld", frame.pts, frame.dts, frame.duration);
	}
	else if (frame.media_type == INS_MEDIA_CAMM_GPS) //GPS写在后一个camm track
	{
		if (v_camm_stream_.empty()) return INS_OK;
		auto stream = v_camm_stream_.back();
		pkt.stream_index = stream->index;
		pkt.flags |= AV_PKT_FLAG_KEY; //all be keyframe
		av_packet_rescale_ts(&pkt, camm_src_ts_, stream->time_base);
	}
	else if (frame.media_type == INS_MEDIA_CAMM_GYRO || frame.media_type == INS_MEDIA_CAMM_EXP) //gyro写在第一个camm track
	{
		if (v_camm_stream_.empty()) return INS_OK;
		auto stream = v_camm_stream_.front();
		//gyro的时间间隔太小了,可能和gps/exposure的时间相同，这里+1us处理 还有出现gps/exposure/gyro三者时间戳相同的情况，所以pkt.pts + 1也要判断
		if (pkt.pts == last_camm_pts_ || pkt.pts + 1 == last_camm_pts_) 
		{
			pkt.dts = pkt.pts = last_camm_pts_ + 1;
			//LOGINFO("last pts:%ld = cur:%ld size:%d", last_camm_pts_,frame.pts, frame.size); 
		}
		else if (pkt.pts < last_camm_pts_)
		{
			LOGERR("last pts:%ld > cur:%ld size:%d discard", last_camm_pts_,frame.pts, frame.size);
			return INS_OK;
		}
		last_camm_pts_ = pkt.dts;
		pkt.stream_index = stream->index;
		pkt.flags |= AV_PKT_FLAG_KEY; //all be keyframe
		av_packet_rescale_ts(&pkt, camm_src_ts_, stream->time_base);
		//LOGINFO("write camm pts:%lld duraion:%ld size:%d", pkt.pts, frame.pts, pkt.size);
	}
	else
	{
		return INS_OK;
	}

	int ret = av_interleaved_write_frame(ctx_, &pkt);
	if (ret < 0)
	{
		LOGINFO("write media:%d pts:%lld last:%lld size:%d fail:%d %s", frame.media_type,frame.pts,last_camm_pts_,frame.size,ret, FFERR2STR(ret));
		return INS_ERR;
	}
	else
	{
		if(ctx_->pb) frame.position = ctx_->pb->pos;
		//if (frame_flush_ && ctx_->pb) avio_flush(ctx_->pb); //网络流要flush 保证低延时，文件流不能刷新，不然写性能下降
		return INS_OK;
	}
}

void ins_mux::flush()
{
	if (ctx_->pb) avio_flush(ctx_->pb);
}

void ins_mux::get_stream_info_from_url(std::string url, std::string& format, std::string& protocal, bool& flush_packet)
{
	if (std::string::npos != url.find("rtmp", 0))
	{
		format = "flv";
		protocal = "rtmp";
		flush_packet = true;
	}
	else if (std::string::npos != url.find("udp", 0))
	{
		format = "mpegts";
		protocal = "udp";
		flush_packet = true;
	}
	else if (std::string::npos != url.find("tcp", 0))
	{
		format = "mpegts";
		protocal = "tcp";
		flush_packet = true;
	}
	else if (std::string::npos != url.find("m3u8", 0))
	{
		format = "hls"; //hls 不能每帧都flush
		protocal = "http";
		flush_packet = false;
	}
	else if (std::string::npos != url.find("rtsp", 0))
	{
		format = "rtsp";
		protocal = "rtsp";
		flush_packet = false;
	}
	else if (std::string::npos != url.find("mpd", 0))
	{
		format = "dash";
		protocal = "http";
		flush_packet = false;
	}
	else
	{
		protocal = "file";
		format = "mp4";
		flush_packet = false;
	}

	//LOGINFO("mux:%s protocal:%s format:%s frame flush:%d", url.c_str(), protocal.c_str(), format.c_str(), flush_packet);
}
