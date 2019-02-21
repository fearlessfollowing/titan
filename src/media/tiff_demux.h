#ifndef _TIFF_DEMUX_H_
#define _TIFF_DEMUX_H_

#include "tiff_tag.h"
#include "ins_frame.h"
#include <string>
#include <memory>

class tiff_demux
{
public:
    int parse(std::string filename, ins_img_frame& frame, tiff_tags& tags);
};

#endif