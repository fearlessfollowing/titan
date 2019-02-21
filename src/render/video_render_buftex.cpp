
#include "video_render_buftex.h"
#include "inslog.h"
#include "common.h"

#define TARGET_INDEX_SCN 0xffff

int video_render_buftex::setup(int width, int height, EGLNativeWindowType native_wnd)
{
	o_w_ = width;
	o_h_ = height;

	set_input_texture_type(false);

	int ret;
	#ifdef TARGET_INDEX_SCN //render to hdmi
	ret = egl_.setup_screen(TARGET_INDEX_SCN, width, height);
	RETURN_IF_NOT_OK(ret);
	ret = egl_.make_current(TARGET_INDEX_SCN);
	RETURN_IF_NOT_OK(ret);
	#else //render to encoder
	ret = egl_.setup_window(0, native_wnd);
	ret = egl_.make_current(0);
	RETURN_IF_NOT_OK(ret);
	#endif

	ret = create_master_program();
	RETURN_IF_NOT_OK(ret);

	gen_texture();

	glUseProgram(program_[0]);
	glDisable(GL_DEPTH_TEST);
	prepare_vao();

	LOGINFO("video bufftex render in w:%d h:%d out w:%d h:%d mode:%d map:%d", in_width_, in_height_, o_w_, o_h_, mode_, map_type_);

	return INS_OK;
}

void video_render_buftex::draw(long long pts, const std::vector<const unsigned char*>& image, const float* mat)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	draw_master_vao(o_w_, o_h_, mat, image);
	glFlush();
	glFinish();
	egl_.presentation_time(pts);
	egl_.swap_buffers();
}

// int video_render_buftex::setup(int width, int height, EGLNativeWindowType native_wnd)
// {
// 	o_w_ = width;
// 	o_h_ = height;

// 	set_input_texture_type(false);

// 	int ret;
// 	ret = egl_.setup_pbuff(0, o_w_, o_h_);
// 	RETURN_IF_NOT_OK(ret);

// 	ret = egl_.make_current(0);
// 	RETURN_IF_NOT_OK(ret);

// 	ret = create_master_program();
// 	RETURN_IF_NOT_OK(ret);

// 	gen_texture();

// 	glUseProgram(program_[0]);
// 	glDisable(GL_DEPTH_TEST);
// 	prepare_vao();

// 	pix_buff_ = std::make_shared<insbuff>(o_w_*o_h_*4);

// 	LOGINFO("video_render_buftex in w:%d h:%d", o_w_, o_h_);

// 	return INS_OK;
// }

// void video_render_buftex::draw(long long pts, const std::vector<const unsigned char*>& image, const float* mat)
// {
// 	glClearColor(0, 0, 0, 1);
// 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// 	draw_master_vao(o_w_, o_h_, mat, image);
// 	glFlush();
// 	glFinish();
// 	egl_.swap_buffers();

// 	glPixelStorei(GL_PACK_ALIGNMENT, 4);
// 	glReadPixels(0, 0, o_w_, o_h_, GL_RGBA, GL_UNSIGNED_BYTE, pix_buff_->data());
// }
