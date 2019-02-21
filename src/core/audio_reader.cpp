#include "audio_reader.h"
#include "audio_dev.h"
#include "common.h"
#include "inslog.h"
#include <sys/time.h>
#include "cam_manager.h"
#include "access_msg_center.h"
#include "hw_util.h"

#define PCM_USEC_PER_BYTE (1000000.0/fmt_size_/samplerate_/channel_)

audio_reader::~audio_reader()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int32_t audio_reader::open(int32_t card, int32_t dev, int32_t dev_type)
{
    dev_ = std::make_shared<audio_dev>();
    auto ret = dev_->open(card, dev);
    RETURN_IF_NOT_OK(ret);
    name_ = dev_->get_name();

	if (card == 2)
	{
    	dev_->get_param(samplerate_, channel_, fmt_);
	}
	else
	{
		samplerate_ = 48000;
		channel_ = 2;
		fmt_ = SND_PCM_FORMAT_S16_LE;
	}
    dev_->set_param(samplerate_, channel_, fmt_);
    frame_size_ = dev_->get_frame_size();
    fmt_size_ = dev_->get_fmt_size(fmt_);
	dev_type_ = dev_type;

    pool_ = std::make_shared<obj_pool<insbuff>>(-1, "pcm");
	// pool_->alloc(32);
	// for (int32_t i = 0; i < 32; i++)
	// {
	// 	auto buff = pool_->pop();
	// 	if (!buff->data()) buff->alloc(frame_size_);
	// }

    th_ = std::thread(&audio_reader::task, this);

    return INS_OK;
}

void audio_reader::set_offset(int64_t offset_time)
{
    assert(offset_time >= 0);
    offset_time_ = offset_time;
    LOGINFO("%s offset time:%lld", name_.c_str(), offset_time_);
}

void audio_reader::task()
{
	while (!quit_)
	{
        auto buff = pool_->pop();
        if (!buff->data()) buff->alloc(frame_size_);
        auto ret = dev_->read(buff);
		if (ret == INS_ERR_RETRY) 
		{
			continue;
		}
		else if (ret != INS_OK)
		{
			send_rec_over_msg(INS_ERR_MICPHONE_EXCEPTION);
			break;
		}

		if (base_time_ == -1) set_base_time();

		if (offset_time_ == -1) //还没有设置偏移表示还没有启动
		{
			frame_seq_++;
		}
        else
        {
		    aliagn_frame(buff);
        }
	}

    LOGINFO("%s read task exit", name_.c_str());
}

void audio_reader::aliagn_frame(const std::shared_ptr<insbuff>& buff)
{
    uint32_t channel_sample_size = fmt_size_*channel_;

	if (pre_buff_ == nullptr)
	{
		pre_buff_ = pool_->pop();
        if (!pre_buff_->data()) pre_buff_->alloc(frame_size_);
		pre_buff_->set_offset(0);
		if (offset_time_)
		{
			//fmt_size_*channel_放在最后保证是一个sample字节数的整数倍
			int total_offset_bytes = offset_time_*samplerate_/1000000*channel_sample_size; 
			int offset_bytes = total_offset_bytes % frame_size_;
			memcpy(pre_buff_->data(), buff->data()+buff->size()-offset_bytes, offset_bytes);
			pre_buff_->set_offset(offset_bytes);
			base_time_ -= offset_time_;
			frame_seq_ += total_offset_bytes/frame_size_ + 1;
			LOGINFO("---%s base time:%lld offset bytes:%d framesize:%d total:%d", name_.c_str(), base_time_, offset_bytes, frame_size_, total_offset_bytes);
			return;
		}
	}

    //计算需要补多少sample
	int32_t sample_cnt = calc_delta_sample();
	uint8_t* latest_sample = buff->data() + buff->size() - channel_sample_size;

	if (!pre_buff_->offset())
	{
		output(buff);
	}
	else
	{
        auto buff_n = pool_->pop();
        if (!buff_n->data()) buff_n->alloc(frame_size_);
		memcpy(buff_n->data(), pre_buff_->data(), pre_buff_->offset());
		memcpy(buff_n->data()+pre_buff_->offset(), buff->data(), buff->size()-pre_buff_->offset());
		memcpy(pre_buff_->data(), buff->data()+buff->size()-pre_buff_->offset(), pre_buff_->offset());
		output(buff_n);
	}

	if (sample_cnt >= 0)
	{
		//复制最后一帧填充
		for (int32_t i = 0; i < sample_cnt; i++)
		{
			if (pre_buff_->offset() + channel_sample_size > pre_buff_->size()) 
			{
				LOGINFO("---%s left pcm sample:%d up to a frame", name_.c_str(), pre_buff_->offset());
				output(pre_buff_);
				pre_buff_ = pool_->pop();
				if (!pre_buff_->data()) pre_buff_->alloc(frame_size_);
				pre_buff_->set_offset(0);
			}
			else
			{
				memcpy(pre_buff_->data()+pre_buff_->offset(), latest_sample, channel_sample_size);
				pre_buff_->set_offset(pre_buff_->offset()+channel_sample_size);
			}
		}
	}
	else 
	{
		auto discard_size = std::max((int32_t)pre_buff_->offset()+(int32_t)channel_sample_size*sample_cnt, (int32_t)0);
		//printf("discard size:%d must discard size:%d buff offset:%d\n", discard_size, channel_sample_size*sample_cnt, pre_buff_->offset());
		pre_buff_->set_offset(discard_size);
	}
}

// bool audio_reader::deque_frame(ins_pcm_frame& frame)
// {
//     std::lock_guard<std::mutex> lock(mtx_);
//     if (queue_.size() < 2) return false;
//     frame = queue_.front();
//     queue_.pop_front();
//     return true;
// }

void audio_reader::output(const std::shared_ptr<insbuff>& buff)
{
    ins_pcm_frame frame;
    frame.buff = buff;
	frame.pts = base_time_ + frame_seq_++*frame_size_*PCM_USEC_PER_BYTE;
    if (frame_cb_) frame_cb_(index_, std::ref(frame));
}

int32_t audio_reader::calc_delta_sample()
{
	read_cnt_++;
	if (read_cnt_ > 1000)
	{
		double sample_cnt = min_delta_*samplerate_/1000000.0;
		LOGINFO("%s delta sample cnt:%lf min delta:%lld", name_.c_str(), sample_cnt, min_delta_);
		min_delta_ = 0x7fffffff;
		read_cnt_ = 0;
		return (int32_t)sample_cnt;
	}
	else if (read_cnt_ > 1000 - 20)
	{
		struct timeval tm;
		gettimeofday(&tm, nullptr);
		long long pts = base_time_ + (frame_seq_*frame_size_ + pre_buff_->offset())*PCM_USEC_PER_BYTE;
		long long delta = (long long)tm.tv_sec*1000000 + tm.tv_usec - cam_manager::nv_amba_delta_usec() - pts;
		min_delta_ =  INS_ABS_MIN(min_delta_, delta);
		//LOGINFO("-----audio device:%d delta:%lld amba:%d", device_, delta, cam_manager::nv_amba_delta_usec());
	}

	return 0;
}

void audio_reader::set_base_time()
{
    struct timeval tm;
    gettimeofday(&tm, nullptr);
    base_time_ = tm.tv_sec*1000000 + tm.tv_usec;
    LOGINFO("%s first time:%d %d", name_.c_str(), tm.tv_sec, tm.tv_usec);
}

void audio_reader::send_rec_over_msg(int32_t errcode) const
{	
	bool dev_gone = false;;
	if (dev_type_ == INS_SND_TYPE_35MM)
	{
		if (!hw_util::check_35mm_mic_on()) dev_gone = true;
	}
	else if (dev_type_ == INS_SND_TYPE_USB)
	{
		if (!hw_util::check_usb_mic_on()) dev_gone = true;
	}
    
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
	obj.set_int("code", errcode);
	obj.set_bool("dev_gone", dev_gone);
	access_msg_center::queue_msg(0, obj.to_string());
}