
#ifndef __RENDER_H__
#define __RENDER_H__

#include "render_base.h"
#include <string>
#include <queue>
#include <map>

class render : public render_base
{
public:
	virtual ~render() override 
	{ 
		release_render(); 
	};

	void set_logo(std::string logo_file) 
	{ 
		logo_file_ = logo_file; 
	};

	void set_mode(int32_t mode)
	{
		mode_ = mode;
	};

	void set_map_type(int32_t map_type)
	{
		map_type_ = map_type;
	};

	void set_offset_type(int32_t crop_flag, int32_t type)
	{
		crop_flag_ = crop_flag;
		offset_type_ = type;
	};

	void set_input_resolution(int32_t width, int32_t height)
	{
		in_width_ = width;
		in_height_ = height;
	};

	void set_output_resolution(int32_t width, int32_t height)
	{
		out_width_ = width;
		out_height_ = height;
	};

	void set_input_texture_type(bool b_external)
	{
		b_external_ = b_external;
	};

	const int32_t* get_input_texture() const
	{
		return (int32_t*)texture_;
	}

	EGLDisplay get_display()
    {
        return egl_.get_display();
    };

	void draw(const std::vector<EGLImageKHR>& in_img, const EGLImageKHR& out_img, const float* mat = nullptr)
	{
		draw_master_vao(in_img, out_img, mat);
	}

	int32_t create_out_image(uint32_t num, std::queue<EGLImageKHR>& out_img_queue);

	int32_t setup();

protected:
	static const int32_t texture_num_ = INS_CAM_NUM;
	int32_t mode_ = 0;
	int32_t map_type_ = 0;
	int32_t in_width_ = 0;
	int32_t in_height_ = 0;
	int32_t out_width_ = 0;
	int32_t out_height_ = 0;
	int32_t screen_width_ = 0;
	int32_t screen_height_ = 0;
	int32_t vetex_w_ = 0;
	int32_t vetex_h_ = 0;
	void gen_texture();
	int32_t create_master_program();
	int32_t create_aux_program();
	void prepare_vao();
	void draw_master_vao(const std::vector<EGLImageKHR>& in_img, const EGLImageKHR& out_img, const float* mat = nullptr);
	void draw_aux_vao(GLuint texture) const;

private:
	static const int32_t vao_num_ = 1;
	static const int32_t vbo_num_ = 2;
	static const int32_t vao_aux_num_ = 1;
	static const int32_t vbo_aux_num_ = 2;
	
	void release_render();
	void prepare_master_vao(GLuint program, int32_t w, int32_t h, float* vetex_data, GLuint* index_data);
	void prepare_aux_vao(GLuint program);
	void prepare_vao_flat();
	void prepare_vao_cube();
	void active_uv_texture(const GLuint* texture_uv) const ;
	void active_img_texture(const std::vector<void*>& eglimg);
	void load_logo_texture(std::string filename, GLuint texture);

	bool b_external_ = true;
	GLuint vbo_[vbo_num_] = {0};
	GLuint vbo_aux_[vbo_aux_num_] = {0};
	GLuint vao_[vao_num_] = {0};
	GLuint vao_aux_[vao_aux_num_] = {0};
	GLuint texture_[texture_num_]; 
	GLuint texture_uv_[texture_num_]; 
	GLuint texture_uv2_[texture_num_]; 
	GLuint texture_logo_ = 0;
	GLuint frame_buff_ = 0;
	int32_t index_num_ = 0;
	std::string logo_file_;
	float logo_ratio_ = 0.0;
	int32_t crop_flag_ = 0;
	int32_t offset_type_ = 0;

	uint32_t out_tex_num_ = 0;
	GLuint* o_texture_ = nullptr;
	std::map<EGLImageKHR, GLuint> out_tex_map_;
};

#endif