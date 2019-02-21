#include "render.h"
#include "inslog.h"
#include "common.h"
#include "Dewarp.h"
#include "xml_config.h"

#define FLAT_WIDTH   100
#define FLAT_HEIGHT  50

#define CUBE_WIDTH   32
#define CUBE_HEIGHT  32

static const char vertex_shader[] =
	"attribute vec4 position;\n"
	"attribute vec2 vetex_tex_coord_1;\n"
	"attribute vec2 vetex_tex_coord_2;\n"
	"attribute vec2 vetex_tex_coord_3;\n"
	"attribute vec2 vetex_tex_coord_4;\n"
	"attribute vec2 vetex_tex_coord_5;\n"
	"attribute vec2 vetex_tex_coord_6;\n"
	"attribute float vetex_alpha_1;\n"
	"attribute float vetex_alpha_2;\n"
	"attribute float vetex_alpha_3;\n"
	"attribute float vetex_alpha_4;\n"
	"attribute float vetex_alpha_5;\n"
	"attribute float vetex_alpha_6;\n"
	"varying vec2 frag_tex_coord_1;\n"
	"varying vec2 frag_tex_coord_2;\n"
	"varying vec2 frag_tex_coord_3;\n"
	"varying vec2 frag_tex_coord_4;\n"
	"varying vec2 frag_tex_coord_5;\n"
	"varying vec2 frag_tex_coord_6;\n"
	"varying float frag_alpha_1;\n"
	"varying float frag_alpha_2;\n"
	"varying float frag_alpha_3;\n"
	"varying float frag_alpha_4;\n"
	"varying float frag_alpha_5;\n"
	"varying float frag_alpha_6;\n"
	"void main()\n"
	"{\n"
	"frag_tex_coord_1 = vetex_tex_coord_1;\n"
	"frag_tex_coord_2 = vetex_tex_coord_2;\n"
	"frag_tex_coord_3 = vetex_tex_coord_3;\n"
	"frag_tex_coord_4 = vetex_tex_coord_4;\n"
	"frag_tex_coord_5 = vetex_tex_coord_5;\n"
	"frag_tex_coord_6 = vetex_tex_coord_6;\n"
	"frag_alpha_1 = vetex_alpha_1;\n"
	"frag_alpha_2 = vetex_alpha_2;\n"
	"frag_alpha_3 = vetex_alpha_3;\n"
	"frag_alpha_4 = vetex_alpha_4;\n"
	"frag_alpha_5 = vetex_alpha_5;\n"
	"frag_alpha_6 = vetex_alpha_6;\n"
	"gl_Position = position;\n"
	"}\n";

//for record window render mediump
static const char fragment_shader_ex[] =
	"#extension GL_OES_EGL_image_external : require\n"
	"precision mediump float;\n"
	"uniform samplerExternalOES tex_1;\n"
	"uniform samplerExternalOES tex_2;\n"
	"uniform samplerExternalOES tex_3;\n"
	"uniform samplerExternalOES tex_4;\n"
	"uniform samplerExternalOES tex_5;\n"
	"uniform samplerExternalOES tex_6;\n"
	"varying vec2 frag_tex_coord_1;\n"
	"varying vec2 frag_tex_coord_2;\n"
	"varying vec2 frag_tex_coord_3;\n"
	"varying vec2 frag_tex_coord_4;\n"
	"varying vec2 frag_tex_coord_5;\n"
	"varying vec2 frag_tex_coord_6;\n"
	"varying float frag_alpha_1;\n"
	"varying float frag_alpha_2;\n"
	"varying float frag_alpha_3;\n"
	"varying float frag_alpha_4;\n"
	"varying float frag_alpha_5;\n"
	"varying float frag_alpha_6;\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = texture2D(tex_1, frag_tex_coord_1)*frag_alpha_1 "
	"+ texture2D(tex_2, frag_tex_coord_2)*frag_alpha_2 "
	"+ texture2D(tex_3, frag_tex_coord_3)*frag_alpha_3 "
	"+ texture2D(tex_4, frag_tex_coord_4)*frag_alpha_4 "
	"+ texture2D(tex_5, frag_tex_coord_5)*frag_alpha_5 "
	"+ texture2D(tex_6, frag_tex_coord_6)*frag_alpha_6;\n"
	"}\n";

//for picture pbuff render
static const char fragment_shader[] =
	"precision mediump float;\n"
	"uniform sampler2D tex_1;\n"
	"uniform sampler2D tex_2;\n"
	"uniform sampler2D tex_3;\n"
	"uniform sampler2D tex_4;\n"
	"uniform sampler2D tex_5;\n"
	"uniform sampler2D tex_6;\n"
	"varying vec2 frag_tex_coord_1;\n"
	"varying vec2 frag_tex_coord_2;\n"
	"varying vec2 frag_tex_coord_3;\n"
	"varying vec2 frag_tex_coord_4;\n"
	"varying vec2 frag_tex_coord_5;\n"
	"varying vec2 frag_tex_coord_6;\n"
	"varying float frag_alpha_1;\n"
	"varying float frag_alpha_2;\n"
	"varying float frag_alpha_3;\n"
	"varying float frag_alpha_4;\n"
	"varying float frag_alpha_5;\n"
	"varying float frag_alpha_6;\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = texture2D(tex_1, frag_tex_coord_1)*frag_alpha_1 "
	"+ texture2D(tex_2, frag_tex_coord_2)*frag_alpha_2 "
	"+ texture2D(tex_3, frag_tex_coord_3)*frag_alpha_3 "
	"+ texture2D(tex_4, frag_tex_coord_4)*frag_alpha_4 "
	"+ texture2D(tex_5, frag_tex_coord_5)*frag_alpha_5 "
	"+ texture2D(tex_6, frag_tex_coord_6)*frag_alpha_6;\n"
	"}\n";

static const char vertex_shader_quad[] =
	"attribute vec4 position;\n"
	"attribute vec2 vertTexCoord;\n"
	"varying vec2 fragTexCoord;\n"
	"void main()\n"
	"{\n"
	"fragTexCoord = vertTexCoord;\n"
	"gl_Position = position;\n"
	"}\n";

static const char fragment_shader_quad[] =
    "precision mediump float;\n"
	"uniform sampler2D tex;\n"
	"varying vec2 fragTexCoord;\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = texture2D(tex, fragTexCoord);\n"
	"}\n";

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
}

const int* render::get_tex_id() const
{
	return (int*)texture_;
}

int render::create_master_program()
{
	return create_program(vertex_shader, b_external_?fragment_shader_ex:fragment_shader);
}

int render::create_aux_program()
{
	return create_program(vertex_shader_quad, fragment_shader_quad);
}

void render::prepare_vao()
{
	//prepare_vao_cube();
	prepare_vao_flat();
}

#if 0
void render_circle::prepare_vao()
{ 
	int out_width;
	int out_height;

	if (mode_ == INS_MODE_PANORAMA)
	{
		out_width = VECT_WIDTH;
		out_height = VECT_HEIGHT;
	}
	else
	{
		out_width = VECT_WIDTH;
		out_height = VECT_HEIGHT*2;
	}

	ins::Dewarp map;
	std::string offset;
	//4:3 和 16:9 的offset不一样
	if (in_width_*9 == in_height_*16)
	{
		offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9);
		LOGINFO("16:9 offset:%s", offset.c_str());
	}
	else
	{
		offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3);
		LOGINFO("4:3 offset:%s", offset.c_str());
	}
	map.setOffset(offset);
	std::vector<int> vect_width(texture_num_, in_width_);
	std::vector<int> vect_height(texture_num_, in_height_);
	map.init(vect_width, vect_height, VECT_WIDTH, VECT_HEIGHT);
	
	float* uv[texture_num_] = {nullptr};
	float* alpha[texture_num_] = {nullptr};
	for (int i = 0; i < texture_num_; i++)
	{
		if (mode_ == INS_MODE_PANORAMA)
		{
			uv[i] = map.getMap(i, ins::Dewarp::MAPTYPE::OPENGL);
			alpha[i] = map.getAlpha(i, 0, ins::Dewarp::MAPTYPE::OPENGL);
		}
		else
		{
			uv[i] = new float[out_width*out_height*2];
			float* uv_map = map.getMap(i, ins::Dewarp::MAPTYPE::OPENGL);
			memcpy(uv[i], uv_map, out_width*out_height*sizeof(float));
			memcpy(uv[i]+out_width*out_height, uv_map, out_width*out_height*sizeof(float));

			alpha[i] = new float[out_width*out_height]();
			float* alpha_right = map.getAlphaRight(i, ins::Dewarp::MAPTYPE::OPENGL);
			float* alpha_left = map.getAlphaLeft(i, ins::Dewarp::MAPTYPE::OPENGL);

			if (submode_ == INS_SUBMODE_3D_TOP_LEFT)
			{
				//LOGINFO("----3d top left");
				memcpy(alpha[i], alpha_right, out_width*out_height*sizeof(float)/2);
				memcpy(alpha[i]+out_width*out_height/2, alpha_left, out_width*out_height*sizeof(float)/2);
			}
			else
			{
				//LOGINFO("----3d top right");
				memcpy(alpha[i], alpha_left, out_width*out_height*sizeof(float)/2);
				memcpy(alpha[i]+out_width*out_height/2, alpha_right, out_width*out_height*sizeof(float)/2);
			}
		}
	}

	int k = 0, n = 0, m=0;
	float* vetex_data = new float[out_width*out_height*4]();
	for (int j = 0; j < out_height; j++)
	{
		for (int i = 0; i < out_width; i++)
		{
			vetex_data[k++] = 2.0*i/(float)(out_width-1) - 1.0;
			if (mode_ == INS_MODE_PANORAMA || j < out_height/2)
			{
				vetex_data[k++] = 1.0 - 2.0*j/((float)(out_height-2));
			}
			else
			{
				vetex_data[k++] = 1.0 - 2.0*(j-1)/((float)(out_height-2));
			}
			vetex_data[k++] = 0;
			vetex_data[k++] = 1.0; 
		}
	}

	GLuint* index_data = new GLuint[(out_width-1)*(out_height-2)*2*3]();
	for (int j = 0, k = 0; j < out_height - 1; j++)
	{
		if (mode_ == INS_MODE_3D && j == out_height/2 -1)
		{
			continue;
		}

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

	prepare_master_vao(program_[0], out_width, out_height, vetex_data, index_data, uv, alpha);

	if (program_.size() > 1)
	{
		prepare_aux_vao(program_[1]);
	}	

	delete[] vetex_data;
	delete[] index_data;

	for (int i = 0; i < texture_num_; i++)
	{
		if (mode_ != INS_MODE_PANORAMA)
		{
			delete[] alpha[i];
			delete[] uv[i];
		}
	}

	LOGINFO("prepare vao fininsh");
}

#else

void render::prepare_vao_flat()
{ 
	int out_width;
	int out_height;
	float* uv[texture_num_] = {nullptr};
	float* alpha[texture_num_] = {nullptr};

	if (mode_ == INS_MODE_PANORAMA)
	{
		out_width = FLAT_WIDTH;
		out_height = FLAT_HEIGHT;

		std::string offset;
		if (in_width_*9 == in_height_*16)
		{
			offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9);
			LOGINFO("pano 16:9 offset:%s", offset.c_str());
		}
		else
		{
			offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3);
			LOGINFO("pano 4:3 offset:%s", offset.c_str());
		}
		
		ins::Dewarp map;
		map.setOffset(offset);
		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);
		map.init(v_width, v_height, FLAT_WIDTH, FLAT_HEIGHT);

		for (int i = 0; i < texture_num_; i++)
		{
			uv[i] = new float[out_width*out_height*2];
			alpha[i] = new float[out_width*out_height]();

			float* uv_get = map.getMap(i, ins::MAPTYPE::OPENGL);
			memcpy(uv[i], uv_get, out_width*out_height*sizeof(float)*2);
			
			float* alpha_get = map.getAlpha(i, 0, ins::MAPTYPE::OPENGL);
			memcpy(alpha[i], alpha_get, out_width*out_height*sizeof(float));
		}
	}
	else
	{
		out_width = FLAT_WIDTH;
		out_height = FLAT_HEIGHT*2;

		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);

		std::string offset_left = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT);
		LOGINFO("3d left offset:%s", offset_left.c_str());
		ins::Dewarp map_left;
		map_left.setOffset(offset_left);
		map_left.init(v_width, v_height, FLAT_WIDTH, FLAT_HEIGHT);

		std::string offset_right = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT);
		LOGINFO("3d right offset:%s", offset_right.c_str());
		ins::Dewarp map_right;
		map_right.setOffset(offset_right);
		map_right.init(v_width, v_height, FLAT_WIDTH, FLAT_HEIGHT);

		for (int i = 0; i < texture_num_; i++)
		{
			uv[i] = new float[out_width*out_height*2];
			alpha[i] = new float[out_width*out_height]();

			if (submode_ == INS_SUBMODE_3D_TOP_LEFT)
			{
				float* uv_right = map_right.getMap(i, ins::MAPTYPE::OPENGL);
				memcpy(uv[i], uv_right, out_width*out_height*sizeof(float));
				float* uv_left = map_left.getMap(i, ins::MAPTYPE::OPENGL);
				memcpy(uv[i]+out_width*out_height, uv_left, out_width*out_height*sizeof(float));

				float* alpha_right = map_right.getAlphaRight(i, ins::MAPTYPE::OPENGL);
				float* alpha_left = map_left.getAlphaLeft(i, ins::MAPTYPE::OPENGL);
				memcpy(alpha[i], alpha_right, out_width*out_height*sizeof(float)/2);
				memcpy(alpha[i]+out_width*out_height/2, alpha_left, out_width*out_height*sizeof(float)/2);
			}
			else
			{
				float* uv_left = map_left.getMap(i, ins::MAPTYPE::OPENGL);
				memcpy(uv[i], uv_left, out_width*out_height*sizeof(float));
				float* uv_right = map_right.getMap(i, ins::MAPTYPE::OPENGL);
				memcpy(uv[i]+out_width*out_height, uv_right, out_width*out_height*sizeof(float));

				float* alpha_right = map_right.getAlphaRight(i, ins::MAPTYPE::OPENGL);
				float* alpha_left = map_left.getAlphaLeft(i, ins::MAPTYPE::OPENGL);
				memcpy(alpha[i], alpha_left, out_width*out_height*sizeof(float)/2);
				memcpy(alpha[i]+out_width*out_height/2, alpha_right, out_width*out_height*sizeof(float)/2);
			}
		}
	}

	int k = 0, n = 0, m=0;
	float* vetex_data = new float[out_width*out_height*4]();
	for (int j = 0; j < out_height; j++)
	{
		for (int i = 0; i < out_width; i++)
		{
			vetex_data[k++] = 2.0*(i-i/FLAT_WIDTH)/(float)(out_width-out_width/FLAT_WIDTH) - 1.0;
            vetex_data[k++] = 1.0 - 2.0*(j-j/FLAT_HEIGHT)/((float)(out_height-out_height/FLAT_HEIGHT));
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

	prepare_master_vao(program_[0], out_width, out_height, vetex_data, index_data, uv, alpha);

	if (program_.size() > 1)
	{
		prepare_aux_vao(program_[1]);
	}	

	delete[] vetex_data;
	delete[] index_data;

	for (int i = 0; i < texture_num_; i++)
	{
		delete[] alpha[i];
		delete[] uv[i];
	}

	LOGINFO("prepare vao fininsh");
}
#endif

void render::prepare_vao_cube()
{ 
    int out_width;
    int out_height;
	float* uv[texture_num_] = {nullptr};
	float* alpha[texture_num_] = {nullptr};

	if (mode_ == INS_MODE_PANORAMA)
	{
        out_width = CUBE_WIDTH*3;
        out_height = CUBE_HEIGHT*2;

		std::string offset;
		if (in_width_*9 == in_height_*16)
		{
			offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_16_9);
			LOGINFO("pano 16:9 offset:%s", offset.c_str());
		}
		else
		{
			offset = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3);
			LOGINFO("pano 4:3 offset:%s", offset.c_str());
		}
		
		ins::Dewarp map;
		map.setOffset(offset);
		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);
		map.init(v_width, v_height, CUBE_WIDTH, CUBE_HEIGHT);

		for (int i = 0; i < texture_num_; i++)
		{
			uv[i] = new float[out_width*out_height*2];
			alpha[i] = new float[out_width*out_height]();

			float* uv_left = map.getCubeMap(i, ins::CUBETYPE::LEFT, ins::MAPTYPE::OPENGL);
            float* uv_right = map.getCubeMap(i, ins::CUBETYPE::RIGHT, ins::MAPTYPE::OPENGL);
            float* uv_top = map.getCubeMap(i, ins::CUBETYPE::TOP, ins::MAPTYPE::OPENGL);
            float* uv_bottom = map.getCubeMap(i, ins::CUBETYPE::BOTTOM, ins::MAPTYPE::OPENGL);
            float* uv_back = map.getCubeMap(i, ins::CUBETYPE::BACK, ins::MAPTYPE::OPENGL);
            float* uv_front = map.getCubeMap(i, ins::CUBETYPE::FRONT, ins::MAPTYPE::OPENGL);
			
			float* alpha_left = map.getCubeAlpha(i, ins::CUBETYPE::LEFT, 0, ins::MAPTYPE::OPENGL);
            float* alpha_right = map.getCubeAlpha(i, ins::CUBETYPE::RIGHT, 0, ins::MAPTYPE::OPENGL);
            float* alpha_top = map.getCubeAlpha(i, ins::CUBETYPE::TOP, 0, ins::MAPTYPE::OPENGL);
            float* alpha_bottom = map.getCubeAlpha(i, ins::CUBETYPE::BOTTOM, 0, ins::MAPTYPE::OPENGL);
            float* alpha_back = map.getCubeAlpha(i, ins::CUBETYPE::BACK, 0, ins::MAPTYPE::OPENGL);
            float* alpha_front = map.getCubeAlpha(i, ins::CUBETYPE::FRONT, 0, ins::MAPTYPE::OPENGL);
            
            int uv_offset = 0;
            int alpha_offset = 0;
            for (int j = 0; j < CUBE_HEIGHT; j++)
            {
                memcpy(uv[i]+uv_offset, uv_left+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_right+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_top+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;

                memcpy(alpha[i]+alpha_offset, alpha_left+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_right+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_top+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
            }

            for (int j = 0; j < CUBE_HEIGHT; j++)
            {
                memcpy(uv[i]+uv_offset, uv_bottom+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_back+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_front+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;

                memcpy(alpha[i]+alpha_offset, alpha_bottom+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_back+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_front+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
            }
		}
	}
	else
	{
        out_width = CUBE_WIDTH*3;
        out_height = CUBE_HEIGHT*4;

		std::vector<int> v_width(texture_num_, in_width_);
		std::vector<int> v_height(texture_num_, in_height_);

		std::string offset_left = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_LEFT);
		LOGINFO("3d left offset:%s", offset_left.c_str());
		ins::Dewarp map_left;
		map_left.setOffset(offset_left);
		map_left.init(v_width, v_height, CUBE_WIDTH, CUBE_HEIGHT);

		std::string offset_right = XML_GET_STRING(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_3D_RIGHT);
		LOGINFO("3d right offset:%s", offset_right.c_str());
		ins::Dewarp map_right;
		map_right.setOffset(offset_right);
		map_right.init(v_width, v_height, CUBE_WIDTH, CUBE_HEIGHT);

		for (int i = 0; i < texture_num_; i++)
		{
			uv[i] = new float[out_width*out_height*2];
			alpha[i] = new float[out_width*out_height]();

            float* uv_left = map_right.getCubeMap(i, ins::CUBETYPE::LEFT, ins::MAPTYPE::OPENGL);
            float* uv_right = map_right.getCubeMap(i, ins::CUBETYPE::RIGHT, ins::MAPTYPE::OPENGL);
            float* uv_top = map_right.getCubeMap(i, ins::CUBETYPE::TOP, ins::MAPTYPE::OPENGL);
            float* uv_bottom = map_right.getCubeMap(i, ins::CUBETYPE::BOTTOM, ins::MAPTYPE::OPENGL);
            float* uv_back = map_right.getCubeMap(i, ins::CUBETYPE::BACK, ins::MAPTYPE::OPENGL);
            float* uv_front = map_right.getCubeMap(i, ins::CUBETYPE::FRONT, ins::MAPTYPE::OPENGL);
			
			float* alpha_left = map_right.getCubeAlpha(i, ins::CUBETYPE::LEFT, 0, ins::MAPTYPE::OPENGL);
            float* alpha_right = map_right.getCubeAlpha(i, ins::CUBETYPE::RIGHT, 0, ins::MAPTYPE::OPENGL);
            float* alpha_top = map_right.getCubeAlpha(i, ins::CUBETYPE::TOP, 0, ins::MAPTYPE::OPENGL);
            float* alpha_bottom = map_right.getCubeAlpha(i, ins::CUBETYPE::BOTTOM, 0, ins::MAPTYPE::OPENGL);
            float* alpha_back = map_right.getCubeAlpha(i, ins::CUBETYPE::BACK, 0, ins::MAPTYPE::OPENGL);
            float* alpha_front = map_right.getCubeAlpha(i, ins::CUBETYPE::FRONT, 0, ins::MAPTYPE::OPENGL);
            
            int uv_offset = 0;
            int alpha_offset = 0;
            for (int j = 0; j < CUBE_HEIGHT; j++)
            {
                memcpy(uv[i]+uv_offset, uv_left+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_right+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_top+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;

                memcpy(alpha[i]+alpha_offset, alpha_left+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_right+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_top+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
            }

            for (int j = 0; j < CUBE_HEIGHT; j++)
            {
                memcpy(uv[i]+uv_offset, uv_bottom+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_back+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_front+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;

                memcpy(alpha[i]+alpha_offset, alpha_bottom+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_back+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_front+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
            }

            uv_left = map_left.getCubeMap(i, ins::CUBETYPE::LEFT, ins::MAPTYPE::OPENGL);
            uv_right = map_left.getCubeMap(i, ins::CUBETYPE::RIGHT, ins::MAPTYPE::OPENGL);
            uv_top = map_left.getCubeMap(i, ins::CUBETYPE::TOP, ins::MAPTYPE::OPENGL);
            uv_bottom = map_left.getCubeMap(i, ins::CUBETYPE::BOTTOM, ins::MAPTYPE::OPENGL);
            uv_back = map_left.getCubeMap(i, ins::CUBETYPE::BACK, ins::MAPTYPE::OPENGL);
            uv_front = map_left.getCubeMap(i, ins::CUBETYPE::FRONT, ins::MAPTYPE::OPENGL);
			
			alpha_left = map_left.getCubeAlpha(i, ins::CUBETYPE::LEFT, 0, ins::MAPTYPE::OPENGL);
            alpha_right = map_left.getCubeAlpha(i, ins::CUBETYPE::RIGHT, 0, ins::MAPTYPE::OPENGL);
            alpha_top = map_left.getCubeAlpha(i, ins::CUBETYPE::TOP, 0, ins::MAPTYPE::OPENGL);
            alpha_bottom = map_left.getCubeAlpha(i, ins::CUBETYPE::BOTTOM, 0, ins::MAPTYPE::OPENGL);
            alpha_back = map_left.getCubeAlpha(i, ins::CUBETYPE::BACK, 0, ins::MAPTYPE::OPENGL);
            alpha_front = map_left.getCubeAlpha(i, ins::CUBETYPE::FRONT, 0, ins::MAPTYPE::OPENGL);
            
            for (int j = 0; j < CUBE_HEIGHT; j++)
            {
                memcpy(uv[i]+uv_offset, uv_left+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_right+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_top+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;

                memcpy(alpha[i]+alpha_offset, alpha_left+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_right+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_top+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
            }

            for (int j = 0; j < CUBE_HEIGHT; j++)
            {
                memcpy(uv[i]+uv_offset, uv_bottom+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_back+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;
                memcpy(uv[i]+uv_offset, uv_front+j*CUBE_WIDTH*2, CUBE_WIDTH*2*sizeof(float));
                uv_offset += CUBE_WIDTH*2;

                memcpy(alpha[i]+alpha_offset, alpha_bottom+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_back+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
                memcpy(alpha[i]+alpha_offset, alpha_front+j*CUBE_WIDTH, CUBE_WIDTH*sizeof(float));
                alpha_offset += CUBE_WIDTH;
            }
		}
	}

	int k = 0, n = 0, m = 0;
	float* vetex_data = new float[out_width*out_height*4]();
	for (int j = 0; j < out_height; j++)
	{
		for (int i = 0; i < out_width; i++)
		{
            vetex_data[k++] = 2.0*(i-i/CUBE_WIDTH)/(float)(out_width-out_width/CUBE_WIDTH) - 1.0;
            vetex_data[k++] = 1.0 - 2.0*(j-j/CUBE_HEIGHT)/((float)(out_height-out_height/CUBE_HEIGHT));
			vetex_data[k++] = 0;
			vetex_data[k++] = 1.0; 
		}
	}

	index_num_ = (out_width-out_width/CUBE_WIDTH)*(out_height-out_height/CUBE_HEIGHT)*2*3;
	GLuint* index_data = new GLuint[index_num_]();
	for (int j = 0, k = 0; j < out_height - 1; j++)
	{
        if ((j+1)%CUBE_HEIGHT == 0) continue;

		for (int i = 0; i < out_width - 1; i++)
		{
            if ((i+1)%CUBE_WIDTH == 0) continue;

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

	prepare_master_vao(program_[0], out_width, out_height, vetex_data, index_data, uv, alpha);

	if (program_.size() > 1)
	{
		prepare_aux_vao(program_[1]);
	}	

	delete[] vetex_data;
	delete[] index_data;

	for (int i = 0; i < texture_num_; i++)
	{
		delete[] alpha[i];
		delete[] uv[i];
	}

	LOGINFO("prepare vao fininsh");
}

void render::prepare_master_vao(GLuint program, int width, int height, float* vetex_data, GLuint* index_data, float** uv, float** alpha)
{
	GLuint loc_position =  glGetAttribLocation(program, "position");
	GLuint loc_coord_1 =  glGetAttribLocation(program, "vetex_tex_coord_1");
	GLuint loc_coord_2 =  glGetAttribLocation(program, "vetex_tex_coord_2");
	GLuint loc_coord_3 =  glGetAttribLocation(program, "vetex_tex_coord_3");
	GLuint loc_coord_4 =  glGetAttribLocation(program, "vetex_tex_coord_4");
	GLuint loc_coord_5 =  glGetAttribLocation(program, "vetex_tex_coord_5");
	GLuint loc_coord_6 =  glGetAttribLocation(program, "vetex_tex_coord_6");
	GLuint loc_alpha_1 =  glGetAttribLocation(program, "vetex_alpha_1");
	GLuint loc_alpha_2 =  glGetAttribLocation(program, "vetex_alpha_2");
	GLuint loc_alpha_3 =  glGetAttribLocation(program, "vetex_alpha_3");
	GLuint loc_alpha_4 =  glGetAttribLocation(program, "vetex_alpha_4");
	GLuint loc_alpha_5 =  glGetAttribLocation(program, "vetex_alpha_5");
	GLuint loc_alpha_6 =  glGetAttribLocation(program, "vetex_alpha_6");

	int window_size = width * height;

	glGenVertexArraysOES(vao_num_, vao_);
	glGenBuffers(vbo_num_, vbo_);
	glBindVertexArrayOES(vao_[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
	glBufferData(GL_ARRAY_BUFFER, window_size*4*sizeof(GLfloat), vetex_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_position);
	glVertexAttribPointer(loc_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*2*sizeof(GLfloat), uv[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_coord_1);
	glVertexAttribPointer(loc_coord_1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[2]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*2*sizeof(GLfloat), uv[1], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_coord_2);
	glVertexAttribPointer(loc_coord_2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[3]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*2*sizeof(GLfloat), uv[2], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_coord_3);
	glVertexAttribPointer(loc_coord_3, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[4]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*2*sizeof(GLfloat), uv[3], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_coord_4);
	glVertexAttribPointer(loc_coord_4, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[5]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*2*sizeof(GLfloat), uv[4], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_coord_5);
	glVertexAttribPointer(loc_coord_5, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[6]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*2*sizeof(GLfloat), uv[5], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_coord_6);
	glVertexAttribPointer(loc_coord_6, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_[7]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*sizeof(GLfloat), alpha[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_alpha_1);
	glVertexAttribPointer(loc_alpha_1, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[8]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*sizeof(GLfloat), alpha[1], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_alpha_2);
	glVertexAttribPointer(loc_alpha_2, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[9]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*sizeof(GLfloat), alpha[2], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_alpha_3);
	glVertexAttribPointer(loc_alpha_3, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[10]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*sizeof(GLfloat), alpha[3], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_alpha_4);
	glVertexAttribPointer(loc_alpha_4, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[11]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*sizeof(GLfloat), alpha[4], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_alpha_5);
	glVertexAttribPointer(loc_alpha_5, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_[12]);	
	glBufferData(GL_ARRAY_BUFFER, window_size*sizeof(GLfloat), alpha[5], GL_STATIC_DRAW);
	glEnableVertexAttribArray(loc_alpha_6);
	glVertexAttribPointer(loc_alpha_6, 1, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_[13]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_num_*sizeof(GLuint), index_data, GL_STATIC_DRAW);

	glBindVertexArrayOES(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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

//external texture
void render::draw_master_vao() const
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[0]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_1"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[1]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_2"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[2]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_3"), 2);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[3]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_4"), 3);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[4]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_5"), 4);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_[5]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_6"), 5);
	glBindVertexArrayOES(vao_[0]);

	glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);
}

//internal texture
void render::draw_master_vao(const std::vector<const unsigned char*>& image) const
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_[0]);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[0]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_1"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_[1]);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[1]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_2"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture_[2]);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[2]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_3"), 2);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texture_[3]);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[3]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_4"), 3);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, texture_[4]);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[4]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_5"), 4);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, texture_[5]);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, in_width_, in_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image[5]);
	glUniform1i(glGetUniformLocation(program_[0], "tex_6"), 5);
	glBindVertexArrayOES(vao_[0]);

	glDrawElements(GL_TRIANGLES,index_num_,GL_UNSIGNED_INT,0);
}

void render::draw_aux_vao(GLuint texture) const
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(program_[1], "tex"), 0);
	glBindVertexArrayOES(vao_aux_[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
