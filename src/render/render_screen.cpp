
#include "inslog.h"
#include "common.h"
#include "shader.h"
#include "render_screen.h"
 
void render_sceen::release_render()
{   
	if (vao_[0] != 0)
	{
		glDeleteVertexArrays(vao_num_, vao_);
		vao_[0] = 0;
	}

	if (vbo_[0] != 0)
	{
		glDeleteBuffers(vbo_num_, vbo_);
		vbo_[0] = 0;
	}
	
	if (texture_)
	{
		glDeleteTextures(1, &texture_);
		texture_ = 0;
	}
}

int32_t render_sceen::setup()
{
	auto ret = egl_.setup_window(0, screen_width_, screen_height_);
	RETURN_IF_NOT_OK(ret);

	ret = create_program(vertex_shader_quad, fragment_shader_quad_external);
	RETURN_IF_NOT_OK(ret);

	glGenTextures(1, &texture_);

	glUseProgram(program_[0]);
	glDisable(GL_DEPTH_TEST);

	prepare_vao(program_[0]);

	return INS_OK;
}

void render_sceen::prepare_vao(GLuint program)
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

	static GLfloat uv_data_3d[] = {
		0.0f, 0.5f,
		0.0f, 0.0f,
		1.0f, 0.5f,
		1.0f, 0.0f,
	};

	GLuint loc_position1 =  glGetAttribLocation(program, "position");
	GLuint loc_tex_coord =  glGetAttribLocation(program, "vertTexCoord");

	glGenVertexArrays(vao_num_, vao_);
	glGenBuffers(vbo_num_, vbo_);
	glBindVertexArray(vao_[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position1);
	glVertexAttribPointer(loc_position1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);	
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_tex_coord);
	glVertexAttribPointer(loc_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_);
	glUniform1i(glGetUniformLocation(program_[0], "tex"), 0);
}

void render_sceen::draw(void* eglimg)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//glViewport(0, 0, screen_width_, screen_height_);
	//glUseProgram(program_[0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg);
	glBindVertexArray(vao_[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glFlush();
	glFinish();
	egl_.swap_buffers();
}


