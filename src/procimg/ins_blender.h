#ifndef __INS_BLENDER_H__
#define __INS_BLENDER_H__

#include <opencv2/opencv.hpp>
#include "Common/StitcherTypes.h"
#include <stdint.h>
#include <vector>
#include <string>
#include "common.h"
#include "cudaEGL.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "ins_frame.h"

class ins_blender
{
public:
    //type: INS_BLEND_TYPE_TEMPLATE  INS_BLEND_TYPE_OPTFLOW  1
    //fmt:  BLENDERTYPE::YUV420P_CUDA BLENDERTYPE::RAW_CUDA BLENDERTYPE::BGR_CUDA
    ins_blender(uint32_t type, uint32_t mode, ins::BLENDERTYPE fmt); 
    ~ins_blender();
    void set_map_type(uint32_t map);
    void set_rotation(double* mat);
    void set_speed(ins::SPEED speed);
    void set_optflow_type(ins::OPTFLOWTYPE type);
    void set_logo(std::string filename); //必须在setup后调用
    int32_t setup(const std::string& offset);
    int32_t blend(const std::vector<EGLImageKHR>& v_in_img, EGLImageKHR& out_img, bool correlation = false); //yuv420 nv12
    int32_t blend(const std::vector<ins_img_frame>& v_in, ins_img_frame& out); //bgr raw
    
private:
    int32_t blend_yuv420(std::vector<CUeglFrame>& v_src_frame, CUeglFrame& dst_frame, bool correlation);
    int32_t blend_nv12(std::vector<CUeglFrame>& v_src_frame, CUeglFrame& dst_frame, bool correlation);
    int32_t do_blender(const std::vector<cv::_InputArray>& in, std::vector<cv::_InputOutputArray>& out, bool correlation);
    uint32_t blend_type_ = INS_BLEND_TYPE_TEMPLATE;
    ins::BLENDERTYPE blend_fmt_;
    void* handle_ = nullptr;
};

#endif