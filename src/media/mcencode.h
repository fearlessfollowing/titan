#ifndef __MC_ENCODE_H__
#define __MC_ENCODE_H__

#include <gui/Surface.h>
#include <media/ICrypto.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>

#include "common.h"
#include <map>
#include <mutex>
#include "sink_interface.h"

using namespace android;

#define BUFF_TYPE_PAGE 0
#define BUFF_TYPE_NORMAL 1

class mc_encode
{
public:
	virtual ~mc_encode()
	{
		close();
	};
	int open(const sp<AMessage>& format);
	void set_spatial_audio(bool b_spatial) { b_spatial_ = b_spatial; };
	void set_buff_type(int type) { buff_type_ = type; };
	Surface* create_input_surface();
	int start();
	void close();
	int feed_data(const unsigned char* data, unsigned int size, long long pts, int timeout_us);
	int drain_encode(bool eos, int timeout_us);
	void add_stream_sink(unsigned int index, const std::shared_ptr<sink_interface>& sink);
	void del_stream_sink(unsigned int index);

private:
	void write_frame(sp<ABuffer>& buff, long long pts, bool is_key_frame);
	void parse_media_format(sp<AMessage>& format);
	void parse_codec_config(sp<ABuffer>& buff);
	bool init_sink(std::shared_ptr<sink_interface>& sink, bool is_key_frame);
	sp<MediaCodec> encode_;
	sp<ALooper> looper_;
	sp<Surface> surface_;
	unsigned char media_type_ = INS_MEDIA_NONE;
	unsigned int bitrate_ = 0;  //kbit
	float fps_ = 0;
	int samplerate_ = 0;
	bool b_spatial_ = false;
	std::map<unsigned int, std::shared_ptr<sink_interface>> sink_;
	std::mutex mtx_;
	std::shared_ptr<ins_video_param> video_param_;
	std::shared_ptr<ins_audio_param> audio_param_;
	int buff_type_ = BUFF_TYPE_PAGE;
};

#endif
