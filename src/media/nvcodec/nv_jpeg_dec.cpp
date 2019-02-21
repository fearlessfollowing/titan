#include "nv_jpeg_dec.h"
#include "common.h"
#include "inslog.h"

nv_jpeg_dec::~nv_jpeg_dec()
{
    if (dec_)
    {
        delete dec_;
        dec_ = nullptr;
    }
    //LOGINFO("%s close", name_.c_str());
}

int32_t nv_jpeg_dec::open(std::string name)
{
    name_ = name;
    dec_ = NvJPEGDecoder::createJPEGDecoder(name.c_str());
    if (!dec_)
    {
        LOGERR("%s createJPEGDecoder fail", name_.c_str());
        return INS_ERR;
    }
    else
    {
        return INS_OK;
    }
}

int32_t nv_jpeg_dec::decode(ins_img_frame& frame, int32_t& fd)
{
    return dec_->decodeToFd(fd, frame.buff->data(), frame.buff->size(), frame.fmt, frame.w, frame.h);
}

int32_t nv_jpeg_dec::decode(const std::shared_ptr<page_buffer>& buff, int32_t& fd)
{
    uint32_t w, h, fmt;
    auto ret = dec_->decodeToFd(fd, buff->data(), buff->offset(), fmt, w, h);
    if (ret < 0)
    {
        LOGERR("%s decodtofd fail", name_.c_str());
        return INS_ERR;
    }
    else
    {
        return INS_OK;
    }
}

// int32_t nv_jpeg_dec::decode(std::shared_ptr<insbuff> in, ins_img_frame& out)
// {
//     return dec_->decodeToBuffer(out.buff->data(), in->data(), in->size(), out.fmt, out.w, out.h);
// }