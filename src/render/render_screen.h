
#ifndef __RENDER_SCEEN_H__
#define __RENDER_SCEEN_H__

#include "render_base.h"

class render_sceen : public render_base
{
public:
	virtual ~render_sceen() override 
	{ 
		release_render(); 
	};

	int32_t setup();
	void draw(void* eglimg);

private:
	static const int32_t vao_num_ = 1;
	static const int32_t vbo_num_ = 2;

	void release_render();
	void prepare_vao(GLuint program);

	GLuint vbo_[vbo_num_] = {0};
	GLuint vao_[vao_num_] = {0};
	int32_t screen_width_ = 0;
	int32_t screen_height_ = 0;
	GLuint texture_ = 0;
};

#endif