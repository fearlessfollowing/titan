#ifndef _NV_JPEG_ENC_H_
#define _NV_JPEG_ENC_H_

#include <string>
#include "NvJpegEncoder.h"
#include "insbuff.h"

class nv_jpeg_enc
{
public:
    ~nv_jpeg_enc();
    int32_t open(std::string name, int32_t width, int32_t height);
    std::shared_ptr<insbuff> encode(NvBuffer* buff);
private:
    NvJPEGEncoder* enc_ = nullptr;
    std::string name_;
    int32_t width_ = 0;
    int32_t height_ = 0;
};

#endif