
#include "picture_render.h"
#include "inslog.h"
#include "common.h"

int picture_render::setup(int o_w, int o_h)
{
	o_w_ = o_w;
	o_h_ = o_h;

	set_input_texture_type(false);

	int ret;
	ret = egl_.setup_pbuff(0, o_w_, o_h_);
	RETURN_IF_NOT_OK(ret);

	ret = egl_.make_current(0);
	RETURN_IF_NOT_OK(ret);

	ret = create_master_program();
	RETURN_IF_NOT_OK(ret);

	gen_texture();

	glUseProgram(program_[0]);
	glDisable(GL_DEPTH_TEST);
	prepare_vao();

	pix_buff_ = std::make_shared<insbuff>(o_w_*o_h_*4);

	LOGINFO("picture render in w:%d h:%d out w:%d h:%d mode:%d map:%d", in_width_, in_height_, o_w, o_h, mode_, map_type_);

	return INS_OK;
}

const unsigned char* picture_render::draw(const std::vector<const unsigned char*>& image, const float* mat)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	draw_master_vao(o_w_, o_h_, mat, image);
	glFlush();
	glFinish();
	egl_.swap_buffers();

	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(0, 0, o_w_, o_h_, GL_RGBA, GL_UNSIGNED_BYTE, pix_buff_->data());

	return pix_buff_->data();
}
