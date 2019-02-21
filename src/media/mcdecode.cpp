
#include "mcdecode.h"

#define MC_INPUT_TIMEOUT  300
#define MC_OUTPUT_TIMEOUT 1000

void mc_decode::close()
{
	if (decode_ != nullptr) 
	{
		decode_->stop();
		decode_->release();
		decode_.clear();
	}

	if (looper_ != nullptr) 
	{
		looper_->stop();
		looper_.clear();
	}

	if (surfaceTexture_ != nullptr)
	{
		surfaceTexture_->setFrameAvailableListener(nullptr);
		surfaceTexture_->abandon();
		surfaceTexture_.clear();
	}

	if (surface_ != nullptr)
	{
		surface_.clear();
	}
}

int mc_decode::open(const sp<AMessage>& format, int texture_id)
{
	status_t status;
	
	looper_ = new ALooper();
	looper_->setName("MediaDecode_looper");
 	looper_->start();

 	AString mime;
	format->findString("mime", &mime);
 	decode_ = MediaCodec::CreateByType(looper_,  mime.c_str(), false, &status);
 	MC_CHECK_NULL_RETURN(decode_, INS_ERR, "mcdecode createbytype fail");
	MC_CHECK_ERR_RETURN(status, INS_ERR, "mcdecode createbytype fail");

	//sp<Surface> surface;

	//if texture id not invalid, decode to surface
	if (texture_id != 0) {
		sp<IGraphicBufferProducer> producer;
		sp<IGraphicBufferConsumer> consumer;
		BufferQueue::createBufferQueue(&producer, &consumer);

		surfaceTexture_ = new GLConsumer(consumer, texture_id, GL_TEXTURE_EXTERNAL_OES, true, false);
		MC_CHECK_NULL_RETURN(surfaceTexture_, INS_ERR, "mcdecode new surfacetexture fail");

		surfaceTexture_->setName(String8("SurfaceTexture"));
		surfaceTexture_->setFrameAvailableListener(this);
		//surfaceTexture_->setDefaultBufferSize(1920, 1440);
		surfaceTexture_->setDefaultMaxBufferCount(5);
		//surfaceTexture_->setConsumerUsageBits(GRALLOC_USAGE_HW_TEXTURE);

		surface_ = new Surface(producer);
		MC_CHECK_NULL_RETURN(surface_, INS_ERR, "new surface fail");
	}

	if (surface_ != nullptr)
	{
		status = decode_->configure(format, surface_, nullptr, 0);
	}
	else
	{
		status = decode_->configure(format, nullptr, nullptr, 0);
	}

	MC_CHECK_ERR_RETURN(status, INS_ERR, "mcdecode configure fail");
	
	status = decode_->start();
	MC_CHECK_ERR_RETURN(status, INS_ERR, "mcdecode start fail");

	LOGINFO("mcdecode mime:%s open success, decode to surface:%d", mime.c_str(), (texture_id != 0));

	return INS_OK;
}

int mc_decode::feed_data(const unsigned char* data, unsigned int size, long long pts, int timeout_us)
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
		status = decode_->dequeueInputBuffer(&index, timeout_us);
		if (status != OK)
		{
			if (status == -EAGAIN)
			{	
				//printf("%p decode dequeueInputBuffer\n", data);
				//LOGINFO("mcdecode dequeueInputBuffer try again");
				usleep(5000);
				continue;
			}
			LOGERR("decode dequeueInputBuffer err:%d", status);
			return INS_ERR;
		}

		sp<ABuffer> buff;
		status = decode_->getInputBuffer(index, &buff);
		MC_CHECK_ERR_RETURN(status, INS_ERR, "mcdecode getInputBuffer fail");
		MC_CHECK_NULL_RETURN(buff, INS_ERR, "mcdecode getInputBuffer is null");
		//LOGINFO("inbuff offset:%d size:%d capacity:%d", buff->offset(), buff->size(), buff->capacity());

		if (flush)
		{
			status = decode_->queueInputBuffer(index, 0, 0, 0, MediaCodec::BUFFER_FLAG_EOS);
			return INS_OK;
		}
		else
		{
			unsigned int cur_size = std::min((unsigned)buff->capacity(), size - offset);
			memcpy(buff->base(), data+offset, cur_size);
			buff->setRange(0, cur_size);
			offset += cur_size;
			status = decode_->queueInputBuffer(index, 0, buff->size(), pts, 0);
		}

		MC_CHECK_ERR_RETURN(status, INS_ERR, "mcdecode queueInputBuffer fail");
	}

	return INS_OK;
}

int mc_decode::drain_decode(long long& pts, int timeout_us, bool b_flush)
{
	status_t status;

	while (1)
	{
		size_t index, out_offset, out_size;
		int64_t timeus;
		uint32_t flags;
		status = decode_->dequeueOutputBuffer(&index, &out_offset, &out_size, &timeus, &flags, timeout_us);
		if (status == OK)
		{
			if (b_flush)
			{
				decode_->releaseOutputBuffer(index);
				if (flags & MediaCodec::BUFFER_FLAG_EOS)
				{
					//LOGINFO("decode eos");
					return INS_ERR_OVER;
				}
				else
				{
					continue;
				}
			}
			else if (out_size)
			{
				pts = timeus;
				status = decode_->renderOutputBufferAndRelease(index);
				wait_frame_available();
				//surfaceTexture_->updateTexImage();
				return INS_OK;
			}
			else
			{
				 decode_->releaseOutputBuffer(index);
				 return INS_ERR_OVER;
			}
		}
		else if (status == -EAGAIN)
		{
			if (b_flush) continue;
			//LOGINFO("decode dequeueOutputBuffer try later");
			return INS_ERR_TIME_OUT;
		}
		else if (status == INFO_FORMAT_CHANGED)
		{
			//LOGINFO("decode dequeueOutputBuffer INFO_FORMAT_CHANGED");
		}
		else if (status == INFO_OUTPUT_BUFFERS_CHANGED)
		{
			//LOGINFO("decode dequeueOutputBuffer INFO_OUTPUT_BUFFERS_CHANGED");
		}
		else
		{
			LOGINFO("decode dequeueOutputBuffer err:%d", status);
			return INS_ERR;
		}
	}

	return INS_OK;
}

void mc_decode::update_tex_image()
{
	surfaceTexture_->updateTexImage();
}

void mc_decode::wait_frame_available()
{
	std::unique_lock<std::mutex> lock(mtx_);
	while (!frameAvailable_) {
		cv_.wait(lock);
	}

	frameAvailable_ = false;
}

void mc_decode::onFrameAvailable(const BufferItem& item)
{
	std::unique_lock<std::mutex> lock(mtx_);
	frameAvailable_ = true;
	cv_.notify_all();
}

/*------------------- parting line ------------------------*/

int mc_decode::drain_decode(int timeout_us, bool b_flush)
{
	status_t status;

	while (1)
	{
		size_t index, out_offset, out_size; 
		int64_t  pts;
		uint32_t flags;
		status = decode_->dequeueOutputBuffer(&index, &out_offset, &out_size, &pts, &flags, timeout_us);
		if (status == OK)
		{
			sp<ABuffer> buff;
			status = decode_->getOutputBuffer(index, &buff);
			if (status != OK)
			{
				LOGERR("encode getInputBuffer err:%d", status);
				return INS_ERR;
			}

			queue_frame_to_sink(buff, pts);

			decode_->releaseOutputBuffer(index);

			if (flags & MediaCodec::BUFFER_FLAG_EOS)
			{
				LOGINFO("decode eos");
				return INS_ERR_OVER;
			}
			else
			{
				continue;
			}
		}
		else if (status == -EAGAIN)
		{
			if (b_flush) continue;
			//LOGINFO("decode dequeueOutputBuffer try later");
			return INS_ERR_TIME_OUT;
		}
		else if (status == INFO_FORMAT_CHANGED)
		{
			LOGINFO("mcencode dequeueOutputBuffer INFO_FORMAT_CHANGED");
			sp<AMessage> format;
			decode_->getOutputFormat(&format);
			//LOGINFO("----------%s", format->debugString().c_str());
		}
		else if (status == INFO_OUTPUT_BUFFERS_CHANGED)
		{
			//LOGINFO("mcencode dequeueOutputBuffer INFO_OUTPUT_BUFFERS_CHANGED");
		}
		else
		{
			LOGINFO("decode dequeueOutputBuffer err:%d", status);
			return INS_ERR;
		}
	}

	return INS_OK;
}

void mc_decode::add_sink(int index, const std::shared_ptr<raw_video_queue>& sink)
{
	std::lock_guard<std::mutex> lock(sink_mtx_);
	sink_.insert(std::make_pair(index, sink));
}

void mc_decode::delete_sink(int index)
{
	std::lock_guard<std::mutex> lock(sink_mtx_);
	auto it = sink_.find(index);
	if (it != sink_.end()) sink_.erase(it);
}

void mc_decode::queue_frame_to_sink(sp<ABuffer>& buff, long long pts)
{
	if (buff->size() <= 0) return;

	if (frames_++%15) return;

	auto frame = std::make_shared<raw_frame>();
	frame->buff = std::make_shared<page_buffer>(buff->size());
	frame->pts = pts;
	memcpy(frame->buff->data(), buff->data(), buff->size());

	std::lock_guard<std::mutex> lock(sink_mtx_);
	for (auto it = sink_.begin(); it != sink_.end(); it++)
	{	
		it->second->queue_frame(frame);
	}
}
