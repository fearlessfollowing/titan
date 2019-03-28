
#include "audio_mgr.h"
#include "inslog.h"
#include "common.h"
#include "hw_util.h"
#include "audio_dev.h"
#include "audio_reader.h"
#include "ffaacenc.h"
#include "audio_fmt_conv.h"
#include "audio_spatial.h"
#include "audio_play.h"

#define BITRATE_1CH 64000 //bit


#if 0

switch_inner_audio.sh
----------------------------------------------------------------------	
#!/bin/sh	
echo 000e 0100 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 000f 0808 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0027 1400 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0028 1010 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0061 8000 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0063 e818 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0064 0c00 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0065 0000 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0066 0000 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 006a 001b > /sys/bus/i2c/devices/1-001c/codec_reg
echo 006c 0200 > /sys/bus/i2c/devices/1-001c/codec_reg
-----------------------------------------------------------------------

switch_outer_linein_audio.sh
-----------------------------------------------------------------------	
#!/bin/sh	
echo 000e 0040 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 000f 1f1f > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0027 3420 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0028 3030 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0061 8006 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0063 e81c > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0064 1c00 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0065 0c00 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 0066 0300 > /sys/bus/i2c/devices/1-001c/codec_reg
echo 006a 003d > /sys/bus/i2c/devices/1-001c/codec_reg
echo 006c 3600 > /sys/bus/i2c/devices/1-001c/codec_reg
-----------------------------------------------------------------------


switch_outer_mic_audio.sh


#endif 


audio_mgr::~audio_mgr()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);

    dev_.clear(); 
    enc_.clear(); //一定要在dev后关闭enc, dev回调on_pcm_data会调用enc

    LOGINFO("audio manager close");
}

int32_t audio_mgr::open(const audio_param& param)
{
    int32_t ret;
    dev_name_ = INS_AUDIO_DEV_NAME;

    bool dev_35mm = hw_util::check_35mm_mic_on();
    if (dev_35mm) {
        ret = open_outer_35mm_mic(param.hdmi_audio);
        RETURN_IF_NOT_OK(ret);
    } else {
        auto dev = std::make_shared<audio_dev>();
        ret = dev->open(2, 0);
        if (ret == INS_OK) {
            spatial_ = dev->is_spatial(); 
            dev_name_ = dev->get_vender_name();
            dev = nullptr;
            ret = open_outer_usb_mic(param.hdmi_audio);
            RETURN_IF_NOT_OK(ret);
        } else {
            dev = nullptr;
            ret = open_inner_mic(param.type, param.fanless, param.hdmi_audio); 
            RETURN_IF_NOT_OK(ret);
        }
    }

    if (buff_size_) {
        pool_ = std::make_shared<obj_pool<insbuff>>(-1, "audio");
    }

    LOGINFO("audio open name:%s spatial:%d dev type:%d audio type:%d", dev_name_.c_str(), spatial_, dev_type_, param.type);
    
    return INS_OK;
}



int32_t audio_mgr::open_inner_mic(int32_t type, bool fanless, bool hdmi_audio)
{   
    dev_type_ = INS_SND_TYPE_INNER;
    
	system("switch_inner_audio.sh");

    //card=1 device=0
    auto dev = std::make_shared<audio_reader>();
    dev->set_frame_cb(0, [this](uint32_t index, ins_pcm_frame& frame){ on_pcm_data(index, frame); });
    auto ret = dev->open(1, 0, dev_type_);
    RETURN_IF_NOT_OK(ret);

    dev_.push_back(dev);

    dev->get_param(samplerate_, channel_, fmt_size_);

    spatial_handle_ = std::make_shared<audio_spatial>();
    ret = spatial_handle_->open(samplerate_, channel_, fanless);
    RETURN_IF_NOT_OK(ret);


    if (type != INS_AUDIO_N_C) {	// 也就是在预览的时候用到

		spatial_ = true;

        std::deque<ins_pcm_frame> one;
        queue_.push_back(one);
        queue_.push_back(one);

        //card=1 device=1
        dev = std::make_shared<audio_reader>();
        dev->set_frame_cb(1, [this](uint32_t index, ins_pcm_frame& frame){ on_pcm_data(index, frame); });

        ret = dev->open(1, 1, dev_type_);
        RETURN_IF_NOT_OK(ret);
        dev_.push_back(dev);

        ret = open_audio_enc(samplerate_, channel_*2, BITRATE_1CH*channel_*2);
        RETURN_IF_NOT_OK(ret);

        //enc 启动后在启动dev,因为on_pcm_data会调用enc
        if (type == INS_AUDIO_N_C || type == INS_AUDIO_Y_C) {
            ret = open_audio_enc(samplerate_, channel_, BITRATE_1CH*channel_);
            RETURN_IF_NOT_OK(ret);
        }

        if (hdmi_audio) {
            play_ = std::make_shared<audio_play>();
            if (play_->open(samplerate_, channel_) != INS_OK) 
                play_ = nullptr;
        }

        buff_size_ = 1024 * 4 * sizeof(short);

        th_ = std::thread(&audio_mgr::process_task, this);
    } else {

        /* enc 启动后在启动dev,因为on_pcm_data会调用enc */
        if (type == INS_AUDIO_N_C || type == INS_AUDIO_Y_C) {
            ret = open_audio_enc(samplerate_, channel_, BITRATE_1CH*channel_);
            RETURN_IF_NOT_OK(ret);
        }

        if (hdmi_audio) {
            play_ = std::make_shared<audio_play>();
            if (play_->open(samplerate_, channel_) != INS_OK) 
				play_ = nullptr;
        }

        dev_[0]->set_offset(0);
    }

    return INS_OK;
}


int32_t audio_mgr::open_outer_35mm_mic(bool hdmi_audio)
{
    dev_type_ = INS_SND_TYPE_35MM;

	system("switch_outer_linein_audio.sh");

    auto dev = std::make_shared<audio_reader>();
    dev->set_frame_cb(0, [this](uint32_t index, ins_pcm_frame& frame){ on_pcm_data(index, frame); });
    auto ret = dev->open(1, 0, dev_type_);
    RETURN_IF_NOT_OK(ret);

    dev->get_param(samplerate_, channel_, fmt_size_);
    ret = open_audio_enc(samplerate_, channel_, BITRATE_1CH*channel_);
    RETURN_IF_NOT_OK(ret);

    if (hdmi_audio) {
        play_ = std::make_shared<audio_play>();
        if (play_->open(samplerate_, channel_) != INS_OK) play_ = nullptr;
    }

    //enc 启动后在启动dev,因为on_pcm_data会调用enc
    dev->set_offset(0);
    dev_.push_back(dev);

    return INS_OK;
}


int32_t audio_mgr::open_outer_usb_mic(bool hdmi_audio)
{
    dev_type_ = INS_SND_TYPE_USB;

    auto dev = std::make_shared<audio_reader>();
    dev->set_frame_cb(0, [this](uint32_t index, ins_pcm_frame& frame){ on_pcm_data(index, frame); });
    auto ret = dev->open(2, 0, dev_type_);
    RETURN_IF_NOT_OK(ret);

    dev->get_param(samplerate_, channel_, fmt_size_);

    ret = open_audio_enc(samplerate_, channel_, BITRATE_1CH*channel_);
    RETURN_IF_NOT_OK(ret);

    if (hdmi_audio) {
        play_ = std::make_shared<audio_play>();
        if (play_->open(samplerate_, channel_) != INS_OK) play_ = nullptr;
    }

    //enc 启动后在启动dev,因为on_pcm_data会调用enc
    dev->set_offset(0);
    dev_.push_back(dev);

    if (fmt_size_ == 3) 
        buff_size_ = 1024*2*sizeof(short);

    return INS_OK;
}



int32_t audio_mgr::open_audio_enc(uint32_t samplerate, uint32_t channel, uint32_t bitrate)
{
    ffaacenc_param param;
    param.samplerate = samplerate;
    param.channel = channel;
    param.bitrate = bitrate;
    param.timescale.den = 1000*1000;
    param.timescale.num = 1;
    param.spatial = spatial_;

    auto enc = std::make_shared<ffaacenc>();
    auto ret = enc->open(param, false);
    RETURN_IF_NOT_OK(ret);

    enc_.push_back(enc);

    return INS_OK;
}

void audio_mgr::on_pcm_data(uint32_t index, ins_pcm_frame& frame)
{

	struct timeval start_time, end_time;
    if (dev_type_ == INS_SND_TYPE_USB) {	/* USB接口的音频设备 */
        if (fmt_size_ == 3) { 
            auto buff_s16 = pool_->pop();
            if (!buff_s16->data()) 
                buff_s16->alloc(frame.buff->size()/3*2);
            
            s24_to_s16(frame.buff, buff_s16);
            frame.buff = buff_s16;
        }
        
        if (enc_.size() <= 0) { 
            LOGERR("----------------------------usbmic on_pcm_data,but audio enc deinit");
        }

        enc_[0]->encode(frame);
        if (play_) 
            play_->queue_frame(frame);
    } else if (dev_type_ == INS_SND_TYPE_35MM) {	/* 3.5MM外置MIC */
        if (enc_.size() <= 0) { 
            LOGERR("----------------------------35mm on_pcm_data,but audio enc deinit");
        }

        enc_[0]->encode(frame);
        if (play_) 
            play_->queue_frame(frame);
    } else if (dev_type_ == INS_SND_TYPE_INNER) {
		if (dev_.size() == 1) {	/* 内置MIC: 只有一路音频 */
	        if (enc_.size() <= 0) { 
	            LOGERR("----------------------------inner on_pcm_data,but audio enc deinit");
	        }

			gettimeofday(&start_time, nullptr);			
	        auto dn_buff = spatial_handle_->denoise(frame.buff);	/* 全景声降噪 */
			gettimeofday(&end_time, nullptr);

			double fps = (double)(end_time.tv_sec*1000000 + end_time.tv_usec - start_time.tv_sec*1000000 - start_time.tv_usec);
			LOGINFO("time consumer: [%lf]", fps);

	        f32_to_s16(dn_buff, frame.buff);
	        enc_[0]->encode(frame);
			
	        if (play_) 	/* 如果需要播放 */
	            play_->queue_frame(frame);	/* 将该帧数据丢入播放队列中 */
			
    	} else {	/* 两路的时候有专门的线程process_task去处理 */	
        	queue_pcm_frame(index, frame);
    	}
	}
}


void audio_mgr::process_task()
{   
    if (align_first_frame() != INS_OK) return; 

    ins_pcm_frame frame;
    while (!quit_) {
        std::vector<ins_pcm_frame> v_frame;
        for (uint32_t i = 0; i < 2; i++) {
            while (!quit_) {
                if (deque_pcm_frame(i, frame)) {
                    v_frame.push_back(frame);
                    break;
                } else {
                    usleep(10*1000);
                }
            }
        }

        if (v_frame.size() != 2) break;
        process(v_frame);
    }

    LOGINFO("audio sync taks exit");
}



int32_t audio_mgr::align_first_frame()
{
    while (1) {
        //wait dev ready
        if (dev_[0]->base_time_ != -1 && dev_[1]->base_time_ != -1) 
            break;
        
        usleep(10*1000);
        if (quit_) 
            return INS_ERR;
    }

    if (dev_[0]->base_time_ > dev_[1]->base_time_) {
        dev_[0]->set_offset(dev_[0]->base_time_ - dev_[1]->base_time_);
        dev_[1]->set_offset(0);
    } else {
        dev_[1]->set_offset(dev_[1]->base_time_ - dev_[0]->base_time_);
        dev_[0]->set_offset(0);
    }

    ins_pcm_frame frame_0, frame_1;
    frame_0.pts = frame_1.pts = -1;

    while (1) {
        if (quit_) 
            return INS_ERR;
        
        usleep(10*1000);

        if (frame_0.pts == -1) 
            deque_pcm_frame(0, frame_0);
        if (frame_1.pts == -1) 
            deque_pcm_frame(1, frame_1);

        if (frame_0.pts == -1 || frame_1.pts == -1) 
            continue;

        //LOGINFO("====== audio sync first frame %lld %lld", frame_0.pts, frame_1.pts);

        if (frame_0.pts > frame_1.pts) {
            frame_1.pts = -1;
        } else if (frame_0.pts < frame_1.pts) {
            frame_0.pts = -1;
        } else {
            break;
        }
    }

    LOGINFO("---audio align first frame %lld", frame_0.pts);

    return INS_OK;
}



void audio_mgr::process(std::vector<ins_pcm_frame>& v_frame)
{
    std::vector<std::shared_ptr<insbuff>> v_buff;
    for (auto& it : v_frame) 
		v_buff.push_back(it.buff);

    //denoise, output is float
    auto v_dn_buff = spatial_handle_->denoise(v_buff);

    if (enc_.size() > 1 || play_)  {
        if (spatial_handle_->b_denoise()) {
            f32_to_s16(v_dn_buff[0], v_buff[0]); //如果不降噪直接用原始的音频
        }

        if (enc_.size() > 1) 
            enc_[1]->encode(v_frame[0]);
        if (play_) 
            play_->queue_frame(v_frame[0]);
    }

    auto buff_4ch = pool_->pop();
    if (!buff_4ch->data()) 
        buff_4ch->alloc(v_frame[0].buff->size()*2);

    //compose spatial, output s16
    spatial_handle_->compose(v_dn_buff, buff_4ch);
    //mix_4channel_s16(v_frame[0].buff, v_frame[1].buff, buff_4ch);

    ins_pcm_frame frame_4ch;
    frame_4ch.pts = v_frame[0].pts;
    frame_4ch.buff = buff_4ch;
    enc_[0]->encode(frame_4ch);
}

void audio_mgr::queue_pcm_frame(uint32_t index, ins_pcm_frame& frame)
{
    std::lock_guard<std::mutex> lock(mtx_[index]);
    if (queue_[index].size() > 200) {
        LOGERR("pcm queue:%d size:%d discard 10 frame", index, queue_[index].size());
        for (int32_t i = 0; i < 10; i++) 
            queue_[index].pop_front();
    }
    queue_[index].push_back(frame);
}

bool audio_mgr::deque_pcm_frame(uint32_t index, ins_pcm_frame& frame)
{
    std::lock_guard<std::mutex> lock(mtx_[index]);
    if (queue_[index].size() < 2) 
        return false;
    frame = queue_[index].front();
    queue_[index].pop_front();
    return true;
}

void audio_mgr::add_output(uint32_t index, const std::shared_ptr<sink_interface>& sink, bool aux)
{
    //LOGINFO("audio mgr add output index:%d enc cnt:%lu", index, enc_.size());
    
    if (aux && enc_.size() > 1) {
        enc_[1]->add_output(index, sink);
    } else {
        enc_[0]->add_output(index, sink);
    }
}

void audio_mgr::del_output(uint32_t index)
{
    for (auto& it : enc_) {
        it->del_output(index);
    }
}


