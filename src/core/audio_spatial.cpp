
#include "audio_spatial.h"
#include "inslog.h"
#include "common.h"
#include "TwirlingCapture.h"
#include "audio_fmt_conv.h"
#include <sstream>
#include "camera_info.h"

audio_spatial::~audio_spatial()
{
    for (auto it : tl_ans_) {
        ansRelease(it);
    }

    tl_ans_.clear();

    if (tl_cap_) {
        captureRelease(tl_cap_);
        tl_cap_ = nullptr;
    }
}

int32_t audio_spatial::open(uint32_t samplerate, uint32_t channel, bool fanless)
{
    fanless_ = fanless;
    frame_size_ = 1024*sizeof(float);

    cur_dn_vol_ = camera_info::get_volume();
    int32_t dn_db = get_ansdb_by_volume(cur_dn_vol_);

    for (int i = 0; i < 2; i++) {
        if (!fanless_) {
            auto tl_ans = ansInit(512, channel, samplerate, dn_db, true);  //-20 -- -6
            RETURN_IF_TRUE(!tl_ans, "ansInit fail", INS_ERR);

            auto noise_buff = get_noise_sample(fanless_, i);
            if (noise_buff) 
				ansSetNoiseSample(tl_ans, (float*)noise_buff->data(), noise_buff->size()/sizeof(float), 2.0);

            tl_ans_.push_back(tl_ans);
        }

        auto in_buff = std::make_shared<insbuff>(frame_size_*channel);
        ans_in_buff_.push_back(in_buff);

        auto out_buff = std::make_shared<insbuff>(frame_size_*channel);
        ans_out_buff_.push_back(out_buff);
    }

    tl_cap_ = captureInit(channel*2, 512, channel*2, samplerate, true);
    RETURN_IF_TRUE(!tl_cap_, "captureInit fail", INS_ERR);
    captureSet(tl_cap_, true, 1.0);

    cap_in_buff_ = std::make_shared<insbuff>(frame_size_*channel*2);
    cap_out_buff_ = std::make_shared<insbuff>(frame_size_*channel*2);

    LOGINFO("spatial audio handle open vol:%d denoise db:%d", cur_dn_vol_, dn_db);

    return INS_OK;
}


std::vector<std::shared_ptr<insbuff>> audio_spatial::denoise(const std::vector<std::shared_ptr<insbuff>>& v_in)
{
    int32_t dn_db = 0;
    if (camera_info::get_volume() != cur_dn_vol_) { /* 音量改变的时候改变降噪幅度 */
        cur_dn_vol_ = camera_info::get_volume();
        dn_db = get_ansdb_by_volume(cur_dn_vol_);
        LOGINFO("volume:%d change denoise db:%d", cur_dn_vol_, dn_db);
    }

    /* 由于denoise最大只能处理512samples,所以要分两次 */
    for (uint32_t i = 0; i < v_in.size(); i++) {
        s16_to_f32(v_in[i], ans_in_buff_[i]);
        if (!fanless_) {
            if (dn_db) 
				ansSet(tl_ans_[i], dn_db);
            ansProcess(tl_ans_[i], (float*)ans_in_buff_[i]->data(), (float*)ans_out_buff_[i]->data());
            ansProcess(tl_ans_[i], (float*)(ans_in_buff_[i]->data()+frame_size_), (float*)(ans_out_buff_[i]->data()+frame_size_));
        }
    }

    if (fanless_) {	// 不降噪
        return ans_in_buff_;
    } else {	// 降噪
        return ans_out_buff_;
    }
}

std::shared_ptr<insbuff> audio_spatial::denoise(const std::shared_ptr<insbuff> in)
{
    int32_t dn_db = 0;
    if (camera_info::get_volume() != cur_dn_vol_) {	// 音量改变的时候改变降噪幅度
        cur_dn_vol_ = camera_info::get_volume();
        dn_db = get_ansdb_by_volume(cur_dn_vol_);
        LOGINFO("volume:%d change denoise db:%d", cur_dn_vol_, dn_db);
    }

    /* 由于denoise最大只能处理512samples,所以要分两次 */
    s16_to_f32(in, ans_in_buff_[0]);
    if (!fanless_) {
        if (dn_db) 
			ansSet(tl_ans_[0], dn_db);

        ansProcess(tl_ans_[0], (float*)ans_in_buff_[0]->data(), (float*)ans_out_buff_[0]->data());
        ansProcess(tl_ans_[0], (float*)(ans_in_buff_[0]->data()+frame_size_), (float*)(ans_out_buff_[0]->data()+frame_size_));
    }

    if (fanless_)  { //不降噪
        return ans_in_buff_[0];
    } else {	//降噪
        return ans_out_buff_[0];
    }
}



int32_t audio_spatial::compose(const std::vector<std::shared_ptr<insbuff>>& v_in, std::shared_ptr<insbuff>& out)
{
    assert(v_in.size() == 2);

    mix_4channel_f32(v_in, cap_in_buff_);

    captureProcess(tl_cap_, 0, 0, 0, (float*)cap_in_buff_->data(), (float*)cap_out_buff_->data(), nullptr);
    captureProcess(tl_cap_, 0, 0, 0, (float*)(cap_in_buff_->data()+frame_size_*2), (float*)(cap_out_buff_->data()+frame_size_*2), nullptr);

    f32_to_s16(cap_out_buff_, out);

    return INS_OK;
}




/***********************************************************************************************
** 函数名称: get_noise_sample
** 函数功能: 获取噪声样本
** 入口参数:
**		fanless - 是否为无风扇模式(true: 无风扇模式; false: 开风扇)
**		index	- 索引值(0,1 - in1p, in1n)
** 返 回 值: 噪声样本存在,返回样本数据; 否则返回nullptr
*************************************************************************************************/
std::shared_ptr<insbuff> audio_spatial::get_noise_sample(bool fanless, int index)
{
    std::stringstream ss;
	
	/* 根据是否开风扇及索引来找到对应的噪声样本 */ 
    if (fanless) {	
        ss << INS_NOISE_SAMPLE_PATH << "/fanless_" << index << ".wav";
    } else {
        ss << INS_NOISE_SAMPLE_PATH << "/" << index << ".wav";
    }

	/* 打开噪声样本文件 */
    FILE* fp = fopen(ss.str().c_str(), "rb");
    if (fp == nullptr) {
        LOGERR("file:%s open fail", ss.str().c_str());
        return nullptr;
    }

	/* 定位到音频数据文件的offset,这个位置可靠??? */
    if (fanless) {
        fseek(fp, 44+48000*4*sizeof(short), SEEK_SET);
    } else {
        fseek(fp, 44+48000*12*sizeof(short), SEEK_SET);
    }

	/* 从噪声样本文件的指定位置读取51200次采样数据到缓冲区中 */
    int32_t sample_cnt = 5120 * 10;
    auto s16_buff = std::make_shared<insbuff>(sample_cnt * 2 * sizeof(short));
    int32_t ret = fread(s16_buff->data(), 1, s16_buff->size(), fp);
    fclose(fp);

    if (ret != (int)s16_buff->size()) {
        LOGERR("read audio noise sample size:%d < %d", ret, s16_buff->size());
        return nullptr;
    } else {
        LOGINFO("read audio noise sample:%s success", ss.str().c_str());
    }

	/* 将S16 2通道的数据转换为f32 1通道的数据(数据格式转换) */
    auto f32_buff = std::make_shared<insbuff>(sample_cnt*sizeof(float));

    s16_2ch_to_f32_1ch(s16_buff, f32_buff);

    return f32_buff;
}



int32_t audio_spatial::get_ansdb_by_volume(uint32_t volume)
{
    int32_t db_64 = -16, db_96 = -18;
    // if (b_fanless) {
    //     db_64 = -13;
    //     db_96 = -15;
    // } else
    // {
    //     db_64 = -16;
    //     db_96 = -18;
    // }

    if (db_64 == db_96) return db_64;

    int db = ((int32_t)volume - 64)*(db_96-db_64)/32 + db_64;

    return db;
}


