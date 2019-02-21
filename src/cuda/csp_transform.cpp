#include "csp_transform.h"
#include "csp_transform_cu.h"
#include "ins_clock.h"
#include "common.h"
#include "inslog.h"

uint32_t csp_transform::transform_scaling(EGLImageKHR in_img, std::vector<EGLImageKHR> v_out_img, std::vector<bool>& v_half)
{
    //ins_clock t1;

    CUresult status;
    cudaFree(0);

    CUgraphicsResource in_resource = nullptr;
    status = cuGraphicsEGLRegisterImage(&in_resource, in_img, CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
    if (status != CUDA_SUCCESS)
    {
        LOGERR("in cuGraphicsEGLRegisterImage failed: %d",status);
        return INS_ERR;
    }

    CUeglFrame in_eglFrame;
    status = cuGraphicsResourceGetMappedEglFrame(&in_eglFrame, in_resource, 0, 0);
    if (status != CUDA_SUCCESS) 
    {
        LOGERR("in cuGraphicsSubResourceGetMappedArray failed:%d", status);
        return INS_ERR;
    }

    std::vector<CUgraphicsResource> v_out_resource;
    std::vector<CUeglFrame> v_out_eglframe;
    for (uint32_t i = 0; i < v_out_img.size(); i++)
    {
        CUgraphicsResource out_resource = nullptr;
        status = cuGraphicsEGLRegisterImage(&out_resource, v_out_img[i], CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("out cuGraphicsEGLRegisterImage failed: %d",status);
            return INS_ERR;
        }
        v_out_resource.push_back(out_resource);

        CUeglFrame out_eglFrame;
        status = cuGraphicsResourceGetMappedEglFrame(&out_eglFrame, out_resource, 0, 0);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("out cuGraphicsSubResourceGetMappedArray failed:%d", status);
            return INS_ERR;
        }
        v_out_eglframe.push_back(out_eglFrame);
    }

    status = cuCtxSynchronize();
    if (status != CUDA_SUCCESS)
    {
        LOGERR("cuCtxSynchronize failed:%d", status); 
        return INS_ERR;
    }

    // printf("src plane count:%d pitch:%d w:%d h:%d frameType:%d eglColorFormat:%d\n", 
    //     src_eglFrame.planeCount, 
    //     src_eglFrame.pitch, 
    //     src_eglFrame.width, 
    //     src_eglFrame.height, 
    //     src_eglFrame.frameType, 
    //     src_eglFrame.eglColorFormat);

    transform_scaling_cu(in_eglFrame, v_out_eglframe, v_half); 

    status = cuCtxSynchronize();  
    if (status != CUDA_SUCCESS)
    { 
        LOGERR("cuCtxSynchronize failed");
        return INS_ERR;
    }

    status = cuGraphicsUnregisterResource(in_resource);
    if (status != CUDA_SUCCESS)
    {
        LOGERR("in cuGraphicsEGLUnRegisterResource failed: %d", status);
    }

    for (uint32_t i = 0; i < v_out_resource.size(); i++)
    {
        status = cuGraphicsUnregisterResource(v_out_resource[i]);
        if (status != CUDA_SUCCESS)
        {
            LOGERR("out cuGraphicsEGLUnRegisterResource failed: %d", status);
        }
    }
 
    // printf("total:%lf\n", t1.elapse());

    return INS_OK;
}

// uint32_t csp_transform::rgba_to_yuv420(EGLImageKHR in_img, EGLImageKHR out_img)
// {
//     CUresult status;

//     //ins_clock t1;

//     cudaFree(0);

//     CUgraphicsResource out_resource = nullptr;
//     status = cuGraphicsEGLRegisterImage(&out_resource, out_img, CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
//     if (status != CUDA_SUCCESS)
//     {
//         LOGERR("out cuGraphicsEGLRegisterImage failed: %d",status);
//         return INS_ERR;
//     }

//     CUeglFrame out_eglFrame;
//     status = cuGraphicsResourceGetMappedEglFrame(&out_eglFrame, out_resource, 0, 0);
//     if (status != CUDA_SUCCESS)
//     {
//         LOGERR("out cuGraphicsSubResourceGetMappedArray failed:%d", status);
//         return INS_ERR;
//     }

//     CUgraphicsResource in_resource = nullptr;
//     status = cuGraphicsEGLRegisterImage(&in_resource, in_img, CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE);
//     if (status != CUDA_SUCCESS)
//     {
//         LOGERR("in cuGraphicsEGLRegisterImage failed: %d",status);
//         return INS_ERR;
//     }

//     CUeglFrame in_eglFrame;
//     status = cuGraphicsResourceGetMappedEglFrame(&in_eglFrame, in_resource, 0, 0);
//     if (status != CUDA_SUCCESS)
//     {
//         LOGERR("in cuGraphicsSubResourceGetMappedArray failed:%d", status);
//         return INS_ERR;
//     }

//     status = cuCtxSynchronize();
//     if (status != CUDA_SUCCESS)
//     {
//         LOGERR("cuCtxSynchronize failed:%d", status); 
//         return INS_ERR;
//     }

//     // printf("src plane count:%d pitch:%d w:%d h:%d frameType:%d eglColorFormat:%d\n", src_eglFrame.planeCount, src_eglFrame.pitch, src_eglFrame.width, src_eglFrame.height, src_eglFrame.frameType, src_eglFrame.eglColorFormat);
//     // printf("dst plane count:%d pitch:%d w:%d h:%d frameType:%d eglColorFormat:%d\n", dst_eglFrame.planeCount, dst_eglFrame.pitch, dst_eglFrame.width, dst_eglFrame.height, dst_eglFrame.frameType, dst_eglFrame.eglColorFormat);

//     //ins_clock t;
//     rgba_to_yuv420_cu((cudaArray_t)in_eglFrame.frame.pArray[0], out_eglFrame.frame.pPitch, out_eglFrame.pitch);
//     //printf("csp transform time:%lf\n",t.elapse());  

//     status = cuCtxSynchronize();  
//     if (status != CUDA_SUCCESS)
//     { 
//         LOGERR("cuCtxSynchronize failed after memcpy\n"); 
//         return INS_ERR;
//     }

//     status = cuGraphicsUnregisterResource(in_resource);
//     if (status != CUDA_SUCCESS)
//     {
//         LOGERR("in cuGraphicsEGLUnRegisterResource failed: %d", status);
//     }

//     status = cuGraphicsUnregisterResource(out_resource);
//     if (status != CUDA_SUCCESS)
//     {
//         LOGERR("out cuGraphicsEGLUnRegisterResource failed: %d", status);
//     }
 
//     //printf("total:%lf\n", t1.elapse());

//     return INS_OK;
// }




