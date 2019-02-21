
#include "offset_wrap.h"
#include "inslog.h"
#include "common.h"
#include "ins_len_offset.h"
#include "tjpegdec.h"
#include "ins_util.h"

int32_t offset_wrap::calc(std::vector<std::string> v_file, uint32_t version)
{   
    std::vector<std::shared_ptr<page_buffer>> v_img;
    int ret = ins_util::read_file(v_file, v_img);
    RETURN_IF_NOT_OK(ret);

    return calc(v_img, version);
}

int32_t offset_wrap::calc(const std::vector<std::shared_ptr<page_buffer>>& v_img, uint32_t version)
{
    tjpeg_dec dec;
    int ret = dec.open(TJPF_BGR);
    RETURN_IF_NOT_OK(ret);

    std::vector<ins_img_frame> v_data;
    for (uint32_t i = 0; i < v_img.size(); i++)
    {
        ins_img_frame frame;
        ret = dec.decode(v_img[i]->data(), v_img[i]->size(), frame);
        RETURN_IF_NOT_OK(ret);
        v_data.push_back(frame);
    }

    return calc(v_data, version);
}

int32_t offset_wrap::calc(const std::vector<ins_img_frame>& v_data, uint32_t version, bool b_raw)
{
    len_offset_ = std::make_shared<ins_len_offset>();
    return len_offset_->calc(v_data, version, b_raw);
}

std::string offset_wrap::get_offset()
{
    if (len_offset_)
    {
        return len_offset_->get_offset();
    }
    else
    {
        return "";
    }
}

// std::string offset_wrap::get_16_9_offset()
// {
//     if (len_offset_)
//     {
//         return len_offset_->get_16_9_offset();
//     }
//     else
//     {
//         return "";
//     }
// }

// std::string offset_wrap::get_2_1_offset()
// {
//     if (len_offset_)
//     {
//         return len_offset_->get_2_1_offset();
//     }
//     else
//     {
//         return "";
//     }
// }