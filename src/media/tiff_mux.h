#ifndef _TIFF_MUX_H_
#define _TIFF_MUX_H_

#include "tiff_tag.h"
#include "ins_frame.h"
#include <string>

class tiff_mux
{
public:
    int mux(std::string filename, const ins_img_frame& frame, const tiff_tags& tags);
};

#endif