
#include "video_render_texbuf.h"
#include "inslog.h"
#include "common.h"
#include "rgbyuv_shader.h"

void video_render_texbuf::release()
{
	if (frame_buff_)
	{
		glDeleteFramebuffers(1, &frame_buff_);
		frame_buff_ = 0;
	}

	if (frame_buff_tex_)
	{
		glDeleteTextures(1, &frame_buff_tex_);
		frame_buff_tex_ = 0;
	}

	if (vao_y_)
	{
		glDeleteVertexArraysOES(1, &vao_y_);
		vao_y_ = 0;
	}

	if (vbo_y_[0])
	{
		glDeleteBuffers(2, vbo_y_);
		vbo_y_[0] = 0;
	}

	if (vao_uv_)
	{
		glDeleteVertexArraysOES(1, &vao_uv_);
		vao_uv_ = 0;
	}

	if (vbo_uv_[0])
	{
		glDeleteBuffers(2, vbo_uv_);
		vbo_uv_[0] = 0;
	}
}

int video_render_texbuf::setup(int o_w, int o_h)
{
	o_w_ = o_w;
	o_h_ = o_h;

	int ret;
	ret = egl_.setup_pbuff(0, o_w_/4, o_h_*3/2);
	RETURN_IF_NOT_OK(ret);

	ret = egl_.make_current(0);
	RETURN_IF_NOT_OK(ret);
	
	ret = create_master_program();
	RETURN_IF_NOT_OK(ret);

	gen_texture();

	glUseProgram(program_[0]);
	prepare_vao();

	gen_frame_buff();

	ret = create_program(r_vertex_shader, r_fragment_shade_y);
	RETURN_IF_NOT_OK(ret);

	ret = create_program(r_vertex_shader, r_fragment_shade_uv);
	RETURN_IF_NOT_OK(ret);

	ret = prepare_rgba_to_nv12_vao(program_[1], vao_y_, vbo_y_);
	RETURN_IF_NOT_OK(ret);

	ret = prepare_rgba_to_nv12_vao(program_[2], vao_uv_, vbo_uv_);
	RETURN_IF_NOT_OK(ret);

	glDisable(GL_DEPTH_TEST);

	LOGINFO("video_render_texbuf w:%d h:%d", o_w_, o_h_);

	return INS_OK;
}

void video_render_texbuf::gen_frame_buff()
{
	glGenTextures(1, &frame_buff_tex_);
	glBindTexture(GL_TEXTURE_2D, frame_buff_tex_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, o_w_, o_h_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	
	glGenFramebuffers(1, &frame_buff_);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buff_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame_buff_tex_, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

int video_render_texbuf::prepare_rgba_to_nv12_vao(GLuint program, GLuint& vao, GLuint* vbo)
{
	static GLfloat vertex_data[] = {
        -1.0f, 1.0f, 0.0f, 1.0f, 
        -1.0f, -1.0f, 0.0f, 1.0f, 
        1.0f, 1.0f, 0.0f, 1.0f, 
        1.0f, -1.0f, 0.0f, 1.0f, 
	};

	// static GLfloat uv_data[] = {
	// 	0.0f, 1.0f,
	// 	0.0f, 0.0f,
	// 	1.0f, 1.0f,
	// 	1.0f, 0.0f,
	// };

	//上下颠倒
	static GLfloat uv_data[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};

	glUseProgram(program);

	GLuint loc_position =  glGetAttribLocation(program, "position");
	GLuint loc_tex_coord =  glGetAttribLocation(program, "v_tex_coord");

	glGenVertexArraysOES(1, &vao);
	glGenBuffers(2, vbo);
	glBindVertexArrayOES(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position);
	glVertexAttribPointer(loc_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);	
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_tex_coord);
	glVertexAttribPointer(loc_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glUniform1f(glGetUniformLocation(program, "width"), (float)o_w_);
	glUniform1f(glGetUniformLocation(program, "height"), (float)o_h_);
	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glBindVertexArrayOES(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return INS_OK;
}

std::shared_ptr<ins_frame> video_render_texbuf::draw(long long pts, const float* mat)
{
	//LOGINFO("draw pts:%lld", pts);

	//draw to framebuff:rgba
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buff_);
	glUseProgram(program_[0]);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	draw_master_vao(o_w_, o_h_, mat);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//rgba -> y
	glUseProgram(program_[1]);
	glViewport(0, 0, o_w_/4, o_h_);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_buff_tex_);
	glBindVertexArrayOES(vao_y_);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//rgba -> uv
	glUseProgram(program_[2]);
	glViewport(0, o_h_, o_w_/4, o_h_/2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, frame_buff_tex_);
	glBindVertexArrayOES(vao_uv_);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArrayOES(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFlush();
	glFinish();
	egl_.swap_buffers();

	auto frame = std::make_shared<ins_frame>();
	frame->pts = pts/1000;
	frame->buff = std::make_shared<page_buffer>(o_w_*o_h_*3/2);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadPixels(0, 0, o_w_/4, o_h_*3/2, GL_RGBA, GL_UNSIGNED_BYTE, frame->buff->data());

	return frame;
}
