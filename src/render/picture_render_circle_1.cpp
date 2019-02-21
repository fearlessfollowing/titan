
#include "picture_render.h"
#include "inslog.h"
#include "common.h"

int picture_render::setup(int mode, int i_width, int i_height, int o_width, int o_height)
{
	LOGINFO("picture render setup in width:%d in height:%d out width:%d out height:%d mode:%d", i_width, i_height, o_width, o_height, mode);

	out_width_ = o_width;
	out_height_ = o_height;
	render_init(i_width, i_height, mode, false);

	if (INS_OK != egl_.setup_pbuff(0, out_width_, out_height_)) return INS_ERR;

	if (INS_OK != egl_.make_current(0)) return INS_ERR;

	if (INS_OK != setup_gles()) return INS_ERR;

	pix_buff_ = std::make_shared<insbuff>(out_width_*out_height_*4);

	return INS_OK;
}

int picture_render::setup_gles()
{
	if (INS_OK != create_master_program()) return INS_ERR;

	gen_texture();

	glUseProgram(program_[0]);
	glDisable(GL_DEPTH_TEST);
	prepare_vao();

	return INS_OK;
}

const unsigned char* picture_render::draw(const std::vector<const unsigned char*>& image)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// glViewport(0, 0, out_width_, out_height_);

	draw_master_vao(0, 0, out_width_, out_height_, image);
	
	glBindVertexArrayOES(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFlush();
	glFinish();

	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(0, 0, out_width_, out_height_, GL_RGBA, GL_UNSIGNED_BYTE, pix_buff_->data());
	egl_.swap_buffers();

	return pix_buff_->data();
}
