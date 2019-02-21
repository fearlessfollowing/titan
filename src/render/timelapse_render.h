
#ifndef __TIMELAPSE_RENDER_H__
#define __TIMELAPSE_RENDER_H__

#include "render.h"
#include <map>

class timelapse_render : public render
{
public:
	int setup(int o_w, int o_h, EGLNativeWindowType native_wnd);
	void draw(long long pts, const std::vector<const unsigned char*>& image, const float* mat);

private:
    int o_w_ = 0;
    int o_h_ = 0;
};

#endif