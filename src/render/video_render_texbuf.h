
#ifndef __VIDEO_RENDER_TEXBUF_H__
#define __VIDEO_RENDER_TEXBUF_H__

#include "render.h"
#include "ins_frame.h"

class video_render_texbuf : public render
{
public:
	virtual ~video_render_texbuf() override { release(); };
	int setup(int o_w, int o_h);
	std::shared_ptr<ins_frame> draw(long long pts, const float* mat);

private:
	void gen_frame_buff();
	int prepare_rgba_to_nv12_vao(GLuint program, GLuint& vao, GLuint* vbo);
	void release();
	int o_w_ = 0;
	int o_h_ = 0;
	GLuint frame_buff_ = 0;
	GLuint frame_buff_tex_ = 0;
	GLuint vao_y_ = 0;
	GLuint vbo_y_[2] = {0};
	GLuint vao_uv_ = 0;
	GLuint vbo_uv_[2] = {0};
};

#endif