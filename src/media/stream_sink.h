
#ifndef _STREAM_SINK_H_
#define _STREAM_SINK_H_

#include <stdint.h>
#include <thread>
#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <vector>
#include "insmux.h"
#include "sink_interface.h"
#include "ins_timer.h"
#include "access_msg_option.h"
#include "metadata.h"
#include "camm_util.h"

#define SINK_NO_FRAME 1

class stream_sink : public sink_interface {
public:
	virtual 		~stream_sink() override;
					stream_sink(std::string url);
	virtual void 	queue_gps(std::shared_ptr<ins_gps_data>& gps) override;
	virtual void 	queue_gyro(std::shared_ptr<insbuff>& data, int64_t delta_ts) override;//模组间对齐时间
	virtual void 	queue_exposure_time(std::shared_ptr<exposure_times>& data) override;
	virtual void 	queue_frame(const std::shared_ptr<ins_frame>& frame) override;
	virtual void 	set_audio_param(const ins_audio_param& param) override;
	virtual void 	set_video_param(const ins_video_param& param) override;

	virtual bool is_audio_param_set() override
	{
		return audio_param_ != nullptr;
	}
	
	virtual bool is_video_param_set() override
	{
		return video_param_ != nullptr;
	}
	
	void set_video(bool value)
	{
		b_video_ = value;
	}
	
	void set_audio(bool value)
	{
		b_audio_ = value;
	}
	
	void set_gps(bool value)
	{
		b_gps_ = value;
		if (b_gps_) gps_buff_ = std::make_shared<insbuff>(camm_util::gps_size);
	}
	
	void set_gyro(bool gyro = true, bool exp = true)
	{
		b_gyro_ = gyro;
		b_exp_ = exp;
		#ifdef GYRO_EXT
		if (b_gyro_) gyro_buff_ = std::make_shared<insbuff>(camm_util::gyro_accel_mgm_size);
		#else
		if (b_gyro_) gyro_buff_ = std::make_shared<insbuff>(camm_util::gyro_accel_size);
		#endif
		if (b_exp_) exp_buff_ = std::make_shared<insbuff>(camm_util::exposure_size);
	}
	
	void set_live(bool value)
	{
		b_live_ = value;
	}
	
	void set_origin_key(bool value)
	{
		b_origin_key_ = value;
	}
	
	void set_stitching(bool value)
	{
		b_stitching_ = value;
	}
	
	void set_just_last_frame(bool value)
	{
		b_just_last_frame_ = value; //只存最近一帧IDR到last_key_frame_,其他都不处理
	}
	
	void set_override(bool value)
	{
		b_override_ = value;
	}
	
	void set_auto_start(bool value)
	{
		b_auto_start_ = value;
	}
	
	void start(int64_t start_pts);
	
	void set_fragment(bool frag, bool manu);
	
	void set_auto_connect(const ins_auto_connect& param)
	{
		if (param.enable) {
			b_auto_connect_ = param.enable;
			auto_connect_interval_ = param.interval;
			auto_connect_cnt_ = param.count;
		} else if (!b_preview_) {	// preview必然重连
			b_auto_connect_ = false;
		}
	}

	void set_preview(bool value) {
		b_preview_ = value;
		if (value) //预览的时候必然自动重连
		{
			b_live_ = true;
			b_auto_connect_ = true;
			auto_connect_interval_ = 1000;
			auto_connect_cnt_ = -1;
		}
	}

private:
	bool 		open_mux();
	int 		write_mux(const ins_frame* frame);
	void 		task();
	bool		av_sync();
	int32_t 	dequeue_and_write();
	std::shared_ptr<ins_frame> dequeue_av_frame();
	void 		check_fragment(const ins_frame* frame);
	void 		check_disk_space(long long pts);
	void 		create_new_mux(int64_t pts);
	void 		do_discard_frame(bool b_keyframe);
	void 		create_new_url();
	int 		key_frame_to_jpeg(const std::shared_ptr<ins_frame>& frame);
	void 		start_pts_to_prj_file(long long pts);
	void 		send_rec_over_msg(int errcode);
	void 		send_net_link_state_msg(bool b_connect);
	void 		do_net_disconnect();
	void 		do_reconnect();
	int 		do_mux_error(bool b_open);

	int32_t camm_tracks()
	{
		int32_t num = 0;
		if (b_gyro_ || b_exp_) num++;
		if (b_gps_) num++;
		return num;
	}

	std::shared_ptr<ins_gps_data> deque_gps_data(int64_t* next_pts = nullptr)
	{
		std::lock_guard<std::mutex> lock(mtx_);
		if (gps_queue_.empty()) return nullptr;
		auto data = gps_queue_.front();
		gps_queue_.pop();
		if (next_pts) {
			if (!gps_queue_.empty())  {
				*next_pts = gps_queue_.front()->pts;
			} else {
				*next_pts = LONG_LONG_MAX;
			}
		}
		return data;
	}

	std::shared_ptr<exposure_times> deque_exp_data(int64_t* next_pts = nullptr)
	{
		std::lock_guard<std::mutex> lock(mtx_);
		if (exp_queue_.empty()) return nullptr;
		auto data = exp_queue_.front();
		exp_queue_.pop();
		if (next_pts) {
			if (!exp_queue_.empty()) {
				*next_pts = exp_queue_.front()->pts;
			} else {
				*next_pts = LONG_LONG_MAX;
			}
		}
		return data;
	}

	std::shared_ptr<insbuff> deque_gyro_data()
	{
		std::lock_guard<std::mutex> lock(mtx_);
		if (gyro_queue_.empty()) return nullptr;
		auto data = gyro_queue_.front();
		gyro_queue_.pop();
		return data;
	}

	int32_t write_gps_frame(std::shared_ptr<ins_gps_data>& d)
	{
		// auto d = deque_gps_data(next_pts);
		// if (!d) return SINK_NO_FRAME;
		camm_util::gen_camm_gps_packet(d.get(), gps_buff_);
		ins_frame frame;
		frame.buf = gps_buff_;
		frame.media_type = INS_MEDIA_CAMM_GPS;
		frame.dts = frame.pts = d->pts;
		//printf("write gps pts:%ld queue:%lu\n", d->pts, gps_queue_.size());
		return write_mux(&frame);
	}

	int32_t write_exp_frame(int64_t* next_pts = nullptr)
	{
		auto d = deque_exp_data(next_pts);
		if (!d) return SINK_NO_FRAME;
		camm_util::gen_camm_exposure_packet(d.get(), exp_buff_);
		ins_frame frame;
		frame.buf = exp_buff_;
		frame.media_type = INS_MEDIA_CAMM_EXP;
		frame.dts = frame.pts = d->pts;
		//printf("write exp pts:%ld queue:%lu\n", d->pts, exp_queue_.size());
		return write_mux(&frame);
	}

	int32_t write_gyro_frame(amba_gyro_data* gyro)
	{
		#ifdef GYRO_EXT
		camm_util::gen_gyro_accel_mgm_packet(gyro, gyro_buff_);
		#else
		camm_util::gen_gyro_accel_packet(gyro, gyro_buff_);
		#endif
		
		ins_frame frame;
		frame.buf = gyro_buff_;
		frame.media_type = INS_MEDIA_CAMM_GYRO;

		frame.dts = frame.pts = gyro->pts - gyro_delta_ts_;
		
		//printf("write gyro pts:%ld queue:%lu\n", frame.pts, gyro_queue_.size());
		return write_mux(&frame);
	}

	std::thread 		th_;
	bool 				b_release_ = false;
	std::string 		url_;
	std::string 		path_;
	int 				url_sequence_ = -1;
	bool 				b_audio_ = false;
	bool 				b_video_ = false;
	bool 				b_live_ = false;
	bool 				b_preview_ = false;
	bool 				b_gps_ = false;
	bool 				b_gyro_ = false; //gyro/accel
	bool 				b_exp_ = false; //exposure
	int 				fragment_type_ = 0;
	bool 				b_override_ = false;
	bool 				b_just_last_frame_ = false;
	bool 				b_origin_key_ = false;//原始流中的key stream
	bool 				b_stitching_ = false; //拼接流
	std::shared_ptr<ins_audio_param> 				audio_param_;
	std::shared_ptr<ins_video_param> 				video_param_;
	std::shared_ptr<ins_mux> 						mux_;
	std::mutex 										mtx_;
	std::queue<std::shared_ptr<ins_frame>> 			video_queue_;
	std::queue<std::shared_ptr<ins_frame>> 			audio_queue_;
	std::queue<std::shared_ptr<ins_gps_data>> 		gps_queue_;
	std::queue<std::shared_ptr<insbuff>> 			gyro_queue_;
	std::queue<std::shared_ptr<exposure_times>> 	exp_queue_;
	std::shared_ptr<ins_frame> 						last_key_frame_;

	bool 						b_auto_connect_ = false;
	int 						auto_connect_interval_ = 0;
	int 						auto_connect_cnt_ = 0;
	int 						retry_cnt_ = 0;
	
	long long 					start_pts_ = INS_PTS_NO_VALUE;
	long long 					file_pos_ = 0;
	long long 					next_check_size_ = 25*1024*1024;
	int 						discard_frame_cnt_ = 0; //连续丢帧次数
	std::shared_ptr<ins_timer> 	timer_;
	double 						fps_ = 30.0;
	bool 						b_start_ = true;
	bool 						b_auto_start_ = true;
	int64_t 					gyro_delta_ts_ = 0;

	std::shared_ptr<insbuff> 	gps_buff_;
	std::shared_ptr<insbuff> 	gyro_buff_;
	std::shared_ptr<insbuff> 	exp_buff_;
};

#endif