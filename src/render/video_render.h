
#ifndef __VIDEO_RENDER_H__
#define __VIDEO_RENDER_H__

#include "render.h"
#include <map>

#define TARGET_INDEX_SCN 0xffff

struct target_ctx;

struct render_target
{
    int index = 0;
    int width = 0;
    int height = 0;
    EGLNativeWindowType native_wnd = nullptr;
};

class video_render : public render
{
public:
	virtual ~video_render() { release(); };
	int setup(render_target& target);
	void draw(long long pts, float* mat);
	int add_render_target(render_target& target);
	void delete_render_target(int index);

private:
	void release();
	int setup_egl(render_target& target);
	int setup_gles();
	void do_add_render_target(render_target& target);
	std::map<int, std::shared_ptr<target_ctx>, std::greater<int>> target_;
};

#endif