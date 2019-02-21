
#ifndef __MC_DECODE_H__
#define __MC_DECODE_H__

#include <gui/Surface.h>
#include <media/ICrypto.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <gui/GLConsumer.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <mutex>
#include "common.h"
#include "inslog.h"
#include "raw_video_queue.h"
#include <map>

using namespace android;

class mc_decode : public GLConsumer::FrameAvailableListener
{
public:
	virtual ~mc_decode()
	{
		close();
	};
	virtual void onFrameAvailable(const BufferItem& item) override;
	int open(const sp<AMessage>& format, int texture_id = 0);
	void close();
	int feed_data(const unsigned char* data, unsigned int size, long long pts, int timeout_us);
	int drain_decode(long long& pts, int timeout_us, bool b_flush = false);
	void update_tex_image();

	int drain_decode(int timeout_us, bool b_flush);
	void add_sink(int index, const std::shared_ptr<raw_video_queue>& sink);
	void delete_sink(int index);

private:
	void wait_frame_available();
	void queue_frame_to_sink(sp<ABuffer>& buff, long long pts);
	sp<MediaCodec> decode_;
	sp<GLConsumer> surfaceTexture_;
	sp<Surface> surface_;
	sp<ALooper> looper_;
	bool frameAvailable_ = false;
	std::mutex mtx_;
	std::condition_variable cv_;
	std::map<unsigned int, std::shared_ptr<raw_video_queue>> sink_;
	std::mutex sink_mtx_;
	int frames_ = 0;
};

#endif
