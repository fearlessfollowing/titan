
#ifndef _INS_MUX_H_
#define _INS_MUX_H_

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "common.h"
#include "inslog.h"
#include "metadata.h"

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/opt.h"
#include "libavutil/error.h"
#include "libavutil/intreadwrite.h"
}

#define INS_SPERICAL_MONO "mono"
#define INS_SPERICAL_LEFT_RIGHT "left-right"
#define INS_SPERICAL_TOP_BOTTOM "top-bottom"

struct ins_mux_param
{
	bool has_video = false;
	std::string video_mime;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int v_timebase = 1000000;
	unsigned char* v_config = nullptr;
	unsigned int v_config_size = 0;
	unsigned int v_bitrate = 0; //kbit
	double fps = 0.0;
    std::string spherical_mode; //left-right/top-bottom/mono
	std::string gps_metadata; //xmp metadata
  
	bool has_audio = false;
	std::string audio_mime;
	unsigned int samplerate = 0;
	unsigned char channels = 0;
	unsigned int a_timebase = 1000000;
	unsigned char* a_config = nullptr;
	unsigned int a_config_size = 0;
	unsigned int audio_bitrate = 0; //kbit
	bool b_spatial_audio = false;

	int camm_tracks = 0;
	unsigned int c_timebase = 1000000;
    
    std::map<std::string, std::string> mux_options;
    std::map<std::string, std::string> io_options;
};

struct ins_mux_frame
{
	unsigned char* data = nullptr;
	unsigned int size = 0;
	long long pts = 0;
	long long dts = 0;
	long long duration = 0;
	unsigned char media_type = 0;
	bool b_key_frame = false;
	long long position = 0;
};

class ins_mux
{
public:
	~ins_mux() 
	{ 
		close(); 
	}
	int open(const ins_mux_param& param, const char* filename);
	int write(ins_mux_frame& frame);
	void flush();

private:
	void close();
	int init_audio(const ins_mux_param& param);
	int init_video(const ins_mux_param& param);
	int init_camm(const ins_mux_param& param);
	void get_stream_info_from_url(std::string url, std::string& format, std::string& protocal, bool& flush_packet);

	AVFormatContext* ctx_ = nullptr;
	AVStream* video_stream_ = nullptr;
	AVStream* audio_stream_ = nullptr;
	std::vector<AVStream*> v_camm_stream_;
	AVRational audio_src_ts_;
	AVRational video_src_ts_;
	AVRational camm_src_ts_;
	int64_t last_camm_pts_ = LONG_LONG_MIN;
    bool b_head_write_ = false;
	std::mutex mutex_;
};

#endif

