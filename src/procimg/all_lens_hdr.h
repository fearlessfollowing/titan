#ifndef _ALL_LEN_HDR_H_
#define _ALL_LEN_HDR_H_

#include <mutex>
#include <vector>
#include <string>
#include "ins_frame.h"

class all_lens_hdr
{
public:
    std::vector<ins_img_frame> process(const std::vector<std::vector<std::string>>& vv_file);
    ins_img_frame single_group_process(const std::vector<std::string>& v_file);
};

#endif