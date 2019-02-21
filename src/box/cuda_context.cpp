
#include "cuda_context.h"
#include "inslog.h"
#include "common.h"
#include <cassert>

void cuda_context::release()
{
    if (context_)
    {
        cuCtxDestroy(context_);
        context_ = nullptr;
    }

    if (display_)
    {
        eglTerminate(display_);
        display_ = nullptr;
    }

    LOGINFO("cuda context release");
}

int cuda_context::setup()
{
    cuInit(0);

    cuDeviceGet(&device_, 0);
    if (device_ == -1)
    {
        LOGERR("cuDeviceGet fail");
        return INS_ERR;
    }

    cuCtxCreate(&context_, CU_CTX_MAP_HOST, device_);
    if (context_ == nullptr)
    {
        LOGERR("cuCtxCreate fail");
        return INS_ERR;
    }

    cuCtxSetCurrent(context_);

    int drv_version = -1;
    cuDriverGetVersion(&drv_version);
    LOGINFO("cuda driver version:%d", drv_version);

    // char name[256] = "";
    // cuDeviceGetName(name, sizeof(name), device_);
    // LOGINFO("cuda device name %s", name);

    // size_t size = 0;
    // cuDeviceTotalMem(&size, device_);
    // LOGINFO("cuda device total memory: %u bytes", size);

    unsigned int apiVersion = -1;
    cuCtxGetApiVersion(context_, &apiVersion);
    LOGINFO("cuda API verison: %u", apiVersion);

    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if ((display_ == EGL_NO_DISPLAY) || (eglGetError() != EGL_SUCCESS))
	{
		LOGERR("eglGetDisplay fail");
		return INS_ERR;
	}

	int ret = eglInitialize(display_, nullptr, nullptr);
	if ((ret == EGL_FALSE) || (eglGetError() != EGL_SUCCESS))
	{
		LOGERR("eglInitialize fail");
		return INS_ERR;
	}

    pfnEglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    pfnEglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");

    assert(pfnEglCreateImageKHR);
    assert(pfnEglDestroyImageKHR);

    return INS_OK;
}