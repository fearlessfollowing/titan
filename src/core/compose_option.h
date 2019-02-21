#ifndef _COMPOSE_OPTION_H_
#define _COMPOSE_OPTION_H_

#include <stdint.h>
#include <string>
#include <map>
#include "Arational.h"

class stream_sink;

struct compose_option
{
    uint32_t in_w = 0;
    uint32_t in_h = 0;
    uint32_t index = 0;
    std::string mime;
    uint32_t width = 0;
    uint32_t height = 0;
    Arational ori_framerate;
    Arational framerate;
    uint32_t bitrate = 0; //kbit
    int32_t mode = 0;
    int32_t map = 0;
    int32_t crop_flag = 0;
    int32_t offset_type = 0;
    std::string logo_file;
    bool hdmi_display = false;
    bool jpeg = false;
    std::map<uint32_t, std::shared_ptr<stream_sink>> m_sink;
};

#endif