#ifndef _INS_ENC_OPTION_H_
#define _INS_ENC_OPTION_H_

#include <string>

struct ins_enc_option
{
    std::string name = "enc";
    std::string codec_name;
    int width;
    int height;
    double framerate = 30.0;
    int bitrate = 0;
    int spherical_mode = -1; //-1:means not spherical
    unsigned thread_cnt = 1; // for ffmpeg enc
    std::string preset = "superfast"; // for ffmpeg enc
    std::string profile = "high"; // for ffmpeg enc
    //int subpel_mb = 1; // for ffmpeg enc
};

#endif