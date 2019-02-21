
#ifndef __VIDEO_RENDER_BUFTEX_H__
#define __VIDEO_RENDER_BUFTEX_H__

#include "render.h"
#include "insbuff.h"

class video_render_buftex : public render
{
public:
	virtual ~video_render_buftex() override {};
	int setup(int width, int height, EGLNativeWindowType native_wnd);
	void draw(long long pts, const std::vector<const unsigned char*>& image, const float* mat);

private:
	int o_w_ = 0;
	int o_h_ = 0;
	//std::shared_ptr<insbuff> pix_buff_;
};

#endif

