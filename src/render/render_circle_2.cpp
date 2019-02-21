#include "render.h"
#include "inslog.h"
#include "common.h"
#include "Dewarp/Dewarp.h"
#include "xml_config.h"
#include "sharder.h"

#define FLAT_WIDTH   100
#define FLAT_HEIGHT  50

#define CUBE_WIDTH   32
#define CUBE_HEIGHT  32

void render::release_render_cicle()
{   
	if (vao_[0] != 0)
	{
		glDeleteVertexArraysOES(vao_num_, vao_);
		vao_[0] = 0;
	}

	if (vbo_[0] != 0)
	{
		glDeleteBuffers(vbo_num_, vbo_);
		vbo_[0] = 0;
	}

	if (vao_aux_[0] != 0)
	{
		glDeleteVertexArraysOES(vao_aux_num_, vao_aux_);
		vao_aux_[0] = 0;
	}

	if (vbo_aux_[0] != 0)
	{
		glDeleteBuffers(vbo_aux_num_, vbo_aux_);
		vbo_aux_[0] = 0;
	}

	if (texture_[0])
	{
		glDeleteTextures(texture_num_, texture_);
		texture_[0] = 0;
	}

	if (texture_uv_[0])
	{
		glDeleteTextures(texture_num_, texture_uv_);
		texture_uv_[0] = 0;
	}

	if (texture_uv2_[0])
	{
		glDeleteTextures(texture_num_, texture_uv2_);
		texture_uv2_[0] = 0;
	}
}

void render::render_init(int width, int height, int mode, bool b_external)
{
	in_width_ = width;
	in_height_ = height;
	mode_ = mode;
	b_external_ = b_external;
}

void render::gen_texture()
{
	glGenTextures(texture_num_, texture_);

	if (!b_external_)
	{
		for (int i = 0; i < texture_num_; i++)
		{
			glBindTexture(GL_TEXTURE_2D, texture_[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glGenTextures(texture_num_, texture_uv_);
	for (int i = 0; i < texture_num_; i++)
	{
		glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, FLAT_WIDTH, FLAT_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	if (mode_ != INS_MODE_PANORAMA)
	{
		glGenTextures(texture_num_, texture_uv2_);
		for (int i = 0; i < texture_num_; i++)
		{
			glBindTexture(GL_TEXTURE_2D, texture_uv2_[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, FLAT_WIDTH, FLAT_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

const int* render::get_tex_id() const
{
	return (int*)texture_;
}

int render::create_master_program()
{
	if (b_external_)
	{
		return create_program(vertex_shader_highp, fragment_shader_highp_v);
	}
	else
	{
		return create_program(vertex_shader_highp, fragment_shader_highp_p);
	}
	
	//return create_program(vertex_shader, b_external_?fragment_shader_ex:fragment_shader);

	// return create_program(vertex_shader_new, b_external_?fragment_shader_ex_new:fragment_shader);

	//return create_program(vertex_shader_2, b_external_?fragment_shader_ex_2:fragment_shader);

	 //return create_program(vertex_shader_highp, b_external_?fragment_shader_highp:fragment_shader);
}

int render::create_aux_program()
{
	return create_program(vertex_shader_quad, fragment_shader_quad);
}

void render::prepare_vao()
{
	prepare_vao_flat();
}

void render::prepare_vao_flat()
{ 
	int out_width = FLAT_WIDTH;
	int out_height = FLAT_HEIGHT;

	if (mode_ == INS_MODE_PANORAMA)
	{
		std::string offset;
		if (in_width_*9 == in_height_*16)
		{
			offset = xml_config::get_string(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9);
			if (offset == "") offset = INS_DEFAULT_OFFSET_PANO_16_9;
			LOGINFO("pano 16:9 offset:%s", offset.c_str());
		}
		else
		{
			offset = xml_config::get_string(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3);
			if (offset == "") offset = INS_DEFAULT_OFFSET_PANO_4_3;
			LOGINFO("pano 4:3 offset:%s", offset.c_str());
		}
		
		ins::Dewarp map;
		map.setOffset(offset);
		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);
		map.init(v_width, v_height, FLAT_WIDTH, FLAT_HEIGHT);

		for (int i = 0; i < texture_num_; i++)
		{
			float* uv_get = map.getMap(i, ins::MAPTYPE::OPENGL);
			float* alpha_get = map.getAlpha(i, 0, ins::MAPTYPE::OPENGL);
			float* uv = new float[(out_width)*out_height*3];
			for (int j = 0; j< out_width * out_height; j++)
			{
				uv[j*3] = uv_get[j*2];
				uv[j*3+1] = uv_get[j*2+1];
				uv[j*3+2] = alpha_get[j];
			}

			glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, out_width, out_height, 0, GL_RGB, GL_FLOAT, uv);

			// short* uv = new short[(out_width)*out_height*3];
			// for (int j = 0; j< out_width * out_height; j++)
			// {
			// 	uv[j*3] = FloatToFloat16(uv_get[j*2]);
			// 	uv[j*3+1] = FloatToFloat16(uv_get[j*2+1]);
			// 	uv[j*3+2] = FloatToFloat16(alpha_get[j]);
			// }
			
			// glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
			// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_EXT, out_width, out_height, 0, GL_RGB, GL_HALF_FLOAT_OES, uv);
			// unsigned char* uv = new unsigned char[(out_width)*out_height*4];
			// for (int j = 0; j< out_width * out_height; j++)
			// {
			// 	uv[j*4] = uv_get[j*2] * 255;
			// 	uv[j*4+1] = uv_get[j*2+1] * 255;
			// 	uv[j*4+2] = alpha_get[j] * 255;
			// }
			
			// glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
			// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, out_width, out_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, uv);
			
			delete[] uv;
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);

		std::string offset_left = xml_config::get_string(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT);
		if (offset_left == "") offset_left = INS_DEFAULT_OFFSET_3D_LEFT;
		LOGINFO("3d left offset:%s", offset_left.c_str());

		ins::Dewarp map_left;
		map_left.setOffset(offset_left);
		map_left.init(v_width, v_height, FLAT_WIDTH, FLAT_HEIGHT);

		std::string offset_right = xml_config::get_string(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT);
		if (offset_right == "") offset_right = INS_DEFAULT_OFFSET_3D_RIGHT;
		LOGINFO("3d right offset:%s", offset_right.c_str());
		
		ins::Dewarp map_right;
		map_right.setOffset(offset_right);
		map_right.init(v_width, v_height, FLAT_WIDTH, FLAT_HEIGHT);

		for (int i = 0; i < texture_num_; i++)
		{
			float *uv_1_get, *uv_2_get, *alpha_1_get, *alpha_2_get;
			if (submode_ == INS_SUBMODE_3D_TOP_LEFT)
			{
				uv_1_get = map_right.getMap(i, ins::MAPTYPE::OPENGL);
				uv_2_get = map_left.getMap(i, ins::MAPTYPE::OPENGL);
				alpha_1_get = map_right.getAlphaRight(i, ins::MAPTYPE::OPENGL);
				alpha_2_get = map_left.getAlphaLeft(i, ins::MAPTYPE::OPENGL);
			}
			else
			{
				uv_1_get = map_left.getMap(i, ins::MAPTYPE::OPENGL);
				uv_2_get = map_right.getMap(i, ins::MAPTYPE::OPENGL);
				alpha_1_get = map_right.getAlphaRight(i, ins::MAPTYPE::OPENGL);
				alpha_2_get = map_left.getAlphaLeft(i, ins::MAPTYPE::OPENGL);
			}

			float* uv_1 = new float[out_width*out_height*3];
			for (int j = 0; j< out_width * out_height; j++)
			{
				uv_1[j*3] = uv_1_get[j*2];
				uv_1[j*3+1] = uv_1_get[j*2+1];
				uv_1[j*3+2] = alpha_1_get[j];
			}
			glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, out_width, out_height, 0, GL_RGB, GL_FLOAT, uv_1);
			delete[] uv_1;

			float* uv_2 = new float[out_width*out_height*3];
			for (int j = 0; j< out_width * out_height; j++)
			{
				uv_2[j*3] = uv_2_get[j*2];
				uv_2[j*3+1] = uv_2_get[j*2+1];
				uv_2[j*3+2] = alpha_2_get[j];
			}
			glBindTexture(GL_TEXTURE_2D, texture_uv2_[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, out_width, out_height, 0, GL_RGB, GL_FLOAT, uv_2);
			delete[] uv_2;
		}
	}

	int k = 0, n = 0, m=0;
	float* vetex_data = new float[out_width*out_height*4]();
	for (int j = 0; j < out_height; j++)
	{
		for (int i = 0; i < out_width; i++)
		{
			vetex_data[k++] = 2.0 * i / (out_width - 1.0) - 1.0;
			vetex_data[k++] = 1.0 - 2.0 * j / (out_height - 1.0);

			vetex_data[k++] = 0;
			vetex_data[k++] = 1.0; 
		}
	}

	index_num_ = (out_width-out_width/FLAT_WIDTH)*(out_height-out_height/FLAT_HEIGHT)*2*3;
	GLuint* index_data = new GLuint[index_num_]();
	for (int j = 0, k = 0; j < out_height - 1; j++)
	{
		if ((j+1)%FLAT_HEIGHT == 0) continue;

		for (int i = 0; i < out_width - 1; i++)
		{
			GLuint tmp1 = i+j*out_width;
			GLuint tmp2 = i+(j+1)*out_width;
			index_data[k++] = tmp1;
			index_data[k++] = tmp2;
			index_data[k++] = tmp2+1;
			index_data[k++] = tmp1;
			index_data[k++] = tmp1+1;
			index_data[k++] = tmp2+1;
		}
	}

	prepare_master_vao(program_[0], out_width, out_height, vetex_data, index_data);

	if (program_.size() > 1)
	{
		prepare_aux_vao(program_[1]);
	}	

	delete[] vetex_data;
	delete[] index_data;

	LOGINFO("prepare vao fininsh");
}

void render::prepare_master_vao(GLuint program, int width, int height, float* vetex_data, GLuint* index_data)
{
	GLuint loc_position =  glGetAttribLocation(program, "position");

	glGenVertexArraysOES(vao_num_, vao_);
	glGenBuffers(vbo_num_, vbo_);
	glBindVertexArrayOES(vao_[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
	glBufferData(GL_ARRAY_BUFFER, width * height*4*sizeof(GLfloat), vetex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position);
	glVertexAttribPointer(loc_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_num_*sizeof(GLuint), index_data, GL_STATIC_DRAW);

	glBindVertexArrayOES(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint type = b_external_?GL_TEXTURE_EXTERNAL_OES:GL_TEXTURE_2D;
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(type, texture_[0]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_1"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(type, texture_[1]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_2"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(type, texture_[2]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_3"), 2);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(type, texture_[3]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_4"), 3);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(type, texture_[4]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_5"), 4);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(type, texture_[5]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_6"), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[0]);
	glUniform1i(glGetUniformLocation(program_[0], "map_1"), 6);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[1]);
	glUniform1i(glGetUniformLocation(program_[0], "map_2"), 7);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[2]);
	glUniform1i(glGetUniformLocation(program_[0], "map_3"), 8);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[3]);
	glUniform1i(glGetUniformLocation(program_[0], "map_4"), 9);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[4]);
	glUniform1i(glGetUniformLocation(program_[0], "map_5"), 10);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[5]);
	glUniform1i(glGetUniformLocation(program_[0], "map_6"), 11);

	glUniform1f(glGetUniformLocation(program_[0], "width"), (float)FLAT_WIDTH);
	glUniform1f(glGetUniformLocation(program_[0], "height"), (float)FLAT_HEIGHT);

	float mat[16] = { 1, 0, 0, 0,
					  0, 1, 0, 0,
					  0, 0, 1, 0,
					  0, 0, 0, 1};
    
	glUniformMatrix4fv(glGetUniformLocation(program_[0], "rotate_mat4"), 1, GL_FALSE, mat);
}

void render::prepare_aux_vao(GLuint program)
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

	GLuint loc_position1 =  glGetAttribLocation(program, "position");
	GLuint loc_tex_coord =  glGetAttribLocation(program, "vertTexCoord");

	glGenVertexArraysOES(vao_aux_num_, vao_aux_);
	glGenBuffers(vbo_aux_num_, vbo_aux_);
	glBindVertexArrayOES(vao_aux_[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_aux_[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position1);
	glVertexAttribPointer(loc_position1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_aux_[1]);	
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_tex_coord);
	glVertexAttribPointer(loc_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindVertexArrayOES(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//colin_insta360

void render::draw_master_vao(int w, int h, const float* mat, const std::vector<const unsigned char*> image) const
{
	float default_mat[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	
	//如果没有读到陀螺仪数据，那么使用上一次的
	if (mat)
	{
		glUniformMatrix4fv(glGetUniformLocation(program_[0], "rotate_mat4"), 1, GL_FALSE, mat);
	}
	else
	{
		glUniformMatrix4fv(glGetUniformLocation(program_[0], "rotate_mat4"), 1, GL_FALSE, default_mat);
	}

	if (mode_ == INS_MODE_PANORAMA)
	{
		glViewport(0, 0, w, h);
		active_img_texture(image);
		active_uv_texture(texture_uv_);
		glBindVertexArrayOES(vao_[0]);
		glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);
	}
	else
	{
		//top
		glViewport(0, 0, w, h/2);
		active_img_texture(image);
		active_uv_texture(texture_uv_);
		glBindVertexArrayOES(vao_[0]);
		glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);

		//bottom
		glViewport(0, h/2, w, h/2);
		active_uv_texture(texture_uv2_);
		glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);
	}
}

void render::draw_aux_vao(GLuint texture) const
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(program_[1], "tex"), 0);
	glBindVertexArrayOES(vao_aux_[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void render::active_img_texture(const std::vector<const unsigned char*>& image) const
{
	if (image.size() == 6)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture_[0]);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture_[1]);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture_[2]);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, texture_[3]);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[3]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, texture_[4]);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[4]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, texture_[5]);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[5]);
	}
	else if (image.size() == 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[3]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[4]);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[5]);
	}
	else
	{
		LOGERR("invalid img cnt:%d", image.size());
	}
}

void render::active_uv_texture(const GLuint* texture_uv) const
{
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, texture_uv[0]);
	glUniform1i(glGetUniformLocation(program_[0], "map_1"), 6);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, texture_uv[1]);
	glUniform1i(glGetUniformLocation(program_[0], "map_2"), 7);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, texture_uv[2]);
	glUniform1i(glGetUniformLocation(program_[0], "map_3"), 8);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, texture_uv[3]);
	glUniform1i(glGetUniformLocation(program_[0], "map_4"), 9);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, texture_uv[4]);
	glUniform1i(glGetUniformLocation(program_[0], "map_5"), 10);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, texture_uv[5]);
	glUniform1i(glGetUniformLocation(program_[0], "map_6"), 11);
}


// void render::prepare_vao_cube()
// { 
//     int out_width;
//     int out_height;
// 	float* uv[texture_num_] = {nullptr};
// 	float* alpha[texture_num_] = {nullptr};

// 	if (mode_ == INS_MODE_PANORAMA)
// 	{
//         out_width = CUBE_WIDTH*3;
//         out_height = CUBE_HEIGHT*2;

// 		std::string offset;
// 		if (in_width_*9 == in_height_*16)
// 		{
// 			offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9);
// 			LOGINFO("pano 16:9 offset:%s", offset.c_str());
// 		}
// 		else
// 		{
// 			offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3);
// 			LOGINFO("pano 4:3 offset:%s", offset.c_str());
// 		}
		
// 		ins::Dewarp map;
// 		map.setOffset(offset);
// 		std::vector<int> v_width(texture_num_, in_width_);
// 		std::vector<int> v_height(texture_num_, in_height_);
// 		map.init(v_width, v_height, CUBE_WIDTH, CUBE_HEIGHT);

// 		for (int i = 0; i < texture_num_; i++)
// 		{
// 			uv[i] = new float[out_width*out_height*2];
// 			alpha[i] = new float[out_width*out_height]();

// 			float* uv_left = map.getCubeMap(i, ins::CUBETYPE::LEFT, ins::MAPTYPE::OPENGL);
//             float* uv_right = map.getCubeMap(i, ins::CUBETYPE::RIGHT, ins::MAPTYPE::OPENGL);
//             float* uv_top = map.getCubeMap(i, ins::CUBETYPE::TOP, ins::MAPTYPE::OPENGL);
//             float* uv_bottom = map.getCubeMap(i, ins::CUBETYPE::BOTTOM, ins::MAPTYPE::OPENGL);
//             float* uv_back = map.getCubeMap(i, ins::CUBETYPE::BACK, ins::MAPTYPE::OPENGL);
//             float* uv_front = map.getCubeMap(i, ins::CUBETYPE::FRONT, ins::MAPTYPE::OPENGL);
			
// 			float* alpha_left = map.getCubeAlpha(i, ins::CUBETYPE::LEFT, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_right = map.getCubeAlpha(i, ins::CUBETYPE::RIGHT, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_top = map.getCubeAlpha(i, ins::CUBETYPE::TOP, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_bottom = map.getCubeAlpha(i, ins::CUBETYPE::BOTTOM, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_back = map.getCubeAlpha(i, ins::CUBETYPE::BACK, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_front = map.getCubeAlpha(i, ins::CUBETYPE::FRONT, 0, ins::MAPTYPE::OPENGL);
            
//             int uv_offset = 0;
//             int alpha_offset = 0;
//             for (int j = 0; j < CUBE_HEIGHT; j++)
//             {
//                 memcpy(uv[i]+uv_offset, uv_left+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_right+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_top+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;

//                 memcpy(alpha[i]+alpha_offset, alpha_left+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_right+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_top+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//             }

//             for (int j = 0; j < CUBE_HEIGHT; j++)
//             {
//                 memcpy(uv[i]+uv_offset, uv_bottom+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_back+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_front+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;

//                 memcpy(alpha[i]+alpha_offset, alpha_bottom+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_back+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_front+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//             }
// 		}
// 	}
// 	else
// 	{
//         out_width = CUBE_WIDTH*3;
//         out_height = CUBE_HEIGHT*4;

// 		std::vector<int> v_width(texture_num_, in_width_);
// 		std::vector<int> v_height(texture_num_, in_height_);

// 		std::string offset_left = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT);
// 		LOGINFO("3d left offset:%s", offset_left.c_str());
// 		ins::Dewarp map_left;
// 		map_left.setOffset(offset_left);
// 		map_left.init(v_width, v_height, CUBE_WIDTH, CUBE_HEIGHT);

// 		std::string offset_right = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT);
// 		LOGINFO("3d right offset:%s", offset_right.c_str());
// 		ins::Dewarp map_right;
// 		map_right.setOffset(offset_right);
// 		map_right.init(v_width, v_height, CUBE_WIDTH, CUBE_HEIGHT);

// 		for (int i = 0; i < texture_num_; i++)
// 		{
// 			uv[i] = new float[out_width*out_height*2];
// 			alpha[i] = new float[out_width*out_height]();

//             float* uv_left = map_right.getCubeMap(i, ins::CUBETYPE::LEFT, ins::MAPTYPE::OPENGL);
//             float* uv_right = map_right.getCubeMap(i, ins::CUBETYPE::RIGHT, ins::MAPTYPE::OPENGL);
//             float* uv_top = map_right.getCubeMap(i, ins::CUBETYPE::TOP, ins::MAPTYPE::OPENGL);
//             float* uv_bottom = map_right.getCubeMap(i, ins::CUBETYPE::BOTTOM, ins::MAPTYPE::OPENGL);
//             float* uv_back = map_right.getCubeMap(i, ins::CUBETYPE::BACK, ins::MAPTYPE::OPENGL);
//             float* uv_front = map_right.getCubeMap(i, ins::CUBETYPE::FRONT, ins::MAPTYPE::OPENGL);
			
// 			float* alpha_left = map_right.getCubeAlpha(i, ins::CUBETYPE::LEFT, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_right = map_right.getCubeAlpha(i, ins::CUBETYPE::RIGHT, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_top = map_right.getCubeAlpha(i, ins::CUBETYPE::TOP, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_bottom = map_right.getCubeAlpha(i, ins::CUBETYPE::BOTTOM, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_back = map_right.getCubeAlpha(i, ins::CUBETYPE::BACK, 0, ins::MAPTYPE::OPENGL);
//             float* alpha_front = map_right.getCubeAlpha(i, ins::CUBETYPE::FRONT, 0, ins::MAPTYPE::OPENGL);
            
//             int uv_offset = 0;
//             int alpha_offset = 0;
//             for (int j = 0; j < CUBE_HEIGHT; j++)
//             {
//                 memcpy(uv[i]+uv_offset, uv_left+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_right+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_top+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;

//                 memcpy(alpha[i]+alpha_offset, alpha_left+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_right+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_top+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//             }

//             for (int j = 0; j < CUBE_HEIGHT; j++)
//             {
//                 memcpy(uv[i]+uv_offset, uv_bottom+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_back+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_front+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;

//                 memcpy(alpha[i]+alpha_offset, alpha_bottom+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_back+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_front+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//             }

//             uv_left = map_left.getCubeMap(i, ins::CUBETYPE::LEFT, ins::MAPTYPE::OPENGL);
//             uv_right = map_left.getCubeMap(i, ins::CUBETYPE::RIGHT, ins::MAPTYPE::OPENGL);
//             uv_top = map_left.getCubeMap(i, ins::CUBETYPE::TOP, ins::MAPTYPE::OPENGL);
//             uv_bottom = map_left.getCubeMap(i, ins::CUBETYPE::BOTTOM, ins::MAPTYPE::OPENGL);
//             uv_back = map_left.getCubeMap(i, ins::CUBETYPE::BACK, ins::MAPTYPE::OPENGL);
//             uv_front = map_left.getCubeMap(i, ins::CUBETYPE::FRONT, ins::MAPTYPE::OPENGL);
			
// 			alpha_left = map_left.getCubeAlpha(i, ins::CUBETYPE::LEFT, 0, ins::MAPTYPE::OPENGL);
//             alpha_right = map_left.getCubeAlpha(i, ins::CUBETYPE::RIGHT, 0, ins::MAPTYPE::OPENGL);
//             alpha_top = map_left.getCubeAlpha(i, ins::CUBETYPE::TOP, 0, ins::MAPTYPE::OPENGL);
//             alpha_bottom = map_left.getCubeAlpha(i, ins::CUBETYPE::BOTTOM, 0, ins::MAPTYPE::OPENGL);
//             alpha_back = map_left.getCubeAlpha(i, ins::CUBETYPE::BACK, 0, ins::MAPTYPE::OPENGL);
//             alpha_front = map_left.getCubeAlpha(i, ins::CUBETYPE::FRONT, 0, ins::MAPTYPE::OPENGL);
            
//             for (int j = 0; j < CUBE_HEIGHT; j++)
//             {
//                 memcpy(uv[i]+uv_offset, uv_left+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_right+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_top+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;

//                 memcpy(alpha[i]+alpha_offset, alpha_left+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_right+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_top+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//             }

//             for (int j = 0; j < CUBE_HEIGHT; j++)
//             {
//                 memcpy(uv[i]+uv_offset, uv_bottom+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_back+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;
//                 memcpy(uv[i]+uv_offset, uv_front+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
//                 uv_offset += CUBE_WIDTH*2;

//                 memcpy(alpha[i]+alpha_offset, alpha_bottom+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_back+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//                 memcpy(alpha[i]+alpha_offset, alpha_front+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
//                 alpha_offset += CUBE_WIDTH;
//             }
// 		}
// 	}

// 	int k = 0, n = 0, m = 0;
// 	float* vetex_data = new float[out_width*out_height*4]();
// 	for (int j = 0; j < out_height; j++)
// 	{
// 		for (int i = 0; i < out_width; i++)
// 		{
//             vetex_data[k++] = 2.0*(i-i/CUBE_WIDTH)/(float)(out_width-out_width/CUBE_WIDTH) - 1.0;
//             vetex_data[k++] = 1.0 - 2.0*(j-j/CUBE_HEIGHT)/((float)(out_height-out_height/CUBE_HEIGHT));
// 			vetex_data[k++] = 0;
// 			vetex_data[k++] = 1.0; 
// 		}
// 	}

// 	index_num_ = (out_width-out_width/CUBE_WIDTH)*(out_height-out_height/CUBE_HEIGHT)*2*3;
// 	GLuint* index_data = new GLuint[index_num_]();
// 	for (int j = 0, k = 0; j < out_height - 1; j++)
// 	{
//         if ((j+1)%CUBE_HEIGHT == 0) continue;

// 		for (int i = 0; i < out_width - 1; i++)
// 		{
//             if ((i+1)%CUBE_WIDTH == 0) continue;

// 			GLuint tmp1 = i+j*out_width;
// 			GLuint tmp2 = i+(j+1)*out_width;
// 			index_data[k++] = tmp1;
// 			index_data[k++] = tmp2;
// 			index_data[k++] = tmp2+1;
// 			index_data[k++] = tmp1;
// 			index_data[k++] = tmp1+1;
// 			index_data[k++] = tmp2+1;
// 		}
// 	}