
#include "singlen_render.h"
#include "common.h"
#include "inslog.h"
#include "singlen_shader.h"

void singlen_render::release_render()
{
    if (texture_)
	{
		glDeleteTextures(1, &texture_);
		texture_ = 0;
	}

    if (vao_ != 0)
	{
		glDeleteVertexArrays(1, &vao_);
		vao_ = 0;
	}

	if (vbo_[0] != 0)
	{
		glDeleteBuffers(2, vbo_);
		vbo_[0] = 0;
	}

	if (vao_cross_ != 0)
	{
		glDeleteVertexArrays(1, &vao_cross_);
		vao_cross_ = 0;
	}

	if (vbo_cross_ != 0)
	{
		glDeleteBuffers(1, &vbo_cross_);
		vbo_cross_ = 0;
	}
}

int32_t singlen_render::setup(float x, float y)//EGLNativeWindowType native_wnd
{
    auto ret = egl_.setup_window(0, sceen_w_, sceen_h_);

    ret = setup_gles();
    RETURN_IF_NOT_OK(ret);

    prepare_main_vao(program_[0]);

	prepare_cross_vao(program_[1], x, y);

	return INS_OK;
}

int singlen_render::setup_gles()
{
    int ret = create_program(s_vertex_shader_quad, s_fragment_shader_quad);
	RETURN_IF_NOT_OK(ret);

	ret = create_program(s_vertex_shader_cross, s_fragment_shader_cross);
	RETURN_IF_NOT_OK(ret);

	glGenTextures(1, &texture_);

	glDisable(GL_DEPTH_TEST);
	glUseProgram(program_[0]);

	return INS_OK;
}

void singlen_render::prepare_main_vao(GLuint program)
{
	static GLfloat vertex_data[] = {
        -1.0f, 1.0f, 0.0f, 1.0f, 
        -1.0f, -1.0f, 0.0f, 1.0f, 
        1.0f, 1.0f, 0.0f, 1.0f, 
        1.0f, -1.0f, 0.0f, 1.0f, 
	};

	static GLfloat uv_data[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};

	GLuint loc_position =  glGetAttribLocation(program, "position");
	GLuint loc_tex_coord =  glGetAttribLocation(program, "vertTexCoord");

	glGenVertexArrays(1, &vao_);
	glGenBuffers(2, vbo_);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position);
	glVertexAttribPointer(loc_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);	
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_tex_coord);
	glVertexAttribPointer(loc_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void singlen_render::prepare_cross_vao(GLuint program, float x, float y)
{
	static GLfloat vertex_data_cross[] = {
        0.0f, -1.0f, 0.0f, 1.0f, 
        0.0f, 1.0f, 0.0f, 1.0f, 
        1.0f, 0.0f, 0.0f, 1.0f, 
        -1.0f, 0.0f, 0.0f, 1.0f, 
	};

	vertex_data_cross[0] = x - 0.2;
	vertex_data_cross[1] = y;

	vertex_data_cross[4] = x + 0.2;
	vertex_data_cross[5] = y;

	vertex_data_cross[8] = x;
	vertex_data_cross[9] = y - 0.2;

	vertex_data_cross[12] = x;
	vertex_data_cross[13] = y + 0.2;

	GLuint loc_position =  glGetAttribLocation(program, "position");

	glGenVertexArrays(1, &vao_cross_);
	glGenBuffers(1, &vbo_cross_);
	glBindVertexArray(vao_cross_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cross_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data_cross), vertex_data_cross, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position);
	glVertexAttribPointer(loc_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void singlen_render::draw(EGLImageKHR eglimg)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, sceen_w_, sceen_h_);

	glUseProgram(program_[0]);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg);
	glUniform1i(glGetUniformLocation(program_[0], "tex"), 0);
    glBindVertexArray(vao_);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(program_[1]);
	glBindVertexArray(vao_cross_);
	glDrawArrays(GL_LINES, 0, 4);

	glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    glBindVertexArray(0);

    glFlush();
    glFinish();
    egl_.swap_buffers();
}