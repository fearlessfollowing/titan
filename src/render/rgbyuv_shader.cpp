
#include "rgbyuv_shader.h"

const char r_vertex_shader[] =
	"attribute vec4 position;\n"
	"attribute vec2 v_tex_coord;\n"
	"varying vec2 f_tex_coord;\n"
	"void main()\n"
	"{\n"
	"f_tex_coord = v_tex_coord;\n"
	"gl_Position = position;\n"
	"}\n";

const char r_fragment_shade_y[] =
    "precision highp float;\n"
	"uniform sampler2D tex;\n"
    "uniform float width;\n"
    "uniform float height;\n"
	"varying vec2 f_tex_coord;\n"
	"void main()\n"
	"{\n"
    "vec2 coord;\n"
    "vec4 color;\n"
    "coord.y = f_tex_coord.y;\n"
    "coord.x = f_tex_coord.x - 1.5/width;\n"
    "color = texture2D(tex, coord);\n"
    "gl_FragColor.r = 0.1826*color.r + 0.6142*color.g + 0.0620*color.b + 16.0/256.0;\n" 
    "coord.x = f_tex_coord.x - 0.5/width;\n"
    "color = texture2D(tex, coord);\n"
    "gl_FragColor.g = 0.1826*color.r + 0.6142*color.g + 0.0620*color.b + 16.0/256.0;\n" 
    "coord.x = f_tex_coord.x + 0.5/width;\n"
    "color = texture2D(tex, coord);\n"
    "gl_FragColor.b = 0.1826*color.r + 0.6142*color.g + 0.0620*color.b + 16.0/256.0;\n" 
    "coord.x = f_tex_coord.x + 1.5/width;\n"
    "color = texture2D(tex, coord);\n"
    "gl_FragColor.a = 0.1826*color.r + 0.6142*color.g + 0.0620*color.b + 16.0/256.0;\n" 
	"}\n";

    //bt709 rgb in [16-235] -> yuv non fullrange
    //"gl_FragColor.r = 0.2126*color.r + 0.7152*color.g + 0.0722*color.b + 16.0/256.0;\n" 

    //bt709 rgb in fullrange -> yuv non fullrange
    //"gl_FragColor.r = 0.1826*color.r + 0.6142*color.g + 0.0620*color.b + 16.0/256.0;\n" 

    //bt601
    //"gl_FragColor.r = 0.299*color.r + 0.587*color.g + 0.114*color.b + 16.0/256.0;\n"


const char r_fragment_shade_uv[] =
    "precision highp float;\n"
	"uniform sampler2D tex;\n"
    "uniform float width;\n"
    "uniform float height;\n"
	"varying vec2 f_tex_coord;\n"
	"void main()\n"
	"{\n"
    "vec2 coord;\n"
    "vec4 color;\n"
    "coord.x = f_tex_coord.x - 1.5/width;\n"
    "coord.y = f_tex_coord.y - 0.5/height;\n"
    "color = texture2D(tex, coord);\n"
    "gl_FragColor.r = -0.1006*color.r - 0.3386*color.g + 0.4392*color.b + 0.5;\n"
    "gl_FragColor.g = 0.4392*color.r - 0.3989*color.g - 0.0403*color.b + 0.5;\n"
    "coord.x = f_tex_coord.x + 0.5/width;\n"
    "coord.y = f_tex_coord.y - 0.5/height;\n"
    "color = texture2D(tex, coord);\n"
    "gl_FragColor.b = -0.1006*color.r - 0.3386*color.g + 0.4392*color.b + 0.5;\n"
    "gl_FragColor.a = 0.4392*color.r - 0.3989*color.g - 0.0403*color.b + 0.5;\n"
	"}\n";

    //bt709 rgb in [16 235]
    // "gl_FragColor.r = -0.09991*color.r - 0.33609*color.g + 0.436*color.b + 0.5;\n"
    // "gl_FragColor.g = 0.615*color.r - 0.55861*color.g - 0.05639*color.b + 0.5;\n"

    //bt709 rgb in fullrange
    //"gl_FragColor.r = -0.1006*color.r - 0.3386*color.g + 0.4392*color.b + 0.5;\n"
    //"gl_FragColor.g = 0.4392*color.r - 0.3989*color.g - 0.0403*color.b + 0.5;\n"

    //bt601
    // "gl_FragColor.r = -0.14713*color.r - 0.28886*color.g + 0.436*color.b + 0.5;\n"
    // "gl_FragColor.g = 0.615*color.r - 0.51499*color.g - 0.10001*color.b + 0.5;\n"

    // "gl_FragColor.r = -0.148*color.r - 0.291*color.g + 0.439*color.b + 0.5;\n"
    // "gl_FragColor.g = 0.439*color.r - 0.368*color.g - 0.071*color.b + 0.5;\n"

const char r_fragment_shader_test[] =
    "precision highp float;\n"
	"uniform sampler2D tex;\n"
    "uniform float width;\n"
    "uniform float height;\n"
	"varying vec2 f_tex_coord;\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = texture2D(tex, f_tex_coord);\n"
	"}\n";