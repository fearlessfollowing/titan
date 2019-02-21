
#include "insegl.h"
#include "inslog.h"
#include "common.h"
#include "ins_util.h"
#include "insx11.h"

static const EGLint context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

int32_t insegl::setup_pbuff(int32_t index, int32_t width, int32_t height)
{
    static int32_t pbuff_attr[] = 
    {
        EGL_WIDTH, 0,
        EGL_HEIGHT, 0,
        EGL_NONE
    };

    int32_t ret;
    pbuff_attr[1] = width;
    pbuff_attr[3] = height;

    ret = setup_egl(INS_SURFACE_PBUFF);
    if (INS_OK != ret) return ret;

    surface_ = eglCreatePbufferSurface(display_, config_, pbuff_attr);
    if ((surface_ == EGL_NO_SURFACE) || (eglGetError() != EGL_SUCCESS))
    {
        LOGERR("eglCreatePbufferSurface fail");
        return INS_ERR;
    }

    EGLBoolean ret1 = eglMakeCurrent(display_, surface_, surface_, context_);
    if ((ret1 == EGL_FALSE) || (eglGetError() != EGL_SUCCESS))
    {
        LOGERR("eglMakeCurrent fail");
        return INS_ERR;
    }

    // const GLubyte* vendor = glGetString(GL_VENDOR);
    // printf("GL vendor: %s\n", vendor);
    // const GLubyte* renderer = glGetString(GL_RENDERER);
    // printf("GL renderer: %s\n", renderer);
    // const GLubyte* version = glGetString(GL_VERSION);
    // printf("GL version: %s\n", version);
    // const GLubyte* slVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    // printf("GLSL version: %s\n", slVersion);

    return INS_OK;
}

int32_t insegl::setup_window(int32_t index, int32_t& width, int32_t& height)
{   
    x11_ = std::make_shared<insx11>();
    auto x_window = x11_->create_x_window(width, height);
    if (x_window == 0)
    {
        LOGERR("create x window fail");
        return INS_ERR;
    }
    
    auto ret = setup_egl(INS_SURFACE_WINDOW);
    RETURN_IF_NOT_OK(ret);

    surface_ = eglCreateWindowSurface(display_, config_, (EGLNativeWindowType)x_window, nullptr);
    if ((surface_ == EGL_NO_SURFACE) || (eglGetError() != EGL_SUCCESS))
    {
    	LOGERR("eglCreateWindowSurface fail");
    	return INS_ERR;
    }

    EGLBoolean ret1 = eglMakeCurrent(display_, surface_, surface_, context_);
    if ((ret1 == EGL_FALSE) || (eglGetError() != EGL_SUCCESS))
    {
        LOGERR("eglMakeCurrent fail");
        return INS_ERR;
    }

    return INS_OK;
}

int32_t insegl::setup_egl(int32_t type)
{
    //EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
    static const EGLint config_attr_window[] = 
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    static const EGLint config_attr_pbuff[] = 
    {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    
    if (type == INS_SURFACE_WINDOW)
    {
        display_ = eglGetDisplay(x11_->get_x_display());
    }
    else
    {
        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if ((display_ == EGL_NO_DISPLAY) || (eglGetError() != EGL_SUCCESS))
    {
        LOGERR("eglGetDisplay fail");
        return INS_ERR;
    }

    EGLBoolean ret = eglInitialize(display_, nullptr, nullptr);
    if ((ret == EGL_FALSE) || (eglGetError() != EGL_SUCCESS))
    {
        LOGERR("eglInitialize fail:0x%x", eglGetError());
        return INS_ERR;
    }
    
    //eglBindAPI(EGL_OPENGL_ES_API);

    EGLint num_config;
    ret = eglChooseConfig(display_, (type == INS_SURFACE_WINDOW) ? config_attr_window : config_attr_pbuff, &config_, 1, &num_config);
    if ((ret  == EGL_FALSE) || (num_config < 1))
    {
        LOGERR("eglChooseConfig fail");
        return INS_ERR;
    }
    
    context_ = eglCreateContext(display_, config_ , EGL_NO_CONTEXT, context_attr);
    if ((context_ == EGL_NO_CONTEXT) || (eglGetError() != EGL_SUCCESS))
    {
        LOGERR("eglCreateContext fail");
        return INS_ERR;
    }

    eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	if (eglCreateImageKHR == nullptr)
	{
		LOGERR("eglGetProcAddress eglCreateImageKHR fail");
        return INS_ERR;
	}

    eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    if (eglDestroyImageKHR == nullptr)
	{
		LOGERR("eglGetProcAddress eglDestroyImageKHR fail");
        return INS_ERR;
	}

    glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    if (glEGLImageTargetTexture2DOES == nullptr)
	{
		LOGERR("eglGetProcAddress glEGLImageTargetTexture2DOES fail");
        return INS_ERR;
	}

    eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)eglGetProcAddress("eglCreateSyncKHR");
    if (eglCreateSyncKHR == nullptr)
	{
		LOGERR("eglGetProcAddress eglCreateSyncKHR fail");
        return INS_ERR;
	}

    eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC)eglGetProcAddress("eglDestroySyncKHR");
    if (eglDestroySyncKHR == nullptr)
	{
		LOGERR("eglGetProcAddress eglDestroySyncKHR fail");
        return INS_ERR;
	}

    eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC)eglGetProcAddress("eglClientWaitSyncKHR");
    if (eglClientWaitSyncKHR == nullptr)
	{
		LOGERR("eglGetProcAddress eglClientWaitSyncKHR fail");
        return INS_ERR;
	}

    type_ = type;

    return INS_OK;
}

void insegl::swap_buffers()
{
    eglSwapBuffers(display_, surface_);
}

void insegl::release()
{   
    if (display_ == EGL_NO_DISPLAY) return;

    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (surface_ != EGL_NO_SURFACE)
    {
        eglDestroySurface(display_, surface_);
        surface_ = EGL_NO_SURFACE;
    }

    if (context_ != EGL_NO_CONTEXT) 
    {
        eglDestroyContext(display_, context_);
        context_ = EGL_NO_CONTEXT;
    }

    config_ = nullptr;

    eglReleaseThread();

    eglTerminate(display_);

    display_ = EGL_NO_DISPLAY;

    if (x11_) x11_->release_x_window();
}