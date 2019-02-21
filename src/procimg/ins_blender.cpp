
#include "SeamlessBlender/SeamlessBlender.h"
#include "TemplateBlender/TemplateBlender.h"
#include "inslog.h"
#include "ins_clock.h"
#include "ins_blender.h"
#include <cuda_runtime.h>
#include "ffpngdec.h"

using namespace ins;
using namespace cv;

ins_blender::~ins_blender()
{
    if (handle_ == nullptr) return;

    if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
    {
        delete (SeamlessBlender*)handle_;
    }
    else
    {
        delete (TemplateBlender*)handle_;
    }

    handle_ = nullptr;
}

ins_blender::ins_blender(uint32_t type, uint32_t mode, ins::BLENDERTYPE fmt)
    : blend_fmt_(fmt)
    , blend_type_(type)
{
    PANOTYPE pano_type;
    if (mode == INS_MODE_3D_TOP_LEFT) 
    {
        pano_type = PANOTYPE::LEFT_RIGHT;
    }
    else if (mode == INS_MODE_3D_TOP_RIGHT) 
    {
        pano_type = PANOTYPE::RIGHT_LEFT;
    }
    else
    {
        pano_type = PANOTYPE::PANO;
    }

    assert(fmt == BLENDERTYPE::YUV420P_CUDA 
        || fmt == BLENDERTYPE::RAW_CUDA 
        || fmt == BLENDERTYPE::BGR_CUDA 
        || fmt == BLENDERTYPE::NV12_CUDA);

    if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
    {
        handle_ = (void*) new SeamlessBlender(fmt, pano_type);
        ((SeamlessBlender*)handle_)->setComputeCapability(5.0);
        // ((SeamlessBlender*)handle_)->setOptflowType(ins::OPTFLOWTYPE::OPTFLOW2);
        // ((SeamlessBlender*)handle_)->setSpeed(SPEED::FAST);
    }
    else
    {
        handle_ = (void*) new TemplateBlender(fmt, pano_type);
        ((TemplateBlender*)handle_)->setComputeCapability(5.0);
    }
}

void ins_blender::set_map_type(uint32_t map)
{
    TRANSFORMTYPE transform_type;
    if (map == INS_MAP_CUBE) 
    {
        transform_type = TRANSFORMTYPE::CUBE;
    }
    else
    {
        transform_type = TRANSFORMTYPE::SPHERE;
    }

    if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
    {
        ((SeamlessBlender*)handle_)->setTransformType(transform_type);
    }
    else
    {
        ((TemplateBlender*)handle_)->setTransformType(transform_type);
    }
}    

void ins_blender::set_speed(ins::SPEED speed)
{
    if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
    {
        ((SeamlessBlender*)handle_)->setSpeed(speed);
    }
    else
    {
        LOGINFO("templete blender not support set speed");
    }
}    

void ins_blender::set_optflow_type(ins::OPTFLOWTYPE type)
{
    if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
    {
        ((SeamlessBlender*)handle_)->setOptflowType(type);
    }
    else
    {
        LOGINFO("templete blender not support set optical flow type");
    }
} 

void ins_blender::set_rotation(double* mat)
{
    Mat mat_cpu = (Mat_<float>(3, 3) << mat[0], mat[1], mat[2], mat[3], mat[4], mat[5], mat[6], mat[7], mat[8]);
    cuda::GpuMat mat_gpu(mat_cpu);
    if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
    {
        ((SeamlessBlender*)handle_)->setRotation(mat_gpu);
    }
    else
    {
        ((TemplateBlender*)handle_)->setRotation(mat_gpu);
    }
}

//必须在setup后调用
void ins_blender::set_logo(std::string filename)
{
    if (filename == "") return;

    ffpngdec dec;
    auto frame = dec.decode(filename);
    if (!frame) return;

    if (2.0*frame->h/frame->w >= 1.0)
    {
        LOGERR("logo w:%d h:%d not support", frame->w, frame->h);
        return;
    }

    if (frame->fmt == AV_PIX_FMT_RGBA)  //rgba
    {
        Mat cpu_img(frame->h, frame->w, CV_8UC4, frame->buff->data());
        cvtColor(cpu_img, cpu_img, CV_RGBA2BGR);
        cuda::GpuMat gpu_img(cpu_img);
        if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
        {
            ((SeamlessBlender*)handle_)->setLogoImage(gpu_img);
        }
        else
        {
            ((TemplateBlender*)handle_)->setLogoImage(gpu_img);
        }
    }
    else  if (frame->fmt == AV_PIX_FMT_RGB24)
    {
        Mat cpu_img(frame->h, frame->w, CV_8UC3, frame->buff->data());
        cvtColor(cpu_img, cpu_img, CV_RGB2BGR);
        cuda::GpuMat gpu_img(cpu_img);
        if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
        {
            ((SeamlessBlender*)handle_)->setLogoImage(gpu_img);
        }
        else
        {
            ((TemplateBlender*)handle_)->setLogoImage(gpu_img);
        }
    }
    else 
    {
        LOGERR("png decoded img fmt:%d not support", frame->fmt);
    }
}

// void ins_blender::set_logo_img(int32_t pix_fmt, int32_t w, int32_t h, uint8_t* img)
// {
//     if (pix_fmt == 1)  //rgba
//     {
//         Mat cpu_img(h, w, CV_8UC4, img);
//         cvtColor(cpu_img, cpu_img, CV_RGBA2BGR);
//         cuda::GpuMat gpu_img(cpu_img);
//         if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
//         {
//             ((SeamlessBlender*)handle_)->setLogoImage(gpu_img);
//         }
//         else
//         {
//             ((TemplateBlender*)handle_)->setLogoImage(gpu_img);
//         }
//     }
//     else  //rgb
//     {
//         Mat cpu_img(h, w, CV_8UC3, img);
//         cvtColor(cpu_img, cpu_img, CV_RGB2BGR);
//         cuda::GpuMat gpu_img(cpu_img);
//         if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
//         {
//             ((SeamlessBlender*)handle_)->setLogoImage(gpu_img);
//         }
//         else
//         {
//             ((TemplateBlender*)handle_)->setLogoImage(gpu_img);
//         }
//     }
// }

int32_t ins_blender::setup(const std::string& offset)
{
    int32_t ret;
    if (blend_type_ == INS_BLEND_TYPE_OPTFLOW)
    {
        ret = ((SeamlessBlender*)handle_)->init(offset);
    }
    else
    {
        ret = ((TemplateBlender*)handle_)->init(offset);
    }
    
    if (ret < 0)
    {
        LOGERR("blender init fail:%d", ret);
        return INS_ERR;
    }
    else
    {
        return INS_OK;
    }
}

int32_t ins_blender::blend(const std::vector<EGLImageKHR>& v_in_img, EGLImageKHR& out_img, bool correlation)
{
    cudaFree(0);

    assert(blend_fmt_ == BLENDERTYPE::YUV420P_CUDA || blend_fmt_ == BLENDERTYPE::NV12_CUDA);

    CUresult status;
    std::vector<CUgraphicsResource> v_src_res;
    std::vector<CUeglFrame> v_src_frame;
    for (uint32_t i = 0; i < v_in_img.size(); i++)
    {
        CUgraphicsResource res;
        status = cuGraphicsEGLRegisterImage(&res, v_in_img[i], CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("cuGraphicsEGLRegisterImage i:%d fail", i);
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

    CUgraphicsResource dst_res;
    status = cuGraphicsEGLRegisterImage(&dst_res, out_img, CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
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
 
    if (blend_fmt_ == BLENDERTYPE::YUV420P_CUDA)
    {
        blend_yuv420(v_src_frame, dst_frame, correlation);
    }
    else
    {
        blend_nv12(v_src_frame, dst_frame, correlation);
    }

    status = cuCtxSynchronize();
    if (status != CUDA_SUCCESS)
    {
        LOGERR("cuCtxSynchronize fail"); 
        return INS_ERR;
    }

    for (uint32_t i = 0; i < v_src_res.size(); i++)
    {
        status = cuGraphicsUnregisterResource(v_src_res[i]);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("cuGraphicsUnregisterResource fail");
        }
    }

    status = cuGraphicsUnregisterResource(dst_res);
    if (status != CUDA_SUCCESS)
    {
        LOGERR("cuGraphicsUnregisterResource fail");
    }

    return INS_OK;
}

int32_t ins_blender::blend_nv12(std::vector<CUeglFrame>& v_src_frame, CUeglFrame& dst_frame, bool correlation)
{
    ins_clock t;

    std::vector<cuda::GpuMat> v_in_y_img;
    std::vector<cuda::GpuMat> v_in_uv_img;
    for (unsigned i = 0; i < v_src_frame.size(); i++)
    {
        cuda::GpuMat y_img(v_src_frame[i].height, v_src_frame[i].width, CV_8UC1, v_src_frame[i].frame.pPitch[0], v_src_frame[i].pitch);
        v_in_y_img.push_back(y_img);

        cuda::GpuMat uv_img(v_src_frame[i].height/2, v_src_frame[i].width/2, CV_8UC2, v_src_frame[i].frame.pPitch[1], v_src_frame[i].pitch);
        v_in_uv_img.push_back(uv_img);
    }

    std::vector<cv::_InputArray> vv_in_img;
    vv_in_img.push_back(v_in_y_img);
    vv_in_img.push_back(v_in_uv_img);

    int uv_pitch = dst_frame.pitch/2;
    if (uv_pitch%256)
    {
        uv_pitch = (uv_pitch/256 + 1)*256;
    }

    cuda::GpuMat out_y_img(dst_frame.height, dst_frame.width, CV_8UC1, dst_frame.frame.pPitch[0], dst_frame.pitch);
    cuda::GpuMat out_uv(dst_frame.height/2, dst_frame.width/2, CV_8UC2, dst_frame.frame.pPitch[1], dst_frame.pitch);

    std::vector<cv::_InputOutputArray> v_out_img;
    v_out_img.push_back(out_y_img);
    v_out_img.push_back(out_uv);

    return do_blender(vv_in_img, v_out_img, correlation);
}

int32_t ins_blender::blend_yuv420(std::vector<CUeglFrame>& v_src_frame, CUeglFrame& dst_frame, bool correlation)
{
    std::vector<cuda::GpuMat> v_in_y_img;
    std::vector<cuda::GpuMat> v_in_u_img;
    std::vector<cuda::GpuMat> v_in_v_img;
    for (unsigned i = 0; i < v_src_frame.size(); i++)
    {
        cuda::GpuMat y_img(v_src_frame[i].height, v_src_frame[i].width, CV_8UC1, v_src_frame[i].frame.pPitch[0], v_src_frame[i].pitch);
        v_in_y_img.push_back(y_img);

        cuda::GpuMat u_img(v_src_frame[i].height/2, v_src_frame[i].width/2, CV_8UC1, v_src_frame[i].frame.pPitch[1], v_src_frame[i].pitch/2);
        v_in_u_img.push_back(u_img);

        cuda::GpuMat v_img(v_src_frame[i].height/2, v_src_frame[i].width/2, CV_8UC1, v_src_frame[i].frame.pPitch[2], v_src_frame[i].pitch/2);
        v_in_v_img.push_back(v_img);
    }

    std::vector<cv::_InputArray> vv_in_img;
    vv_in_img.push_back(v_in_y_img);
    vv_in_img.push_back(v_in_u_img);
    vv_in_img.push_back(v_in_v_img);

    int uv_pitch = dst_frame.pitch/2;
    if (uv_pitch%256)
    {
        uv_pitch = (uv_pitch/256 + 1)*256;
    }

    cuda::GpuMat out_y_img(dst_frame.height, dst_frame.width, CV_8UC1, dst_frame.frame.pPitch[0], dst_frame.pitch);
    cuda::GpuMat out_u_img(dst_frame.height/2, dst_frame.width/2, CV_8UC1, dst_frame.frame.pPitch[1], uv_pitch);
    cuda::GpuMat out_v_img(dst_frame.height/2, dst_frame.width/2, CV_8UC1, dst_frame.frame.pPitch[2], uv_pitch);

    std::vector<cv::_InputOutputArray> v_out_img;
    v_out_img.push_back(out_y_img);
    v_out_img.push_back(out_u_img);
    v_out_img.push_back(out_v_img);

    return do_blender(vv_in_img, v_out_img, correlation);
}

int32_t ins_blender::blend(const std::vector<ins_img_frame>& v_in, ins_img_frame& out)
{
    int32_t data_type, data_size;
    if (blend_fmt_ == BLENDERTYPE::BGR_CUDA)
    {
        data_type = CV_8UC3;
        data_size = 3;
    }
    else if (blend_fmt_ == BLENDERTYPE::RAW_CUDA)
    {
        data_type = CV_16UC1;
        data_size = 2;
    }
    else
    {
        assert(false);
    }

    std::vector<cuda::GpuMat> v_in_img;
    for (uint32_t i = 0; i < v_in.size(); i++)
    {
        Mat cpu_img(v_in[i].h, v_in[i].w, data_type, v_in[i].buff->data()); 
        cuda::GpuMat gpu_img(cpu_img);
        v_in_img.push_back(gpu_img);
    }

    std::vector<cv::_InputArray> vv_in_img;
    vv_in_img.push_back(v_in_img);

    cv::cuda::GpuMat out_img(out.h, out.w, data_type);
    std::vector<cv::_InputOutputArray> v_out_img;
    v_out_img.push_back(out_img);

    auto ret = do_blender(vv_in_img, v_out_img, false);
    RETURN_IF_NOT_OK(ret);

    Mat out_cpu_img(out_img);
    out.buff = std::make_shared<insbuff>(out.h*out.w*data_size);
    memcpy(out.buff->data(), out_cpu_img.data, out.buff->size());

    return INS_OK;
}

int32_t ins_blender::do_blender(const std::vector<cv::_InputArray>& in, std::vector<cv::_InputOutputArray>& out, bool correlation)
{
    int32_t ret;
    ins_clock clock;

    if (blend_type_ == INS_BLEND_TYPE_TEMPLATE)
    {
        ret = ((TemplateBlender*)handle_)->blendImage(in, out);
    }
    else
    {
        if (correlation)
        {
            ret = ((SeamlessBlender*)handle_)->blendFrame(in, out);
        }
        else
        {
            ret = ((SeamlessBlender*)handle_)->blendImage(in, out);
        }
    }

    if (ret < 0)
    {
        LOGERR("blend fail:%d", ret);
        return INS_ERR; 
    }
    else
    {
        //LOGINFO("blend time:%lf", clock.elapse());
        return INS_OK;
    }
}
