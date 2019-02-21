#ifndef _SINGLE_LEN_RENDER_H_
#define _SINGLE_LEN_RENDER_H_

#include "render_base.h"

class singlen_render : public render_base
{
public:
    virtual ~singlen_render() override 
    { 
        release_render(); 
    };
    int setup(float x, float y);
    void draw(EGLImageKHR eglimg);
    int get_texture_id();

private:
    int setup_gles();
    void release_render();
    void prepare_main_vao(GLuint program);
    void prepare_cross_vao(GLuint program, float x, float y);
    int get_circle_pos(int index, float& x, float& y);
    GLuint texture_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_[2] = {0};
    GLuint vao_cross_ = 0;
    GLuint vbo_cross_ = 0;
    int sceen_w_ = 0;
    int sceen_h_ = 0;
    int i_w_ = 0;
    int i_h_ = 0;
};

#endif