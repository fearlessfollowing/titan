#ifndef _VIDEO_STITCHER_H_
#define _VIDEO_STITCHER_H_

#include <memory>
#include <thread>
#include <string>
#include <vector>

#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "NvBuffer.h"

#include "nv_video_dec.h"
#include "nv_video_enc.h"
#include "insdemux.h"

class ins_blender;

class video_stitcher
{
public:
    ~video_stitcher();
    int32_t open(std::vector<std::string> in_file, std::string out_file, uint32_t width, uint32_t height, uint32_t bitrate);

private:
    void dec_task(int32_t index);
    void enc_task();
    int32_t read_frame(NvBuffer* buff, int32_t index);
    int32_t frame_stitching(std::vector<NvBuffer*>& v_dec_buff, NvBuffer* enc_buff);

    EGLDisplay egl_display_ = nullptr;
    std::vector<std::shared_ptr<ins_demux>> demux_;
    std::vector<std::shared_ptr<nv_video_dec>> dec_;
    std::shared_ptr<nv_video_enc> enc_;
    std::shared_ptr<ins_blender> blender_;
    bool quit_ = false;
    std::thread th_dec_[6];
    std::thread th_enc_;
};

#endif