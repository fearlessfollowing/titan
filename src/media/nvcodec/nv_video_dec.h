#ifndef __NV_VIDEO_DEC_H__
#define __NV_VIDEO_DEC_H__

#include "NvVideoDecoder.h"
#include "NvVideoConverter.h"
#include "NvBuffer.h"
#include <string>
#include <thread>
#include "ins_queue.h"

class nv_video_dec {
public:
    		~nv_video_dec() { close(); };
			
    int32_t open(std::string name, std::string mime);
	
    int32_t dequeue_input_buff(NvBuffer* &buff, uint32_t timeout_ms);
    void 	queue_input_buff(NvBuffer* buff, int64_t pts); //buff = nullptr || buff->planes[0].bytesused = 0 => eos
    //void input_send_eos();
    int32_t dequeue_output_buff(NvBuffer* &buff, int64_t& pts, uint32_t timeout_ms);
    void 	queue_output_buff(NvBuffer* buff);
    bool 	conv_output_plane_done(struct v4l2_buffer *v4l2_buf,NvBuffer * buffer, NvBuffer * shared_buffer);
    void 	set_close_wait_time_ms(uint32_t time) { close_wait_time_ms_ = time; };

private:
    void 	close();
    int32_t setup_capture_plane();
    void 	capture_plane_loop();
    void 	conv_send_eos();

    NvVideoDecoder* 		dec_ = nullptr;
    NvVideoConverter* 		conv_ = nullptr;
    safe_queue<NvBuffer*> 	conv_outp_queue_;
    std::thread 			th_;
    std::string 			name_;
    std::string 			mime_;
    bool 					b_error_ = true;
    bool 					b_eos_ = false;
    bool 					b_conv_eos_  = false;
    int32_t 				out_plane_index_ = 0;
    struct v4l2_buffer 		v4l2_buf_;
    struct v4l2_plane 		planes_[MAX_PLANES];
    struct v4l2_buffer 		v4l2_buf_conv_cap_;
    struct v4l2_plane 		planes_conv_cap_[MAX_PLANES];
    uint32_t 				close_wait_time_ms_ = 2000;
};

#endif