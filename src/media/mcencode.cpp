
#include "mcencode.h"
#include "inslog.h"

void mc_encode::close()
{
	if (encode_ != nullptr)
	{
		encode_->stop();
		encode_->release();
		encode_.clear();
	}

	if (looper_ != nullptr)
	{
		looper_->stop();
		looper_.clear();
	}
}

int mc_encode::open(const sp<AMessage>& format)
{	
	status_t status;

	looper_ = new ALooper();
	looper_->setName("mc_encode_looper");
	looper_->start();

	AString mime;
	format->findString("mime", &mime);
	format->findInt32(INS_MC_KEY_BITRATE, (int*)&bitrate_);
	format->setInt32(INS_MC_KEY_BITRATE, bitrate_*1000);
	if (mime.startsWithIgnoreCase("video/"))
	{
		format->findFloat(INS_MC_KEY_FRAME_RATE, &fps_);
		media_type_ = INS_MEDIA_VIDEO;
	}
	else if (mime.startsWithIgnoreCase("audio/"))
	{
		media_type_ = INS_MEDIA_AUDIO;
		format->findInt32(INS_MC_KEY_SAMPLE_RATE, &samplerate_);
	}
	else
	{
		LOGERR("mime:%s neither video nor audio", mime.c_str());
		return INS_ERR;
	}

	encode_ = MediaCodec::CreateByType(looper_, mime.c_str(), true, &status);
	MC_CHECK_NULL_RETURN(encode_, INS_ERR, "mcencode createBytype fail");
	MC_CHECK_ERR_RETURN(status, INS_ERR, "mc_encode createbytype fail");

	status = encode_->configure(format, nullptr, nullptr, MediaCodec::CONFIGURE_FLAG_ENCODE);
	MC_CHECK_ERR_RETURN(status, INS_ERR, "mcencode config fail");

	LOGINFO("mcencode mime:%s open fps:%f samplerate:%d", mime.c_str(), fps_, samplerate_);
	
	return INS_OK;
}

Surface* mc_encode::create_input_surface()
{
	sp<IGraphicBufferProducer> producer;
	status_t status = encode_->createInputSurface(&producer);
	MC_CHECK_ERR_RETURN(status, nullptr, "mc_encode createinputsurface fail");
	surface_ = new Surface(producer);
	return surface_.get();
}

int mc_encode::start()
{
	status_t status = encode_->start();
	MC_CHECK_ERR_RETURN(status, INS_ERR, "mc_encode start fail");
	return INS_OK;
}

// int mc_encode::stop()
// {
// 	status_t status = encode_->stop();
// 	MC_CHECK_ERR_RETURN(status, INS_ERR, "mc_encode stop fail");
	
// 	return INS_OK;
// }

int mc_encode::feed_data(const unsigned char* data, unsigned int size, long long pts, int timeout_us)
{
	status_t status;
	unsigned int offset = 0;
	bool flush = false;
	
	if (data == nullptr || size == 0)
	{
		flush = true;
	}

	while (offset < size || flush)
	{
		size_t index;
		status = encode_->dequeueInputBuffer(&index, timeout_us);
		if (status != OK)
		{
			if (status == -EAGAIN)
			{	
				//LOGINFO("mcencode dequeueInputBuffer try again");
				continue;
			}
			LOGERR("mcencode dequeueInputBuffer err:%d", status);
			return INS_ERR;
		}

		sp<ABuffer> buff;
		status = encode_->getInputBuffer(index, &buff);
		MC_CHECK_ERR_RETURN(status, INS_ERR, "mcencode getInputBuffer fail");
		MC_CHECK_NULL_RETURN(buff, INS_ERR, "mcencode getInputBuffer is null");
		//LOGINFO("inbuff offset:%d size:%d capacity:%d", buff->offset(), buff->size(), buff->capacity());

		if (flush)
		{
			status = encode_->queueInputBuffer(index, 0, 0, 0, MediaCodec::BUFFER_FLAG_EOS);
			return INS_OK;
		}
		else
		{
			unsigned int cur_size = std::min((unsigned)buff->capacity(), size - offset);
			memcpy(buff->base(), data+offset, cur_size);
			buff->setRange(0, cur_size);
			offset += cur_size;
			status = encode_->queueInputBuffer(index, 0, buff->size(), pts, 0);
		}

		MC_CHECK_ERR_RETURN(status, INS_ERR, "mcencode queueInputBuffer fail");
	}

	return INS_OK;
}

int mc_encode::drain_encode(bool eos, int timeout_us)
{
	status_t status;

	if (eos) encode_->signalEndOfInputStream();

	while (1)
	{
		size_t index, out_offset, out_size; 
		int64_t  pts;
		uint32_t flags;
		status = encode_->dequeueOutputBuffer(&index, &out_offset, &out_size, &pts, &flags, timeout_us);
		if (status == OK)
		{
			//printf("encode dequeueOutputBuffer ok \n");
			sp<ABuffer> buff;
			status = encode_->getOutputBuffer(index, &buff);
			if (status != OK)
			{
				LOGERR("encode getInputBuffer err:%d", status);
				return INS_ERR;
			}

			if (buff->size())
			{	
				if (flags & MediaCodec::BUFFER_FLAG_CODECCONFIG)
				{
					//LOGINFO("BUFFER_FLAG_CODECCONFIG size:%lu", buff->size());
					parse_codec_config(buff);
				}
				else
				{
					bool keyframe = false;
					if (media_type_ == INS_MEDIA_AUDIO)
					{
						keyframe = true;
					}
					else
					{
						keyframe = (flags & MediaCodec::BUFFER_FLAG_SYNCFRAME) ? true : false;
					}
					//LOGINFO("----encode out frame pts:%lld is key:%d size:%d", pts, keyframe, buff->size());
					write_frame(buff, pts, keyframe);
				}
			}

			encode_->releaseOutputBuffer(index);

			//printf("encode dequeueOutputBuffer ok over\n");

			if (flags & MediaCodec::BUFFER_FLAG_EOS)
			{
				LOGINFO("encode eos");
				break;
			}
		}
		else if (status == -EAGAIN)
		{
			//printf("encode dequeueOutputBuffer try later\n");
			//LOGINFO("mcencode dequeueOutputBuffer try later");
			if (!eos) break;
		}
		else if (status == INFO_FORMAT_CHANGED)
		{
			//LOGINFO("mcencode dequeueOutputBuffer INFO_FORMAT_CHANGED");
			sp<AMessage> format;
			encode_->getOutputFormat(&format);
			parse_media_format(format);
		}
		else if (status == INFO_OUTPUT_BUFFERS_CHANGED)
		{
			//LOGINFO("mcencode dequeueOutputBuffer INFO_OUTPUT_BUFFERS_CHANGED");
		}
		else
		{
			LOGINFO("encode dequeueOutputBuffer err:%d", status);
			return INS_ERR;
		}
	}

	return INS_OK;
}

void mc_encode::add_stream_sink(unsigned int index,  const std::shared_ptr<sink_interface>& sink)
{
	std::lock_guard<std::mutex> lock(mtx_);
	sink_.insert(std::make_pair(index, sink));
}

void mc_encode::del_stream_sink(unsigned int index)
{
	std::lock_guard<std::mutex> lock(mtx_);
	auto it = sink_.find(index);
	if (it != sink_.end())
	{
		sink_.erase(it);
	}
}

void mc_encode::parse_media_format(sp<AMessage>& format)
{
	//LOGINFO("----------%s", format->debugString().c_str());

	std::lock_guard<std::mutex> lock(mtx_);
	
	if (media_type_ == INS_MEDIA_VIDEO)
	{
		if (!video_param_) video_param_ = std::make_shared<ins_video_param>();

		video_param_->fps = fps_;
		video_param_->bitrate = bitrate_; // no bitrate in format, so ...
		video_param_->v_timebase = 1000000;
		format->findInt32(INS_MC_KEY_WIDTH, (int*)&video_param_->width);
		format->findInt32(INS_MC_KEY_HEIGHT, (int*)&video_param_->height);

		AString mime;
		format->findString(INS_MC_KEY_MIME, &mime);
		video_param_->mime = mime.c_str();
	}
	else if (media_type_ == INS_MEDIA_AUDIO)
	{
		if (!audio_param_) audio_param_ = std::make_shared<ins_audio_param>();

		audio_param_->b_spatial = b_spatial_;
		audio_param_->bitrate = bitrate_; // no bitrate in format, so ...
		audio_param_->a_timebase = 1000000;
		format->findInt32(INS_MC_KEY_SAMPLE_RATE, (int*)&audio_param_->samplerate);
		format->findInt32(INS_MC_KEY_CHANNEL_CNT, (int*)&audio_param_->channels);

		AString mime;
		format->findString(INS_MC_KEY_MIME, &mime);
		audio_param_->mime = mime.c_str();

		sp<ABuffer> buff; //buff will be free by mediacodec, so must copy
		format->findBuffer(INS_MC_KEY_CSD0, &buff);
		audio_param_->config = new ABuffer(buff->size());
		memcpy(audio_param_->config->data(),buff->data(), buff->size());
	}
	else
	{
		LOGERR("invalid mediatype:%d", media_type_);
		return;
	}
}

void mc_encode::parse_codec_config(sp<ABuffer>& buff)
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (media_type_ == INS_MEDIA_VIDEO)
	{
		if (!video_param_) video_param_ = std::make_shared<ins_video_param>();

		video_param_->config = new ABuffer(buff->size()); //buff will be free by mediacodec, so must copy
		memcpy(video_param_->config->data(), buff->data(), buff->size());
	}
}

void mc_encode::write_frame(sp<ABuffer>& buff, long long pts, bool is_key_frame)
{
	auto frame = std::make_shared<ins_frame>();
	frame->pts = pts;
	frame->dts = pts;
	if (media_type_ == INS_MEDIA_VIDEO)
	{
		frame->duration = 1000000.0/fps_;
	}
	else if (media_type_ == INS_MEDIA_AUDIO)
	{
		frame->duration = 1000000*1024/samplerate_;
	}
	frame->is_key_frame = is_key_frame;
	frame->media_type = media_type_;

	if (buff_type_ == BUFF_TYPE_NORMAL)
	{
		frame->buff_le = std::make_shared<insbuff>(buff->size());
		memcpy(frame->buff_le->data(), buff->data(), buff->size());
	}
	else
	{
		frame->buff = std::make_shared<page_buffer>(buff->size());
		memcpy(frame->buff->data(), buff->data(), buff->size());
	}

	std::lock_guard<std::mutex> lock(mtx_);
	for (auto it = sink_.begin(); it != sink_.end(); it++)
	{
		if (!init_sink(it->second, is_key_frame)) continue;
		it->second->queue_frame(frame);
	}
}

bool mc_encode::init_sink(std::shared_ptr<sink_interface>& sink, bool is_key_frame)
{
	if (media_type_ == INS_MEDIA_VIDEO)
	{
		if (!sink->is_video_param_set())
		{
			if (is_key_frame && video_param_ && video_param_->config != nullptr && video_param_->mime != "")
			{
				sink->set_video_param(*(video_param_.get()));
			}
			else
			{
				return false;
			}
		}
	}
	else if (media_type_ == INS_MEDIA_AUDIO)
	{
		if (!sink->is_audio_param_set())
		{
			if (audio_param_ && audio_param_->config != nullptr && audio_param_->mime != "")
			{
				sink->set_audio_param(*(audio_param_.get()));
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}
