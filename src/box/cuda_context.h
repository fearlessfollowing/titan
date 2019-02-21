#ifndef _CUDA_CONTEXT_H_
#define _CUDA_CONTEXT_H_

#include <cuda.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <ui/GraphicBuffer.h>

class cuda_context
{
public:
    ~cuda_context() 
    { 
        release(); 
    }

    int setup();

    void make_current()
    {
        if (context_) cuCtxSetCurrent(context_);
    }

    EGLImageKHR eglCreateImageKHR(android::GraphicBuffer* graphic_buff)
    {
        if (graphic_buff == nullptr || pfnEglCreateImageKHR == nullptr || display_ == nullptr) return EGL_NO_IMAGE_KHR;

        return pfnEglCreateImageKHR(display_, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, graphic_buff->getNativeBuffer(), nullptr);
    }

    void eglDestroyImageKHR(EGLImageKHR img)
    {
        if (img == EGL_NO_IMAGE_KHR || pfnEglDestroyImageKHR == nullptr || display_ == nullptr) return;

        pfnEglDestroyImageKHR(display_, img);
    }

private:
    void release();
    CUdevice device_ = -1;
    CUcontext context_ = nullptr;
    EGLDisplay display_ = nullptr;
    PFNEGLCREATEIMAGEKHRPROC pfnEglCreateImageKHR = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC pfnEglDestroyImageKHR = nullptr;
};

#endif