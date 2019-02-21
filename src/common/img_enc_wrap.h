#ifndef __IMG_ENC_WRAP_H__
#define __IMG_ENC_WRAP_H__

#include <stdint.h>
#include <string>
#include <vector>
#include "ins_frame.h"
#include "metadata.h"

class img_enc_wrap
{
public:
    img_enc_wrap(int32_t mode, int32_t map, const jpeg_metadata* metadata = nullptr);
    int32_t encode(const ins_img_frame& frame, std::vector<std::string> url);
    int32_t create_thumbnail(const ins_img_frame& frame, std::string url);
private:
    int32_t mode_ = 0;
    int32_t map_ = 0;
    std::shared_ptr<jpeg_metadata> M;
};

#endif