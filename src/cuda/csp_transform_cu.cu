
#include "csp_transform_cu.h"
#include <stdio.h>

__global__ void rgba_to_yuv420_kernel(cudaTextureObject_t tex_obj, char* y, char* u, char* v, int w, int h, int y_pitch, int uv_pitch)
{
    int vetex_y = blockIdx.y * blockDim.y + threadIdx.y;
    int vetex_x = blockIdx.x * blockDim.x + threadIdx.x;
    if (vetex_x >= w || vetex_y >= h) { return; }

    float tex_x = (vetex_x + 0.5f) / w;
    float tex_y = 1.0f - (vetex_y + 0.5f) / h;
    float4 color = tex2D<float4>(tex_obj, tex_x, tex_y);

    int i = vetex_y*y_pitch + vetex_x;
    //y[i] = (0.1826f*color.x + 0.6142f*color.y + 0.0620f*color.z)*256 + 16;
    //y[i] = (306*color.x + 601*color.y + 117*color.z)*255.0f/1024.0f;
    y[i] = 66*color.x + 129*color.y + 25*color.z + 16; //BT601 limited range
    //y[i] = 77*color.x + 150*color.y + 29*color.z; //BT601 full range
    if ((vetex_x%2 == 0) && (vetex_y%2 == 0))
    {
        int j = vetex_y/2*uv_pitch+vetex_x/2;
        //u[j] = (-0.1006f*color.x - 0.3386f*color.y + 0.4392f*color.z)*256 + 128;
        //v[j] = (0.4392f*color.x - 0.3989f*color.y - 0.0403f*color.z)*256 + 128;
        //u[j] = (-173*color.x - 339*color.y + 512*color.z)*255.0f/1024.0f + 128;
        //v[j] = (512*color.x - 429*color.y - 83*color.z)*255.0f/1024.0f + 128;
        u[j] = (-38*color.x - 74*color.y + 112*color.z) + 128;
        v[j] = (112*color.x - 94*color.y - 18*color.z) + 128;
        //u[j] = (-43*color.x - 84*color.y + 127*color.z) + 128;
        //v[j] = (127*color.x - 106*color.y - 21*color.z) + 128;
    }
}

int transform_scaling_cu(const CUeglFrame& in_eglframe, const std::vector<CUeglFrame>& v_out_eglframe, std::vector<bool>& v_half)
{
     cudaError_t ret;

     auto in_array = (cudaArray_t)in_eglframe.frame.pArray[0];

    //get array info
    struct cudaChannelFormatDesc desc;
    struct cudaExtent extent;
    unsigned int flags;
    ret = cudaArrayGetInfo(&desc, &extent, &flags, in_array);
    if (ret != cudaSuccess) 
    {
        printf("cudaArrayGetInfo fail:%d\n", ret);
        return -1;
    }

    //int w = extent.width;
    //int h = extent.height;
    //int num_channel = (desc.x + desc.y + desc.z + desc.w) / 8;
    //printf("array w:%d h:%d pitch:%d\n", w, h, y_pitch);

    //create texture obj
    struct cudaResourceDesc res_desc;
    memset(&res_desc, 0, sizeof(res_desc));
    res_desc.resType = cudaResourceTypeArray;
    res_desc.res.array.array = in_array;

    struct cudaTextureDesc tex_desc;
    memset(&tex_desc, 0, sizeof(tex_desc));
    tex_desc.filterMode = cudaFilterModeLinear;
    tex_desc.readMode = cudaReadModeNormalizedFloat;
    tex_desc.normalizedCoords = 1;

    cudaTextureObject_t tex_obj;
    ret = cudaCreateTextureObject(&tex_obj, &res_desc, &tex_desc, NULL);
    if (ret != cudaSuccess) 
    {
        printf("cudaCreateTextureObject fail:%d\n", ret);
        return -1;
    }
    
    for (uint32_t i = 0; i < v_out_eglframe.size(); i++)
    {
        int uv_pitch = v_out_eglframe[i].pitch/2;
        if (uv_pitch%256)
        {
            uv_pitch = (uv_pitch/256 + 1)*256;
        }

        int blocks_w = ceil(v_out_eglframe[i].width/32.0);
        int blocks_h = ceil(v_out_eglframe[i].height/16.0);
        
        //每个block的thread数目不要超过1024个
        dim3 thread_per_block(32, 16);
        dim3 blocks(blocks_w,blocks_h);
        rgba_to_yuv420_kernel<<<blocks,thread_per_block>>>(
                        tex_obj, 
                        (char*)v_out_eglframe[i].frame.pPitch[0],
                        (char*)v_out_eglframe[i].frame.pPitch[1],
                        (char*)v_out_eglframe[i].frame.pPitch[2], 
                        v_out_eglframe[i].width, 
                        v_half[i]?v_out_eglframe[i].height*2:v_out_eglframe[i].height, 
                        v_out_eglframe[i].pitch, uv_pitch);
    }

    #if 0
    if (v_out_eglframe.size() >= 2) //默认第二路为预览流了
    {
        uv_pitch = v_out_eglframe[1].pitch/2;
        if (uv_pitch%256)
        {
            uv_pitch = (uv_pitch/256 + 1)*256;
        }
        
        dim3 thread_per_block(32, 16);
        dim3 blocks(v_out_eglframe[1].width/32,v_out_eglframe[1].height/16);
        rgba_to_yuv420_kernel<<<blocks,thread_per_block>>>(
                        tex_obj, 
                        (char*)v_out_eglframe[1].frame.pPitch[0],
                        (char*)v_out_eglframe[1].frame.pPitch[1],
                        (char*)v_out_eglframe[1].frame.pPitch[2], 
                        v_out_eglframe[1].width, 
                        b_3d?v_out_eglframe[1].height*2:v_out_eglframe[1].height, 
                        v_out_eglframe[1].pitch, uv_pitch);
    }
    #endif

    cudaDestroyTextureObject(tex_obj);

    return 0;
}

#if 0
int rgba_to_yuv420_cu(cudaArray_t in, void** out, int y_pitch)
{
    cudaError_t ret;

    //get array info
    struct cudaChannelFormatDesc desc;
    struct cudaExtent extent;
    unsigned int flags;
    ret = cudaArrayGetInfo(&desc, &extent, &flags, in);
    if (ret != cudaSuccess) 
    {
        printf("cudaArrayGetInfo fail:%d\n", ret);
        return -1;
    }

    int w = extent.width;
    int h = extent.height;
    //int num_channel = (desc.x + desc.y + desc.z + desc.w) / 8;
    //printf("array w:%d h:%d pitch:%d\n", w, h, y_pitch);

    //create texture obj
    struct cudaResourceDesc res_desc;
    memset(&res_desc, 0, sizeof(res_desc));
    res_desc.resType = cudaResourceTypeArray;
    res_desc.res.array.array = in;

    struct cudaTextureDesc tex_desc;
    memset(&tex_desc, 0, sizeof(tex_desc));
    tex_desc.filterMode = cudaFilterModeLinear;
    tex_desc.readMode = cudaReadModeNormalizedFloat;
    tex_desc.normalizedCoords = 1;

    cudaTextureObject_t tex_obj;
    ret = cudaCreateTextureObject(&tex_obj, &res_desc, &tex_desc, NULL);
    if (ret != cudaSuccess) 
    {
        printf("cudaCreateTextureObject fail:%d\n", ret);
        return -1;
    }
    
    int uv_pitch = y_pitch/2;
    if (uv_pitch%256)
    {
        uv_pitch = (uv_pitch/256 + 1)*256;
    }

    dim3 threadsPerBlock(16, 16); //每个block的thread数目不要超过1024个
    dim3 blocks(w/16,h/16);
    rgba_to_yuv420_kernel<<<blocks,threadsPerBlock>>>(tex_obj, (char*)out[0], (char*)out[1], (char*)out[2], w, h, y_pitch, uv_pitch);

    cudaDestroyTextureObject(tex_obj);

    return 0;
}
#endif


