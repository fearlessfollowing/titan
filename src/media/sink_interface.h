
#ifndef _SINK_INTERFACE_
#define _SINK_INTERFACE_

#include <memory>
#include <string>
#include <map>
#include <functional>
#include "ins_frame.h"
#include "insbuff.h"
#include "gps_sink.h"
#include "gyro_sink.h"

struct ins_audio_param
{
	std::string mime;
	unsigned int samplerate = 0;
	unsigned char channels = 0;
	unsigned int bitrate = 0; //kbit
	bool b_spatial = false;
	unsigned int a_timebase = 1000000;
	std::shared_ptr<insbuff> config;
};

struct ins_video_param
{
	std::string mime;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int bitrate = 0; //kbit
	double fps = 0;
	int spherical_mode; 
	unsigned int v_timebase = 1000000;
	std::shared_ptr<insbuff> config;
};

class sink_interface : public gps_sink, public gyro_sink
{
public:
	virtual ~sink_interface() {};
	virtual void set_audio_param(const ins_audio_param& param) = 0;
	virtual void set_video_param(const ins_video_param& param) = 0;
	virtual bool is_audio_param_set() = 0;
	virtual bool is_video_param_set() = 0;
	virtual void queue_frame(const std::shared_ptr<ins_frame>& frame) = 0;
	virtual void set_event_cb(std::function<void(int)> event_cb)
	{
		event_cb_ = event_cb;
	}
	virtual void set_mux_io_option(std::map<std::string, std::string>& option)
	{
		mux_io_opt_ = option;
	}
	virtual void flush()
	{
		b_flush_ = true;
	}
protected:
	std::function<void(int)> event_cb_;
	std::map<std::string, std::string> mux_io_opt_;
	bool b_flush_ = false;
};

#endif
