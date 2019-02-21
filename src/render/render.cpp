
#include "inslog.h"
#include "common.h"
#include "Dewarp/Dewarp.h"
#include "xml_config.h"
#include "shader.h"
#include "ffpngdec.h"
#include "render.h"
#include "ins_clock.h"
#include <assert.h>

#define FLAT_WIDTH   100 
#define FLAT_HEIGHT  50
 
void render::release_render()
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

	if (vao_aux_[0] != 0)
	{
		glDeleteVertexArrays(vao_aux_num_, vao_aux_);
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

	if (texture_logo_)
	{
		glDeleteTextures(1, &texture_logo_);
		texture_logo_ = 0;
	}

	if (frame_buff_)
	{
		glDeleteFramebuffers(1, &frame_buff_);
		frame_buff_ = 0;
	}

	if (o_texture_)
	{
		for (auto& it : out_tex_map_)
		{
			egl_.destroy_egl_img(it.first);
		}
		out_tex_map_.clear();

		glDeleteTextures(out_tex_num_, o_texture_);
		delete[] o_texture_;
		o_texture_ = nullptr;
	}
}

int32_t render::setup()
{
	int ret;
	ret = egl_.setup_pbuff(0, out_width_, out_height_);
	RETURN_IF_NOT_OK(ret);

	ret = create_master_program();
	RETURN_IF_NOT_OK(ret);

	// ret = create_aux_program();
	// RETURN_IF_NOT_OK(ret);

	gen_texture();

	glUseProgram(program_[0]);
	glDisable(GL_DEPTH_TEST);
	prepare_vao();

	return INS_OK;
}

int32_t render::create_out_image(uint32_t num, std::queue<EGLImageKHR>& out_img_queue)
{
	assert(num > 0);
	
	out_tex_num_ = num;
	o_texture_ = new GLuint[num]();
	glGenTextures(num, o_texture_);
	for (uint32_t i = 0; i < out_tex_num_; i++)
	{
		glBindTexture(GL_TEXTURE_2D, o_texture_[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, out_width_, out_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		auto img = egl_.create_egl_img(o_texture_[i]);
		out_tex_map_.insert(std::make_pair(img, o_texture_[i]));
		out_img_queue.push(img);
	}
}

void render::gen_texture()
{
	vetex_w_ = FLAT_WIDTH;
	vetex_h_ = FLAT_HEIGHT;

	glGenTextures(texture_num_, texture_);

	glGenFramebuffers(1, &frame_buff_);
	auto ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (ret != GL_FRAMEBUFFER_COMPLETE)
	{
		LOGERR("frame buff error:0x%x status:0x%x", glGetError(), ret);
	}

	//6个map的texture
	glGenTextures(texture_num_, texture_uv_);
	for (int i = 0; i < texture_num_-1; i++)
	{
		glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, nullptr);
	}

	//如果是3d模式，需要双份的map texture
	if (mode_ != INS_MODE_PANORAMA)
	{
		glGenTextures(texture_num_, texture_uv2_);
		for (int i = 0; i < texture_num_-1; i++)
		{
			glBindTexture(GL_TEXTURE_2D, texture_uv2_[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, nullptr);
		}
	}

	//底部logo texture
	glGenTextures(1, &texture_logo_);
	if (logo_file_ != "") load_logo_texture(logo_file_, texture_logo_);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void render::load_logo_texture(std::string filename, GLuint texture)
{
	ffpngdec dec;
	auto frame = dec.decode(filename);
	if (frame == nullptr) return;

	logo_ratio_ = 2.0*frame->h/frame->w;
	if (logo_ratio_ >= 1.0)
	{
		logo_ratio_ = 0.0;
		LOGERR("logo_ratio_:%f > 1.0", logo_ratio_);
		return;
	}
	
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (frame->fmt == AV_PIX_FMT_RGBA)
	{
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, frame->w, frame->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame->buff->data());
	}
	else
	{
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, frame->w, frame->h, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->buff->data());
	}

	LOGINFO("load logo texture logo_ratio:%f", logo_ratio_);
}

int render::create_master_program()
{
	if (b_external_)
	{
		if (map_type_ == INS_MAP_FLAT)
		{
			return create_program(vertex_shader_optimize, fragment_shader_optimize_v);
		}
		else
		{
			return create_program(vertex_shader_optimize, fragment_shader_optimize_v_cube);
		}
	}
	else
	{
		if (map_type_ == INS_MAP_FLAT)
		{
			return create_program(vertex_shader_optimize, fragment_shader_optimize_p);
		}
		else
		{
		return create_program(vertex_shader_optimize, fragment_shader_optimize_p_cube);
		}
	}
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
	if (mode_ == INS_MODE_PANORAMA)
	{
		std::string offset = xml_config::get_offset(crop_flag_, offset_type_);
		LOGINFO("pano crop:%d type:%d offset:%s", crop_flag_, offset_type_, offset.c_str());
		
		ins::Dewarp map;
		map.setOffset(offset);
		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);
		map.init(v_width, v_height, vetex_w_, vetex_h_);

        //alpha
		for (int i = 0; i < 2; i++)
		{
			float *alpha_1 = map.getAlpha(i*4, 0, ins::MAPTYPE::OPENGL);
			float *alpha_2 = map.getAlpha(i*4 + 1, 0, ins::MAPTYPE::OPENGL);
			float *alpha_3 = map.getAlpha(i*4 + 2, 0, ins::MAPTYPE::OPENGL);
			float *alpha_4 = map.getAlpha(i*4 + 3, 0, ins::MAPTYPE::OPENGL);
			float* alpha = new float[vetex_w_*vetex_h_*4];
			for (int j = 0; j < vetex_w_ * vetex_h_; j++)
			{
				alpha[j*4]   = alpha_1[j];
				alpha[j*4+1] = alpha_2[j];
				alpha[j*4+2] = alpha_3[j];
				alpha[j*4+3] = alpha_4[j];
			}			
			glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, alpha);
			delete[] alpha;
		}
		//tex_coord
		for (int i = 0; i < 4; i++){
			float* uv_1 = map.getMap(i*2, ins::MAPTYPE::OPENGL);
			float* uv_2 = map.getMap(i*2 + 1, ins::MAPTYPE::OPENGL);
			float* uv = new float[(vetex_w_)*vetex_h_*4];
			for (int j = 0; j< vetex_w_ * vetex_h_; j++)
			{
				uv[j*4] = uv_1[j*2];
				uv[j*4+1] = uv_1[j*2+1];
				uv[j*4+2] = uv_2[j*2];
				uv[j*4+3] = uv_2[j*2+1];
			}
			glBindTexture(GL_TEXTURE_2D, texture_uv_[i+2]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, uv);
			delete[] uv;
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);

		std::string offset = xml_config::get_offset(crop_flag_, offset_type_);
		LOGINFO("3d crop:%d type:%d offset:%s", crop_flag_, offset_type_, offset.c_str());

		ins::Dewarp map_left;
		map_left.setOffset(offset);
		map_left.init(v_width, v_height, vetex_w_, vetex_h_);

		ins::Dewarp map_right;
		map_right.setOffset(offset);
		map_right.init(v_width, v_height, vetex_w_, vetex_h_);

		//alpha
		for (int i = 0; i < 2; i++)
		{
			float *alpha_1, *alpha_2, *alpha_3, *alpha_4;
			if (mode_ == INS_MODE_3D_TOP_RIGHT)
			{
				alpha_1 = map_left.getAlphaLeft(i*4, ins::MAPTYPE::OPENGL);
				alpha_2 = map_left.getAlphaLeft(i*4 + 1, ins::MAPTYPE::OPENGL);
				alpha_3 = map_left.getAlphaLeft(i*4 + 2, ins::MAPTYPE::OPENGL);
				alpha_4 = map_left.getAlphaLeft(i*4 + 3, ins::MAPTYPE::OPENGL);
			}
			else
			{
				alpha_1 = map_right.getAlphaRight(i*4, ins::MAPTYPE::OPENGL);
				alpha_2 = map_right.getAlphaRight(i*4 + 1, ins::MAPTYPE::OPENGL);
				alpha_3 = map_right.getAlphaRight(i*4 + 2, ins::MAPTYPE::OPENGL);
				alpha_4 = map_right.getAlphaRight(i*4 + 3, ins::MAPTYPE::OPENGL);
			}
			float* alpha = new float[(vetex_w_)*vetex_h_*4];
			for (int j = 0; j< vetex_w_ * vetex_h_; j++)
			{
				alpha[j*4] = alpha_1[j];
				alpha[j*4+1] = alpha_2[j];
				alpha[j*4+2] = alpha_3[j];
				alpha[j*4+3] = alpha_4[j];
			}			
			glBindTexture(GL_TEXTURE_2D, texture_uv_[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, alpha);
			delete[] alpha;
		}
		//tex_coord
		for (int i = 0; i < 4; i++)
		{
			float *uv_1, *uv_2;
			if (mode_ == INS_MODE_3D_TOP_RIGHT)
			{
				uv_1 = map_left.getMap(i*2, ins::MAPTYPE::OPENGL);
				uv_2 = map_left.getMap(i*2 + 1, ins::MAPTYPE::OPENGL);
			}
			else
			{
				uv_1 = map_right.getMap(i*2, ins::MAPTYPE::OPENGL);
				uv_2 = map_right.getMap(i*2 + 1, ins::MAPTYPE::OPENGL);
			}
			float* uv = new float[(vetex_w_)*vetex_h_*4];
			for (int j = 0; j< vetex_w_ * vetex_h_; j++)
			{
				uv[j*4] = uv_1[j*2];
				uv[j*4+1] = uv_1[j*2+1];
				uv[j*4+2] = uv_2[j*2];
				uv[j*4+3] = uv_2[j*2+1];
			}
			glBindTexture(GL_TEXTURE_2D, texture_uv_[i+2]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, uv);
			delete[] uv;
		}

		//alpha
		for (int i = 0; i < 2; i++)
		{
			float *alpha_1, *alpha_2, *alpha_3, *alpha_4;
			if (mode_ == INS_MODE_3D_TOP_RIGHT)
			{
				alpha_1 = map_right.getAlphaRight(i*4, ins::MAPTYPE::OPENGL);
				alpha_2 = map_right.getAlphaRight(i*4 + 1, ins::MAPTYPE::OPENGL);
				alpha_3 = map_right.getAlphaRight(i*4 + 2, ins::MAPTYPE::OPENGL);
				alpha_4 = map_right.getAlphaRight(i*4 + 3, ins::MAPTYPE::OPENGL);
			}
			else
			{
				alpha_1 = map_left.getAlphaLeft(i*4, ins::MAPTYPE::OPENGL);
				alpha_2 = map_left.getAlphaLeft(i*4 + 1, ins::MAPTYPE::OPENGL);
				alpha_3 = map_left.getAlphaLeft(i*4 + 2, ins::MAPTYPE::OPENGL);
				alpha_4 = map_left.getAlphaLeft(i*4 + 3, ins::MAPTYPE::OPENGL);
			}
			float* alpha = new float[vetex_w_*vetex_h_*4];
			for (int j = 0; j< vetex_w_ * vetex_h_; j++)
			{
				alpha[j*4] = alpha_1[j];
				alpha[j*4+1] = alpha_2[j];
				alpha[j*4+2] = alpha_3[j];
				alpha[j*4+3] = alpha_4[j];
			}			
			glBindTexture(GL_TEXTURE_2D, texture_uv2_[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, alpha);
			delete[] alpha;
		}
		//tex_coord
		for (int i = 0; i < 4; i++)
		{
			float *uv_1, *uv_2; 
			if (mode_ == INS_MODE_3D_TOP_RIGHT)
			{
				uv_1 = map_right.getMap(i*2, ins::MAPTYPE::OPENGL);
				uv_2 = map_right.getMap(i*2 + 1, ins::MAPTYPE::OPENGL);
			}
			else
			{
				uv_1 = map_left.getMap(i*2, ins::MAPTYPE::OPENGL);
				uv_2 = map_left.getMap(i*2 + 1, ins::MAPTYPE::OPENGL);
			}
			float* uv = new float[(vetex_w_)*vetex_h_*4];
			for (int j = 0; j< vetex_w_ * vetex_h_; j++)
			{
				uv[j*4] = uv_1[j*2];
				uv[j*4+1] = uv_1[j*2+1];
				uv[j*4+2] = uv_2[j*2];
				uv[j*4+3] = uv_2[j*2+1];
			}
			glBindTexture(GL_TEXTURE_2D, texture_uv2_[i+2]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vetex_w_, vetex_h_, 0, GL_RGBA, GL_FLOAT, uv);
			delete[] uv;
		}
	}

	int k = 0, n = 0, m=0;
	float* vetex_data = new float[vetex_w_*vetex_h_*4]();
	for (int j = 0; j < vetex_h_; j++)
	{
		for (int i = 0; i < vetex_w_; i++)
		{
			vetex_data[k++] = 2.0 * i / (vetex_w_ - 1.0) - 1.0;
			vetex_data[k++] = 1.0 - 2.0 * j / (vetex_h_ - 1.0);

			vetex_data[k++] = 0;
			vetex_data[k++] = 1.0; 
		}
	}

	index_num_ = (vetex_w_-1)*(vetex_h_-1)*2*3;
	GLuint* index_data = new GLuint[index_num_]();
	for (int j = 0, k = 0; j < vetex_h_ - 1; j++)
	{
		for (int i = 0; i < vetex_w_ - 1; i++)
		{
			GLuint tmp1 = i+j*vetex_w_;
			GLuint tmp2 = i+(j+1)*vetex_w_;
			index_data[k++] = tmp1;
			index_data[k++] = tmp2;
			index_data[k++] = tmp2+1;
			index_data[k++] = tmp1;
			index_data[k++] = tmp1+1;
			index_data[k++] = tmp2+1;
		}
	}

	prepare_master_vao(program_[0], vetex_w_, vetex_h_, vetex_data, index_data);

	// if (program_.size() > 1)
	// {
	// 	prepare_aux_vao(program_[1]);
	// }	

	delete[] vetex_data;
	delete[] index_data;

	LOGINFO("prepare vao fininsh");
}

void render::prepare_master_vao(GLuint program, int w, int h, float* vetex_data, GLuint* index_data)
{
	GLuint loc_position =  glGetAttribLocation(program, "position");

	glGenVertexArrays(vao_num_, vao_);
	glGenBuffers(vbo_num_, vbo_);
	glBindVertexArray(vao_[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
	glBufferData(GL_ARRAY_BUFFER, w*h*4*sizeof(GLfloat), vetex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position);
	glVertexAttribPointer(loc_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_num_*sizeof(GLuint), index_data, GL_STATIC_DRAW);

	glBindVertexArray(0);
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
	glBindTexture(type, texture_[6]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_7"), 6);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(type, texture_[7]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_8"), 7);

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[0]);
	glUniform1i(glGetUniformLocation(program_[0], "map_alpha1234"), 8);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[1]);
	glUniform1i(glGetUniformLocation(program_[0], "map_alpha5678"), 9);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[2]);
	glUniform1i(glGetUniformLocation(program_[0], "map_coord12"), 10);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[3]);
	glUniform1i(glGetUniformLocation(program_[0], "map_coord34"), 11);
	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[4]);
	glUniform1i(glGetUniformLocation(program_[0], "map_coord56"), 12);
	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, texture_uv_[5]);
	glUniform1i(glGetUniformLocation(program_[0], "map_coord78"), 13);

	glActiveTexture(GL_TEXTURE14);
	glBindTexture(GL_TEXTURE_2D, texture_logo_);
	glUniform1i(glGetUniformLocation(program_[0], "tex_logo"), 14);
	glUniform1f(glGetUniformLocation(program_[0], "logo_ratio"), logo_ratio_);

	glUniform1f(glGetUniformLocation(program_[0], "width"), (float)w);
	glUniform1f(glGetUniformLocation(program_[0], "height"), (float)h);

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

	static GLfloat uv_data_3d[] = {
		0.0f, 0.5f,
		0.0f, 0.0f,
		1.0f, 0.5f,
		1.0f, 0.0f,
	};

	GLuint loc_position1 =  glGetAttribLocation(program, "position");
	GLuint loc_tex_coord =  glGetAttribLocation(program, "vertTexCoord");

	glGenVertexArrays(vao_aux_num_, vao_aux_);
	glGenBuffers(vbo_aux_num_, vbo_aux_);
	glBindVertexArray(vao_aux_[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_aux_[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position1);
	glVertexAttribPointer(loc_position1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_aux_[1]);	
	// if (mode_ == INS_MODE_PANORAMA)
	// {
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data), uv_data, GL_STATIC_DRAW);
	// }
	// else
	// {
	// 	glBufferData(GL_ARRAY_BUFFER, sizeof(uv_data_3d), uv_data_3d, GL_STATIC_DRAW); //hdmi output 3d -> pano
	// }
	glEnableVertexAttribArray(loc_tex_coord);
	glVertexAttribPointer(loc_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUniform1i(glGetUniformLocation(program_[1], "tex"), 0);
}

void render::draw_master_vao(const std::vector<EGLImageKHR>& in_img, const EGLImageKHR& out_img, const float* mat)
{
	float default_mat[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

	auto it = out_tex_map_.find(out_img);
	assert(it != out_tex_map_.end());

	glBindFramebuffer(GL_FRAMEBUFFER, frame_buff_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, it->second, 0);
	
	glUseProgram(program_[0]);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mat)
	{
		// printf("%f %f %f %f %f %f %f %f %f %f %f %f\n", 
		// 	mat[0], mat[1], mat[2], mat[3],
		// 	mat[4], mat[5], mat[6], mat[7],
		// 	mat[8], mat[9], mat[10], mat[11]);
		glUniformMatrix4fv(glGetUniformLocation(program_[0], "rotate_mat4"), 1, GL_FALSE, mat);
	}
	else
	{
		glUniformMatrix4fv(glGetUniformLocation(program_[0], "rotate_mat4"), 1, GL_FALSE, default_mat);
	}

	if (mode_ == INS_MODE_PANORAMA)
	{
		glViewport(0, 0, out_width_, out_height_);
		active_img_texture(in_img); 
		active_uv_texture(texture_uv_);
		glBindVertexArray(vao_[0]);
		glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);
	}
	else
	{
		//top
		glViewport(0, 0, out_width_, out_height_/2);
		active_img_texture(in_img);
		active_uv_texture(texture_uv_);
		glBindVertexArray(vao_[0]);
		glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);

		//bottom
		glViewport(0, out_height_/2, out_width_, out_height_/2);
		active_uv_texture(texture_uv2_);
		glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);
	}
	
	// if (surface_type_ == INS_SURFACE_WINDOW)
	// {
	// 	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// 	glUseProgram(program_[1]);

	// 	glViewport(0, 0, screen_width_, screen_height_);
	// 	draw_aux_vao(o_texture_);
	// }
	
	glFlush();
	glFinish();
	egl_.swap_buffers();

	// auto buff = std::make_shared<insbuff>(1920*960*4);
	// glPixelStorei(GL_PACK_ALIGNMENT, 4);
	// glReadPixels(0, 0, 1920, 960, GL_RGBA, GL_UNSIGNED_BYTE, buff->data());
	// static FILE* fp = nullptr;
	// if (!fp) fp = fopen("/home/nvidia/1.rgba", "wb");
	// if (fp) fwrite(buff->data(), 1, buff->size(), fp);

	//LOGINFO("render time:%lf", t.elapse());
}

void render::draw_aux_vao(GLuint texture) const
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(vao_aux_[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void render::active_img_texture(const std::vector<void*>& eglimg)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[0]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[1]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[2]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[2]);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[3]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[3]);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[4]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[4]);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[5]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[5]);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[6]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[6]);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[7]);
	egl_.ImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglimg[7]);

	glActiveTexture(GL_TEXTURE14);
	glBindTexture(GL_TEXTURE_2D, texture_logo_);
}

void render::active_uv_texture(const GLuint* texture_uv) const
{
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, texture_uv[0]);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, texture_uv[1]);
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, texture_uv[2]);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, texture_uv[3]);
	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_2D, texture_uv[4]);
	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, texture_uv[5]);
}


