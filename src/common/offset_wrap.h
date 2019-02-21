#ifndef _OFFSET_WRAP_H_
#define _OFFSET_WRAP_H_

#include <string>
#include <vector>
#include <memory>
#include "insbuff.h"
#include "ins_frame.h"

class ins_len_offset;

class offset_wrap
{
public:
    int32_t calc(std::vector<std::string> v_file, uint32_t version);
    int32_t calc(const std::vector<std::shared_ptr<page_buffer>>& v_img, uint32_t version); //未解码img
    int32_t calc(const std::vector<ins_img_frame>& v_data, uint32_t version, bool b_raw = false); //解码后img
    std::string get_offset();
private:
    std::shared_ptr<ins_len_offset> len_offset_;
};

#endif