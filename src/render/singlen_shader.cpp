
#include "singlen_shader.h"

const char s_vertex_shader_quad[] =
	"attribute vec4 position;\n"
	"attribute vec2 vertTexCoord;\n"
	"varying vec2 fragTexCoord;\n"
	"void main()\n"
	"{\n"
	"fragTexCoord = vertTexCoord;\n"
	"gl_Position = position;\n"
	"}\n";

const char s_fragment_shader_quad[] =
	"#extension GL_OES_EGL_image_external : require\n"
    "precision highp float;\n"
	"uniform samplerExternalOES tex;\n"
	"varying vec2 fragTexCoord;\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = texture2D(tex, fragTexCoord);\n"
	"}\n";

const char s_vertex_shader_cross[] =
	"attribute vec4 position;\n"
	"void main()\n"
	"{\n"
	"gl_Position = position;\n"
	"gl_PointSize = 10.0;\n"
	"}\n";

const char s_fragment_shader_cross[] =
    "precision mediump float;\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"}\n";