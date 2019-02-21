#ifndef _NV_JPEG_DEC_H_
#define _NV_JPEG_DEC_H_

#include <string>
#include "NvJpegDecoder.h"
#include "ins_frame.h"

class nv_jpeg_dec
{
public:
    ~nv_jpeg_dec();
    int32_t open(std::string name);
    int32_t decode(ins_img_frame& frame, int32_t& fd);
    int32_t decode(const std::shared_ptr<page_buffer>& buff, int32_t& fd);

private:
    NvJPEGDecoder* dec_ = nullptr;
    std::string name_;
};

#endif