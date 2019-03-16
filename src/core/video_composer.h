#ifndef _VIDEO_COMPOSER_H_
#define _VIDEO_COMPOSER_H_

#include <memory>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include "common.h"

#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "NvBuffer.h"
#include "ins_clock.h"
#include "Arational.h"
#include "stream_sink.h"
#include "compose_option.h"

class nv_video_enc;
class nv_video_dec;
class all_cam_video_queue;
class stabilization;
class render;
class nv_jpeg_enc;
class jpeg_preview_sink;

class video_composer {
public:
    		~video_composer();
    int32_t open(const compose_option& option);
    void 	set_video_repo(std::shared_ptr<all_cam_video_queue> repo) {
        repo_ = repo;
    }
	
    void set_stabilizer(std::shared_ptr<stabilization> stablz) {
        std::lock_guard<std::mutex> lock(mtx_stablz_);
        stablz_ = stablz;   
    }
	
    int32_t add_encoder(const compose_option& option);
    void delete_encoder(uint32_t index);
	
    void encoder_del_output(uint32_t enc_index, uint32_t sink_index);
    void encoder_add_output(uint32_t enc_index, uint32_t sink_index, std::shared_ptr<sink_interface> sink);

    static int32_t 	init_cuda();
    static bool 	hdmi_change_;
    static bool 	in_hdmi_;

private:
    void 	dec_task(int32_t index);
    void 	enc_task();
    void 	screen_render_task();
    int32_t compose(std::vector<NvBuffer*>& v_in_buff, std::map<uint32_t, NvBuffer*>& m_out_buff, const float* mat = nullptr);
    int32_t dequeue_frame(NvBuffer* buff, int32_t index, int64_t& pts);
    void 	print_fps_info() ;
    void 	re_start_encoder(uint32_t index);

    void 	queue_free_rend_img(void* img) {
        std::lock_guard<std::mutex> lock(mtx_rend_img_);
        free_rend_img_queue_.push(img);   
    }

    void 	queue_full_rend_img(void* img) {
        std::lock_guard<std::mutex> lock(mtx_rend_img_);
        full_rend_img_queue_.push(img);   
    }

    void* 	deque_full_rend_img() {
        std::lock_guard<std::mutex> lock(mtx_rend_img_);
        if (full_rend_img_queue_.empty()) return nullptr;
        auto img = full_rend_img_queue_.front();
        full_rend_img_queue_.pop();
        return img;
    }

    void* deque_free_rend_img() {
        std::lock_guard<std::mutex> lock(mtx_rend_img_);
        if (!free_rend_img_queue_.empty()) {
            auto img = free_rend_img_queue_.front();
            free_rend_img_queue_.pop();
            return img;
        } else if (!full_rend_img_queue_.empty()) {
            auto img = full_rend_img_queue_.front();
            full_rend_img_queue_.pop();
            return img;
        } else {
            abort();
        }
    }

    EGLDisplay egl_display_ = nullptr;
    std::vector<std::shared_ptr<nv_video_dec>> dec_;
    std::map<uint32_t,std::shared_ptr<nv_video_enc>> enc_;
    std::shared_ptr<render> render_;
    std::shared_ptr<all_cam_video_queue> repo_;
    std::map<uint32_t, compose_option> option_;
    std::map<uint32_t, int32_t> modes_;
    int32_t 			rend_mode_ = 0;
    std::map<uint32_t, uint32_t> enc_intervals_;
    Arational 			dec_fps_;
    bool 				quit_ = false; 
    uint32_t 			dec_exit_num_ = 0;
    std::thread 		th_dec_[INS_CAM_NUM];
    std::thread 		th_enc_;
    std::mutex 			mtx_;
    std::mutex 			mtx_stablz_;
    std::shared_ptr<ins_clock> clock_;
    int32_t cnt_ = -1;
    uint32_t sec_enc_interval_ = 1; //第二路编码流和第一路帧率不一样的时候，每个多少帧编码一次第二路流
    bool render_setup_ = false;
    bool b_gpu_err_ = false;
    std::shared_ptr<stabilization> stablz_;

    int32_t jpeg_index_ = -1;
    std::shared_ptr<nv_jpeg_enc> jpeg_enc_;
    std::shared_ptr<jpeg_preview_sink> jpeg_sink_;

    std::thread 		th_screen_render_;
    std::mutex 			mtx_rend_img_;
    std::queue<void*> 	free_rend_img_queue_;
    std::queue<void*> 	full_rend_img_queue_;
};

#endif