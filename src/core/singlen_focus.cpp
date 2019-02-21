
#include "singlen_focus.h"
#include "inslog.h"
#include "common.h"
#include "singlen_render.h"
#include "nv_video_dec.h"
#include "nvbuf_utils.h"
#include "NvBuffer.h"
#include "ins_frame.h"
#include <unistd.h>
#include "one_cam_video_buff.h"

singlen_focus::~singlen_focus()
{
    quit_ = true;
	INS_THREAD_JOIN(th_dec_);
	INS_THREAD_JOIN(th_render_);
}

int32_t singlen_focus::open(float x, float y)
{	
	dec_ = std::make_shared<nv_video_dec>();
	auto ret = dec_->open("dec", "h264");
	RETURN_IF_TRUE(ret != INS_OK, "decode open fail", ret);

	th_render_ = std::thread(&singlen_focus::render_task, this, x, y);
	th_dec_ = std::thread(&singlen_focus::dec_task, this);

	return INS_OK;
}

void singlen_focus::dec_task()
{
    int ret;
	int64_t pts;
	NvBuffer* buff = nullptr;
    while (1)
    {
        ret = dec_->dequeue_input_buff(buff, 20);
        if (ret == INS_ERR_TIME_OUT)
        {
            continue;
        }
        else if (ret != INS_OK)
        {
            break;
        }
        else
        {
            ret = dequeue_frame(buff, pts);
            if (ret != INS_OK) buff->planes[0].bytesused = 0;
            dec_->queue_input_buff(buff, pts);
            if (ret != INS_OK) break;
        }
    }

	dec_exit_ = true;

	LOGINFO("dec task exit");
}

void singlen_focus::render_task(float x, float y)
{
	int ret;
	auto render = std::make_shared<singlen_render>();
    ret = render->setup(x, y);
	if (ret != INS_OK) return;
	auto egl_display = render->get_display();

	int64_t pts;
	NvBuffer* buff;
	while (!quit_ || !dec_exit_)
	{
        ret = dec_->dequeue_output_buff(buff, pts, 0);
		BREAK_IF_NOT_OK(ret);

		auto eglimg = NvEGLImageFromFd(egl_display, buff->planes[0].fd); 
		BREAK_IF_TRUE((eglimg == nullptr), "NvEGLImageFromFd fail");
		render->draw(eglimg);
		NvDestroyEGLImage(egl_display, eglimg);

		dec_->queue_output_buff(buff);

		print_fps_info();
	}

	render = nullptr;

	LOGINFO("render task exit");
}

int32_t singlen_focus::dequeue_frame(NvBuffer* buff, int64_t& pts)
{
    while (!quit_)
    {
        auto frame = repo_->deque_frame();
        if (frame == nullptr)
        {
            usleep(5*1000);
            continue;
        }
        else
        {
            memcpy(buff->planes[0].data, frame->page_buf->data(), frame->page_buf->offset());
            buff->planes[0].bytesused = frame->page_buf->offset();
            pts = frame->pts;
            return INS_OK;
        }
    }
    return INS_ERR;
}

void singlen_focus::print_fps_info() const
{
	static int cnt = -1;
	static struct timeval start_time;
	static struct timeval end_time;

	if (cnt == -1)
	{
		gettimeofday(&start_time, nullptr);
	}

	if (cnt++ > 60)
	{
		gettimeofday(&end_time, nullptr);
		double fps = 1000000.0*cnt/(double)(end_time.tv_sec*1000000 + end_time.tv_usec - start_time.tv_sec*1000000 - start_time.tv_usec);
		start_time = end_time;
		cnt = 0;
		LOGINFO("fps: %lf", fps);
	}
}
