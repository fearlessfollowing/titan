#ifndef _FILE_VIDEO_STITCH_
#define _FILE_VIDEO_STITCH_

#include <stdint.h>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include "file_stitcher.h"
#include "ins_clock.h"
#include "sink_interface.h"
#include "NvBuffer.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

class ins_demux;
class ins_blender;
class nv_video_dec;
class nv_video_enc;

class file_video_stitcher : public file_stitcher
{
public:
    virtual ~file_video_stitcher();
    virtual int32_t open(const task_option& option);
private:
    void dec_task(int32_t index);
    void enc_task();
    int32_t read_frame(NvBuffer* buff, int32_t index, int64_t& pts);
    int32_t frame_stitching(std::vector<NvBuffer*>& v_in_buff, NvBuffer* out_buff);
    int32_t calc_offset(const std::vector<std::string>& file, int32_t len_ver, double timestamp, std::string& offset);
    void print_fps_info();
    std::vector<std::shared_ptr<ins_demux>> demux_;
    std::vector<std::shared_ptr<nv_video_dec>> dec_;
    std::shared_ptr<nv_video_enc> enc_;
    std::shared_ptr<ins_blender> blender_;
    //std::shared_ptr<sink_interface> sink_;
    EGLDisplay egl_display_ = nullptr;
    std::thread dec_th_[6];
    std::thread enc_th_;
    bool quit_ = false;
    std::shared_ptr<ins_clock> clock_; //for calc stitching fps 
    int32_t cnt_ = -1; //for calc stitching fps 
    uint64_t cur_frames_ = 0;
    uint64_t total_frames_ = 0;
    double framerate_ = 30.0;
};

#endif