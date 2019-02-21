
#ifndef _INS_DEMUX_H_
#define _INS_DEMUX_H_

extern "C" 
{
#include "libavformat/avformat.h"
}
#include <memory>
#include <map>
#include <mutex>
#include <string>
#include "common.h"
#include "inslog.h"

#define INS_DEMUX_NONE      INS_MEDIA_NONE
#define INS_DEMUX_AUDIO     INS_MEDIA_AUDIO
#define INS_DEMUX_VIDEO     INS_MEDIA_VIDEO
#define INS_DEMUX_SUBTITLE  INS_MEDIA_SUBTITLE

#define INS_DEMUX_OK   INS_OK
#define INS_DEMUX_ERR  INS_ERR

struct ins_demux_param
{
	bool has_video = false;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int v_biterate = 0;
	double fps = 0.0;
	unsigned char* sps = nullptr; //no start code
	unsigned int sps_len = 0;
    unsigned char* pps = nullptr; //no start code
    unsigned int pps_len = 0;
	double v_duration = 0;
	long long v_frames = 0;

	bool has_audio = false;
	unsigned int samplerate = 0;
	unsigned char channels = 0;
	unsigned char channel_layout = 0;
	unsigned int a_biterate = 0;
	unsigned char* config = nullptr;
	unsigned int config_len = 0;
	double a_duration = 0;
	long long a_frames = 0;

	bool has_camm = false;
};

struct ins_demux_frame
{
	~ins_demux_frame();
	unsigned char* data = nullptr;
	unsigned int len = 0;
	long long pts = 0;
	long long dts = 0;
	long long duration;
	unsigned char media_type = INS_DEMUX_NONE;
	bool is_key = false;
	unsigned char private_type = 0;
	void* private_data = nullptr;
};

class ins_demux
{
public:
	~ins_demux() 
	{ 
		close(); 
	};
	int open(const char* url, bool annexb = true, std::map<std::string, std::string>* options = nullptr);                                     
	void get_param(ins_demux_param& param);
	int get_next_frame(std::shared_ptr<ins_demux_frame>& frame);
	int seek_frame(double time, unsigned char media_type); // time unit: second
    char* get_metadata(const char* key);

private:
	void close();
	std::shared_ptr<ins_demux_frame> parse_audio_frame(AVPacket* pkt);	
	std::shared_ptr<ins_demux_frame> parse_video_frame(AVPacket* pkt);
	int parse_nal(unsigned char*& data, unsigned int& len);
	void annexb_to_mp4(unsigned char* annexb, unsigned int annexb_len, unsigned char* mp4, unsigned int& mp4_len);
	bool step_to_next_nal(unsigned char*& data, unsigned int& len);
	int nal_startcode_len(unsigned char* data);
	void parse_sps_pps();
	void parse_audio_config();

	AVFormatContext* ctx_ = nullptr;
	AVBitStreamFilterContext* video_bfs_ = nullptr;
	AVBitStreamFilterContext* audio_bfs_ = nullptr;
	
	int video_stream_index_ = -1;
	int audio_stream_index_ = -1;
    int camm_stream_index_ = -1;
	unsigned char* sps_ = nullptr;
    unsigned int sps_len_ = 0;
    unsigned char* pps_ = nullptr;
	unsigned int pps_len_ = 0;
	bool is_in_annexb_ = false;
	bool is_out_annexb_ = true;
    std::mutex mutex_;
};

#endif

