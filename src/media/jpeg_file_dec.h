#ifndef _JPEG_FILE_DEC_H_
#define _JPEG_FILE_DEC_H_

#include "tjpegdec.h"
#include <string>
#include <memory>
#include "ins_frame.h"

class jpeg_file_dec
{
public:
    static ins_img_frame decode(std::string file_name);
};

#endif