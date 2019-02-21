
#include <sys/time.h>
#include "inslog.h"
#include "common.h"
#include "insdemux.h"
#include "ffh264dec.h"
#include "ffutil.h"

void print_fps_info()
{
	static int cnt = -1;
	static struct timeval start_time;
	static struct timeval end_time;

	if (cnt == -1)
	{
		gettimeofday(&start_time, nullptr);
	}

	if (cnt++ > 30)
	{
		gettimeofday(&end_time, nullptr);
		double fps = 1000000.0*cnt/(double)(end_time.tv_sec*1000000 + end_time.tv_usec - start_time.tv_sec*1000000 - start_time.tv_usec);
		start_time = end_time;
		cnt = 0;
		LOGINFO("fps: %lf", fps);
	}
}

int main(int argc, char* argv[])
{
	ins_log::init("/sdcard", "log");

	if (argc < 2)
	{
		LOGERR("please input mp4 file");
		return -1;
	}

	ffutil::init();

	ins_demux demux;
	if (INS_OK != demux.open(argv[1]))
	{
		return -1;
	}	

	ins_demux_param demux_param;
	demux.get_param(demux_param);

	ffh264dec_param dec_param;
	dec_param.width = demux_param.width;
	dec_param.height = demux_param.height;
	dec_param.sps = demux_param.sps;
	dec_param.sps_len = demux_param.sps_len;
	dec_param.pps = demux_param.pps;
	dec_param.pps_len = demux_param.pps_len;
	dec_param.threads = 4;

	ffh264dec decode;
	if (INS_OK != decode.open(dec_param))
	{
		return -1;
	}

	while (1)
	{
		std::shared_ptr<ins_demux_frame> frame;
		if (INS_OK != demux.get_next_frame(frame))
		{
			LOGERR("demux over");
			break;
		}

		if (frame == nullptr)
		{
			break;
		}

		if (frame->media_type != INS_MEDIA_VIDEO)
		{
			continue;
		}

		auto rgb = decode.decode(frame->data, frame->len, frame->pts, frame->dts);
		if (rgb)
		{
			print_fps_info();
		}
	}
}

