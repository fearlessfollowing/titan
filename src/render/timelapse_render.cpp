
#include "timelapse_render.h"
#include "inslog.h"
#include "common.h"

int timelapse_render::setup(int o_w, int o_h, EGLNativeWindowType native_wnd)
{
	o_w_ = o_w;
	o_h_ = o_h;

	set_input_texture_type(false);

	int ret;
	ret = egl_.setup_window(0, native_wnd);
	RETURN_IF_NOT_OK(ret);

	ret = egl_.make_current(0);
	RETURN_IF_NOT_OK(ret);

	ret = create_master_program();
	RETURN_IF_NOT_OK(ret);

	gen_texture();

	glDisable(GL_DEPTH_TEST);
	glUseProgram(program_[0]);
	prepare_vao();

	LOGINFO("timelapse render in w:%d h:%d out w:%d h:%d mode:%d map:%d", in_width_, in_height_, o_w, o_h, mode_, map_type_);

	return INS_OK;
}

void timelapse_render::draw(long long pts, const std::vector<const unsigned char*>& image, const float* mat)
{	
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_master_vao(o_w_, o_h_, mat, image); 
    glFlush();
    glFinish();
    egl_.presentation_time(pts);//nsecs
    egl_.swap_buffers();
}
