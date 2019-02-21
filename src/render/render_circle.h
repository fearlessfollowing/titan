
#ifndef __RENDER_H__
#define __RENDER_H__

#include "render_base.h"

class render : public render_base
{
public:
	virtual ~render() override
	{
		release_render_cicle();
	};

protected:
	static const int texture_num_ = 6;
	int mode_ = 0;
	int in_width_ = 0;
	int in_height_ = 0;
	void render_init(int width, int height, int mode, bool b_external);
	const int* get_tex_id() const;
	void gen_texture();
	int create_master_program();
	int create_aux_program();
	void prepare_vao();
	void draw_master_vao() const;
	void draw_master_vao(const std::vector<const unsigned char*>& image) const;
	void draw_aux_vao(GLuint texture) const;

private:
	static const int vao_num_ = 1;
	static const int vbo_num_ = 14;
	static const int vao_aux_num_ = 1;
	static const int vbo_aux_num_ = 2;
	
	void release_render_cicle();
	void prepare_master_vao(GLuint program, int width, int height, float* vetex_data, GLuint* index_data, float** uv, float** alpha);
	void prepare_aux_vao(GLuint program);
	void prepare_vao_flat();
	void prepare_vao_cube();

	bool b_external_ = true;
	GLuint vbo_[vbo_num_] = {0};
	GLuint vbo_aux_[vbo_aux_num_] = {0};
	GLuint vao_[vao_num_] = {0};
	GLuint vao_aux_[vao_aux_num_] = {0};
	GLuint texture_[texture_num_]; 
	int index_num_ = 0;
};

#endif