#ifndef _HW_UTIL_H_
#define _HW_UTIL_H_

#include <stdint.h>
#include <string>

class hw_util
{
public:
    static bool check_35mm_mic_on();
    static bool check_usb_mic_on();
    static void switch_fan(bool b_open);
    static int32_t get_temp(const std::string& property);
    static int32_t set_volume(uint32_t volume);
    static int32_t get_fb_res(int32_t& w, int32_t& h);
};

#endif