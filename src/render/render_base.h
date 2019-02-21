#ifndef __RENDER_BASE_H__
#define __RENDER_BASE_H__

#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include "insegl.h"
#include <vector>

class render_base
{
public:
    virtual ~render_base()
    {
        release_program();
    };
    EGLDisplay get_display()
    {
        return egl_.get_display();
    }

protected:
    int create_program(const char* vertex_src, const char* frag_src);
     insegl egl_;
     std::vector<GLuint> program_;

private:
    GLuint link_shader_program(GLuint vs, GLuint fs);
    GLuint compile_shader(GLenum shader_type, const char* shader_src);
    void release_program();
};

#endif

