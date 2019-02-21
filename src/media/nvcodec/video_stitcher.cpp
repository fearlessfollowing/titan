#include "ins_blender.h"
#include "video_stitcher.h"
#include "inslog.h"
#include "common.h"
#include "ffutil.h"
#include <sstream>
#include <cuda.h>
#include <cuda_runtime.h>
#include "cudaEGL.h"
#include "nvbuf_utils.h"

video_stitcher::~video_stitcher()
{
    LOGINFO("join stitch ------");

    for (int32_t i = 0; i < 6; i++)
    {
        INS_THREAD_JOIN(th_dec_[i]);
    }

    INS_THREAD_JOIN(th_enc_);

    if (egl_display_) 
    {
        eglTerminate(egl_display_);
        egl_display_ = nullptr;
    }

    demux_.clear();
    dec_.clear();
    enc_ = nullptr;
    LOGINFO("video_stitcher destroy");
}

int32_t video_stitcher::open(std::vector<std::string> in_file, std::string out_file, uint32_t width, uint32_t height, uint32_t bitrate)
{
    int32_t ret = 0;

    egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    RETURN_IF_TRUE(egl_display_ == EGL_NO_DISPLAY, "eglGetDisplay error", INS_ERR);

    ret = eglInitialize(egl_display_, nullptr, nullptr);
    RETURN_IF_TRUE(ret == 0, "eglInitialize error", INS_ERR);

    for (uint32_t i = 0; i < in_file.size(); i++)
    {
        auto demux = std::make_shared<ins_demux>();
        ret = demux->open(in_file[i].c_str());
        RETURN_IF_TRUE(ret != INS_OK, "demux open error", ret);
        ins_demux_param demux_param;
        demux->get_param(demux_param);
        demux_.push_back(demux);

        std::stringstream ss;
        ss << "dec" << i; 
        auto dec = std::make_shared<nv_video_dec>();
        ret = dec->open(ss.str(), "h264");
        RETURN_IF_TRUE(ret != INS_OK, "decode open fail", ret);
        dec_.push_back(dec);
    }
  
    enc_ = std::make_shared<nv_video_enc>();
    enc_->set_resolution(width, height);
    enc_->set_bitrate(bitrate*1000*1000);
    enc_->set_out_filename(out_file);
    ret = enc_->open("enc", "h264");
    RETURN_IF_TRUE(ret != INS_OK, "enc open fail", ret);

    std::string offset = "6_1280.000_1280.000_720.000_0.000_-0.000_-0.000_2560_1440_12_1280.000_1280.000_720.000_60.000_-0.000_-0.000_2560_1440_12_1280.000_1280.000_720.000_60.000_-180.000_-180.000_2560_1440_12_1280.000_1280.000_720.000_-0.000_-180.000_-180.000_2560_1440_12_1280.000_1280.000_720.000_-60.000_-180.000_-180.000_2560_1440_12_1280.000_1280.000_720.000_-60.000_-0.000_-0.000_2560_1440_12_7";
    blender_ = std::make_shared<ins_blender>(INS_BLEND_TYPE_OPTFLOW, INS_MODE_PANORAMA, ins::BLENDERTYPE::YUV420P_CUDA);
    blender_->set_map_type(INS_MAP_FLAT);
    blender_->set_speed(ins::SPEED::FAST);
    blender_->set_optflow_type(ins::OPTFLOWTYPE::OPTFLOW2);
    blender_->setup(offset);

    for (uint32_t i = 0; i < dec_.size(); i++)
    {
        th_dec_[i] = std::thread(&video_stitcher::dec_task, this, i);
    }
    th_enc_ = std::thread(&video_stitcher::enc_task, this);

    return INS_OK;
}

void video_stitcher::dec_task(int32_t index)
{
    int32_t ret = 0;
    NvBuffer* buff = nullptr;

    while (!quit_)
    {
        ret = dec_[index]->dequeue_input_buff(buff, 20);
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
            ret = read_frame(buff, index);
            if (ret != INS_OK) buff->planes[0].bytesused = 0;
            dec_[index]->queue_input_buff(buff);
            if (ret != INS_OK) break;
        }
    }

    LOGINFO("dec:%d task exit", index);
}

void video_stitcher::enc_task()
{
    int32_t ret = 0;

    while (!quit_)
    {
        std::vector<NvBuffer*> v_dec_buff;
        for (auto& dec : dec_)
        {
            NvBuffer* buff;
            ret = dec->dequeue_output_buff(buff, 0);
            if (ret != INS_OK) break;
            v_dec_buff.push_back(buff);
        }

        if (v_dec_buff.size() != dec_.size()) break;

        NvBuffer* enc_buff = nullptr;
        ret = enc_->dequeue_input_buff(enc_buff, 20);
        if (ret != INS_OK) break;

        for (int32_t i = 0; i < enc_buff->n_planes; i++)
        {
            auto &plane = enc_buff->planes[i];
            plane.bytesused = plane.fmt.stride * plane.fmt.height;
            //LOGINFO("plane:%d stride:%d height:%d", i, plane.fmt.stride, plane.fmt.height);
        }

        ret = frame_stitching(v_dec_buff, enc_buff);
        if (ret != INS_OK) break;

        for (uint32_t i = 0; i < dec_.size(); i++)
        {
            dec_[i]->queue_output_buff(v_dec_buff[i]);
        }

        enc_->queue_intput_buff(enc_buff);
    }

    enc_->send_eos();

    LOGINFO("enc task exit");
}

int32_t video_stitcher::read_frame(NvBuffer* buff, int32_t index)
{
    std::shared_ptr<ins_demux_frame> frame;
    while (1)
    {
        if (demux_[index]->get_next_frame(frame))
        {
            LOGINFO("demux:%d eos", index);
            return INS_ERR;
        }

        if (frame->media_type == INS_MEDIA_VIDEO)
        {
            memcpy(buff->planes[0].data, frame->data, frame->len);
            buff->planes[0].bytesused = frame->len;
            return INS_OK;
        }
    }
}

int32_t video_stitcher::frame_stitching(std::vector<NvBuffer*>& v_in_buff, NvBuffer* out_buff)
{
    std::vector<EGLImageKHR> v_in_img;
    for (uint32_t i = 0; i < v_in_buff.size(); i++)
    {
        auto img = NvEGLImageFromFd(egl_display_, v_in_buff[i]->planes[0].fd); 
        RETURN_IF_TRUE(img == nullptr, "NvEGLImageFromFd fail", INS_ERR);
        v_in_img.push_back(img);
    }

    auto out_img = NvEGLImageFromFd(egl_display_, out_buff->planes[0].fd); 
    RETURN_IF_TRUE(out_img == nullptr, "NvEGLImageFromFd fail", INS_ERR);

    auto ret = blender_->blend(v_in_img, out_img, true);
    
    for (uint32_t i = 0; i < v_in_img.size(); i++)
    {
        NvDestroyEGLImage(egl_display_, v_in_img[i]);
    }

    NvDestroyEGLImage(egl_display_, out_img);

    return ret;
}

#if 0
int32_t video_stitcher::frame_stitching(std::vector<NvBuffer*>& v_dec_buff, NvBuffer* enc_buff)
{
    cudaFree(0);

    CUresult status;

    std::vector<EGLImageKHR> v_src_img;
    std::vector<CUgraphicsResource> v_src_res;
    std::vector<CUeglFrame> v_src_frame;
    for (uint32_t i = 0; i < v_dec_buff.size(); i++)
    {
        auto img = NvEGLImageFromFd(egl_display_, v_dec_buff[i]->planes[0].fd); 
        if (img == nullptr)
        {
            LOGERR("NvEGLImageFromFd fail");
            return INS_ERR;
        }
        v_src_img.push_back(img);

        CUgraphicsResource res;
        status = cuGraphicsEGLRegisterImage(&res, img, CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("cuGraphicsEGLRegisterImage fail");
            return INS_ERR;
        }

        v_src_res.push_back(res);

        CUeglFrame frame;
        status = cuGraphicsResourceGetMappedEglFrame(&frame, res, 0, 0);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("cuGraphicsResourceGetMappedEglFrame fail");
            return INS_ERR;
        }

        v_src_frame.push_back(frame);
    }

    auto dst_img = NvEGLImageFromFd(egl_display_, enc_buff->planes[0].fd); 
    if (dst_img == nullptr)
    {
        LOGERR("NvEGLImageFromFd fail");
        return INS_ERR;
    }

    CUgraphicsResource dst_res;
    status = cuGraphicsEGLRegisterImage(&dst_res, dst_img, CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
    if (status != CUDA_SUCCESS)
    {
        LOGERR("cuGraphicsEGLRegisterImage fail");
        return INS_ERR; 
    }
 
    CUeglFrame dst_frame;
    status = cuGraphicsResourceGetMappedEglFrame(&dst_frame, dst_res, 0, 0);
    if (status != CUDA_SUCCESS)
    {
        LOGERR("cuGraphicsResourceGetMappedEglFrame fail");
        return INS_ERR;
    }

    status = cuCtxSynchronize();   
    if (status != CUDA_SUCCESS)
    { 
        LOGERR("cuCtxSynchronize fail"); 
        return INS_ERR;  
    } 

    blender_->blend(v_src_frame, dst_frame, true);
 
    //blender
    // int32_t ret = template_blend_frame(blender_, v_src_frame, dst_frame);
    // if (ret != 1) 
    // {   
    //     LOGERR("template_blend_frame ret:%d", ret);  
    // }

    status = cuCtxSynchronize();
    if (status != CUDA_SUCCESS)
    {
        LOGERR("cuCtxSynchronize fail"); 
        return INS_ERR;
    }

    for (uint32_t i = 0; i < v_dec_buff.size(); i++)
    {
        status = cuGraphicsUnregisterResource(v_src_res[i]);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("cuGraphicsUnregisterResource fail");
        }
        NvDestroyEGLImage(egl_display_, v_src_img[i]);
    }

    status = cuGraphicsUnregisterResource(dst_res);
    if (status != CUDA_SUCCESS)
    {
        LOGERR("cuGraphicsUnregisterResource fail");
    }

    NvDestroyEGLImage(egl_display_, dst_img);

    return INS_OK;
}
#endif

