#include "nv_jpeg_enc.h"
#include "common.h"
#include "inslog.h"

nv_jpeg_enc::~nv_jpeg_enc()
{
    if (enc_)
    {
        delete enc_;
        enc_ = nullptr;
    }
}

int32_t nv_jpeg_enc::open(std::string name, int32_t width, int32_t height)
{
    name_ = name;
    width_ = width;
    height_ = height;

    enc_ = NvJPEGEncoder::createJPEGEncoder(name.c_str());
    if (!enc_)
    {
        LOGERR("%s createJPEGDecoder fail", name_.c_str());
        return INS_ERR;
    }
    else
    {
        enc_->setCropRect(0, 0, width, height); 
        LOGINFO("%s open w:%d h:%d", name_.c_str(), width, height);
        return INS_OK;
    }
}

std::shared_ptr<insbuff> nv_jpeg_enc::encode(NvBuffer* buff)
{
    auto out_buff = std::make_shared<insbuff>(width_*height_*3/2);
    uint64_t size = out_buff->size();
    uint8_t* out_buff_data = out_buff->data();
    auto ret = enc_->encodeFromBuffer(*buff, JCS_YCbCr, &out_buff_data, size);
    if (ret < 0)
    {
        LOGERR("%s enc fail:%d", name_.c_str(), ret);
        return nullptr;
    }
    else
    {
        out_buff->set_offset(size);
        return out_buff;
    }
}

