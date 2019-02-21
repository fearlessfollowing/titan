
#ifndef _HDR_WRAP_H_
#define _HDR_WRAP_H_

#include <memory>
#include <vector>
#include <string>
#include "ins_frame.h"
#include <functional>

class hdr_wrap
{
public:
    void set_progress_cb(std::function<void(double)> cb)
    {
        progress_cb_ = cb;
    }
    int32_t process(const std::vector<std::vector<std::string>>& file, std::vector<ins_img_frame>& hdr_img);
private:
    void task(std::vector<std::string> file, int32_t index);
    int32_t result_ = 0;
    std::function<void(double)> progress_cb_;
    std::vector<ins_img_frame> hdr_img_;
};

#endif