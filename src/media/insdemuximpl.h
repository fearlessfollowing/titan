
#ifndef _INS_DEMUX_IMPL_H
#define _INS_DEMUX_IMPL_H

extern "C" 
{
#include "libavformat/avformat.h"
}

#include <map>
#include <mutex>
#include <string>
#include "insdemux.h"

#define INS_DEMUX_PRI_NONE     0
#define INS_DEMUX_PRI_FRAME    1
#define INS_DEMUX_PRI_PIC      2
#define INS_DEMUX_PRI_PKT      3
#define INS_DEMUX_PRI_ARRY     4
#define INS_DEMUX_PRI_AVFREE   5

class InsDemuxImpl
{
public:

	int Open(const char* url, bool annexb, std::map<std::string, std::string>* options = nullptr);
	void Close();
	void VideoAudioParam(ins_demux_param& param);
	int GetNextFrame(std::shared_ptr<ins_demux_frame>& frame);
	int SeekFrame(double time, unsigned char media_type);
    char* GetMetadata(const char* key);
    void SetIoInterruptResult(int value);
    int io_interrupt_result_ = 0;
	
private:

	std::shared_ptr<ins_demux_frame> ParseAudioFrame(AVPacket* pkt);	
	std::shared_ptr<ins_demux_frame> ParseVideoFrame(AVPacket* pkt);
	int ParseNal(unsigned char*& data, unsigned int& len);
	void AnnexbToMp4(unsigned char* annexb, unsigned int annexb_len, unsigned char* mp4, unsigned int& mp4_len);
	bool StepToNextNal(unsigned char*& data, unsigned int& len);
	int NalStartCodeLen(unsigned char* data);
	void ParseSpsPps();
	void ParseAudioConfig();

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
	//std::shared_ptr<GyroInfo> gyro_;
};

#endif

