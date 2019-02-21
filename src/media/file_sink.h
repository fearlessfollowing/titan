
#ifndef _FILE_SINK_H_
#define _FILE_SINK_H_

#include <thread>
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <vector>
#include "insmux.h"
#include "sink_interface.h"
#include "ins_timer.h"

class file_sink : public sink_interface
{
public:
	virtual ~file_sink() override;
	file_sink(std::string url, bool b_audio, bool b_video, bool b_camm = false);
	virtual void queue_gps(std::shared_ptr<ins_gps_data>& gps) {};
	virtual void set_audio_param(const ins_audio_param& param) override;
	virtual void set_video_param(const ins_video_param& param) override;
	virtual bool is_audio_param_set() override
	{
		return audio_param_ != nullptr;
	};
	virtual bool is_video_param_set() override
	{
		return video_param_ != nullptr;
	};
	virtual void queue_frame(const std::shared_ptr<ins_frame>& frame) override;
private:
	std::shared_ptr<ins_frame> dequeue_frame();
	bool open_mux();
	int write_mux(const std::shared_ptr<ins_frame>& frame);
	void release();
	void task();
	bool check_disk_space();
	void pending_tail(const std::string& url);

	std::string url_;
	bool quit_ = false;
	bool b_audio_ = false;
	bool b_video_ = false;
	bool b_camm_ = false;
	std::shared_ptr<ins_audio_param> audio_param_;
	std::shared_ptr<ins_video_param> video_param_;
	std::shared_ptr<ins_mux> mux_;
	std::mutex mtx_;
	std::thread th_;
	std::deque<std::shared_ptr<ins_frame>> video_queue_;
	std::deque<std::shared_ptr<ins_frame>> audio_queue_;
	std::deque<std::shared_ptr<ins_frame>> camm_queue_;
};

#endif