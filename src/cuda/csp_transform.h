#ifndef _CSP_TRANSFORM_H_
#define _CSP_TRANSFORM_H_

#include "cudaEGL.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include <vector>

class csp_transform
{
public:
    static uint32_t transform_scaling(EGLImageKHR in_img, std::vector<EGLImageKHR> v_out_img, std::vector<bool>& v_half);
};

#endif