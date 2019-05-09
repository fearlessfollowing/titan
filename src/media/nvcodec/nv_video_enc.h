#ifndef __NV_VIDEO_ENC_H__
#define __NV_VIDEO_ENC_H__

#include "NvVideoEncoder.h"
#include "NvBuffer.h"
#include <string>
#include <mutex>
#include <map>
#include "sink_interface.h"
#include "Arational.h"
#include "obj_pool.h"

class nv_video_enc {
public:
    		~nv_video_enc() { close(); };
    int32_t open(std::string name, std::string mime);
    int32_t dequeue_input_buff(NvBuffer* &buff, uint32_t timeout_ms);
    int32_t queue_intput_buff(NvBuffer* buff, int64_t pts);
    void 	send_eos();
    bool 	capture_plane_done(struct v4l2_buffer *v4l2_buf, NvBuffer *buffer, NvBuffer *shared_buffer);

    void set_bitrate(uint32_t bitrate) {	// bits
        bitrate_ = bitrate;
    }

    void set_framerate(Arational framerate) {
        framerate_ = framerate;
    }
    
    void set_resolution(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
    }

    void add_output(uint32_t index, std::shared_ptr<sink_interface> sink) {
        std::lock_guard<std::mutex> lock(mtx_);
        m_sink_.insert(std::make_pair(index, sink));
    }
	
    void delete_output(uint32_t index) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = m_sink_.find(index);
        if (it == m_sink_.end()) return;
        m_sink_.erase(it);
    }

	/*
	 * Add by skymixos 	2019年5月7日
	 */
	void enableProfiling() {
		if (enc_) {
			enc_->enableProfiling();
		}
	}


	void printProfilingStats() {
		if (enc_) {
			enc_->printProfilingStats();
		}
	}



private:
    void close();
    void output_frame(NvBuffer* buffer, int64_t pts, bool b_keyframe);
    //void write_video_frame(NvBuffer* buffer);

    NvVideoEncoder* 			enc_ = nullptr;
    uint32_t 					width_ = 0;
    uint32_t 					height_ = 0;
    uint32_t 					bitrate_ = 0; 
    std::shared_ptr<insbuff> 	sps_pps_;
    Arational 					framerate_;
    std::string 				mime_;
    std::string 				name_;
    bool 						b_error_ = true;
    bool 						b_eos_ = false;
    int32_t 					out_plane_index_ = 0;
    struct v4l2_buffer 			v4l2_buf_;
    struct v4l2_plane 			planes_[MAX_PLANES];
    std::map<uint32_t, std::shared_ptr<sink_interface>> m_sink_;
    std::mutex 					mtx_;
    std::shared_ptr<obj_pool<ins_frame>> pool_;
	
    //FILE* fp_ = nullptr;
};

#endif