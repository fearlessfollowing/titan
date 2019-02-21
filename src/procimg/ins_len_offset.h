
#ifndef _INS_LEN_OFFSET_H_
#define _INS_LEN_OFFSET_H_

#include <vector>
#include <memory>
#include <string>
#include "insbuff.h"
#include "ins_frame.h"

class ins_len_offset
{
public:
    int32_t calc(const std::vector<ins_img_frame>& v_data, uint32_t version, bool b_raw = false);
    std::string get_offset()
    {
        return offset_pic_;
    }
    // std::string get_16_9_offset()
    // {
    //     return offset_pano_16_9_;
    // }
    // std::string get_2_1_offset()
    // {
    //     return offset_pano_2_1_;
    // }
private:
    std::string offset_pic_;
    // std::string offset_pano_16_9_;
    // std::string offset_pano_2_1_;
};

#endif