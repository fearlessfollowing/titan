
#ifndef _INS_OFFSET_CONV_H_
#define _INS_OFFSET_CONV_H_

#include <stdint.h>
#include <string>

class ins_offset_conv
{
public:
    static int32_t conv(const std::string& in, std::string& out, int32_t crop_flag);
};

#endif