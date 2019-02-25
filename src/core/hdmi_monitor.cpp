#include "hdmi_monitor.h"
#include "common.h"
#include "inslog.h"
#include "ins_util.h"
//#include "camera_info.h"
#include "video_composer.h"
#include "insx11.h"
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>

std::atomic_bool hdmi_monitor::audio_init_(false);


hdmi_monitor::~hdmi_monitor()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int32_t hdmi_monitor::start()
{
    th_ = std::thread(&hdmi_monitor::task, this);
}

// void hdmi_monitor::task()
// {
//     //std::string cmd1 = "cat /sys/devices/virtual/switch/hdmi/state";
//     std::string cmd2 = "cat /sys/class/graphics/fb0/virtual_size";
//     int32_t w = 0, h = 0;


//     while (!quit_)
//     {
//         //std::string state = ins_util::system_call_output(cmd1);
//         std::string out = ins_util::system_call_output(cmd2);
//         int32_t c_w = 0, c_h = 0;
//         sscanf(out.c_str(), "%d,%d", &c_w, &c_h);
//         if (w && h && (c_w != w || c_h != h))
//         {
//             LOGINFO("hdmi change w:%d h:%d", c_w, c_h/2);
//             if (video_composer::in_hdmi_)
//             {
//                 video_composer::hdmi_change_ = true;
//             }
//             else
//             {
//                 insx11::release_x();
// 	                insx11::setup_x(); 
//             }
//         }
//         w = c_w;
//         h = c_h;

//         usleep(1000*1000);
//     }

//     LOGINFO("hdmi dev monitor exit");
// }


void hdmi_monitor::task()
{
    auto fb = open("/dev/fb0", O_RDONLY);
    if (fb < 0) {
        LOGINFO("fb open fail"); 
        return;
    }

    int32_t w = 0, h = 0;
    
    while (!quit_) {

        struct fb_var_screeninfo fb_var;
        if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
            LOGERR("ioctl FBIOGET_VSCREENINFO fail:%d %s", errno, strerror(errno));
        } else {
            if (w && h && (fb_var.xres != w || fb_var.yres != h)) {
                LOGINFO("hdmi change w:%d h:%d", fb_var.xres, fb_var.yres);
                audio_init_ = true;
                if (video_composer::in_hdmi_) {
                    video_composer::hdmi_change_ = true;
                } else {
                    insx11::release_x();
                    insx11::setup_x(); 
                }
            }

            if (!audio_init_) {
                if (fb_var.xres != 640 || fb_var.yres != 480) {
                    audio_init_ = true;
                } else  {
                    std::string cmd = "cat /sys/devices/virtual/switch/hdmi/state";
                    std::string state = ins_util::system_call_output(cmd);
                    if (state == "1") 
                        audio_init_ = true;
                }
                
                if (audio_init_) {
                    LOGINFO("-----hdmi audio init");
                }
            }

            w = fb_var.xres;
            h = fb_var.yres;

            usleep(1000*1000);
        }
    }

    close(fb);

    LOGINFO("hdmi dev monitor exit");
}




