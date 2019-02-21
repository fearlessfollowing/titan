
#include "render_base.h"
#include "inslog.h"
#include "common.h"
#include <assert.h>

int render_base::create_program(const char* vertex_src, const char* frag_src)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_src);
    if (vs == 0)
    {
        return INS_ERR;
    }

    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (fs == 0)
    {
        glDeleteShader(vs);
        return INS_ERR;
    }

    GLuint program = link_shader_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
 
    if (program == 0)
    {
        return INS_ERR;
    }
    else 
    {
        program_.push_back(program);
        return INS_OK;
    }
}

void render_base::release_program()
{
    for (auto it = program_.begin(); it != program_.end(); it++)
    {
        glDeleteProgram(*it);
    }
    program_.clear();
}

GLuint render_base::link_shader_program(GLuint vs, GLuint fs)
{
    GLuint program =  glCreateProgram();
    if (program == 0)
    {
            LOGERR("glCreateProgram error:0x%x", glGetError());
            return 0;
    }

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

     GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        LOGERR("glLinkProgram fail");
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &status);
        if (status)
        {
            char* buff = new char[status];
            glGetProgramInfoLog(program, status, nullptr, buff);
            LOGERR("link log:%s", buff);
            delete [] buff;
        }
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint render_base::compile_shader(GLenum shader_type, const char* shader_src)
{
    GLuint shader = glCreateShader(shader_type);
    if (shader == 0)
    {
        LOGERR("glCreateShader error:0x%x", glGetError());
        return 0;
    }

    glShaderSource(shader, 1, &shader_src, 0);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        LOGERR("compile shader type:%d fail", shader_type);
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &status);
        if (status)
        {
            char* buff = new char[status];
            glGetShaderInfoLog(shader, status, nullptr, buff);
            LOGINFO("compile log:%s", buff);
            delete [] buff;
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

