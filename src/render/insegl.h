#ifndef __INS_EGL_H__
#define __INS_EGL_H__

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <map>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <memory>

enum 
{
    INS_SURFACE_PBUFF = 0,
    INS_SURFACE_WINDOW,
};

struct surface_context;
class insx11;

//egl只能在同一个线程执行，所以不用考虑多线程安全问题

class insegl
{
public:
    ~insegl()
    {
        release();
    }

    int32_t setup_pbuff(int32_t index, int32_t width, int32_t height);
    int32_t setup_window(int32_t index, int32_t& width, int32_t& height); 
    void release_surface(int32_t index);
    int32_t make_current(int32_t index);
    void swap_buffers();
    EGLDisplay get_display()
    {
        return display_;
    };
    EGLImageKHR create_egl_img(GLuint texture)
    {
        return eglCreateImageKHR(display_, context_, EGL_GL_TEXTURE_2D_KHR, (void*)texture, nullptr);
    }
    void destroy_egl_img(EGLImageKHR eglimg)
    {
        eglDestroyImageKHR(display_, eglimg);
    }
    void ImageTargetTexture2DOES(GLenum type,  EGLImageKHR eglimg)
    {
        glEGLImageTargetTexture2DOES(type, eglimg);
    }
    EGLSyncKHR create_egl_sync() //EGL_NO_SYNC_KHR
    {
        return eglCreateSyncKHR(display_, EGL_SYNC_FENCE_KHR, nullptr);
    }
    void destroy_egl_sync(EGLSyncKHR& sync)
    {
        eglDestroySyncKHR(display_, sync);
    }
    int32_t wait_egl_sync(EGLSyncKHR& sync)
    {
        return eglClientWaitSyncKHR(display_, sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER_KHR);
    }

private:
    int32_t setup_egl(int32_t type);
    void release();

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLConfig config_ = nullptr;
    int32_t type_ = INS_SURFACE_PBUFF;
    EGLSurface surface_;
    std::shared_ptr<insx11> x11_;

    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = nullptr;
    PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR = nullptr;
    PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR = nullptr;
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR = nullptr;
};

#endif

