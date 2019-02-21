
#include "insx11.h"
#include "inslog.h"
#include "common.h"
#include "hw_util.h"

Window insx11::x_window_bg_ = 0;
Display* insx11::x_display_ = nullptr;

Window insx11::create_x_window(int32_t& width, int32_t& height)
{
    auto screen_index = DefaultScreen(x_display_);
    auto w = DisplayWidth(x_display_, screen_index);
    auto h = DisplayHeight(x_display_, screen_index); 

    auto ret = hw_util::get_fb_res(width, height);
    if (ret != INS_OK)
    {
        width = w;
        height = h;
    }
    auto depth = DefaultDepth(x_display_, DefaultScreen(x_display_));

    LOGINFO("------screen width:%u height:%d depth:%d w:%d h:%d", width, height, depth, w, h);

    XSetWindowAttributes window_attributes;
    window_attributes.background_pixel = BlackPixel(x_display_, DefaultScreen(x_display_));
    window_attributes.override_redirect = 1;

    x_window_ = XCreateWindow(x_display_,
                             DefaultRootWindow(x_display_), 0, 0, width, height,
                             0, depth, 
                             CopyFromParent,
                             CopyFromParent,
                             (CWBackPixel | CWOverrideRedirect),
                             &window_attributes);

    XSelectInput(x_display_, (int32_t)x_window_, ExposureMask);
    XMapWindow(x_display_, (int32_t)x_window_);
    XFlush(x_display_);

    return x_window_;
}

int32_t insx11::release_x_window()
{
    if (x_window_)
    {
        XUnmapWindow(x_display_, (int32_t)x_window_);
        XFlush(x_display_);
        XDestroyWindow(x_display_, (int32_t)x_window_);
        x_window_ = 0;
    }
}

int32_t insx11::setup_x()
{
    // auto monitor_str = ins_util::system_call_output("xrandr --listmonitors");
    // system("xrandr -s 1920x1080 -r 30");

    x_display_ = XOpenDisplay(nullptr); 
    if (nullptr == x_display_)
    {
        LOGERR("--------XOpenDisplay fail");
        return INS_ERR;
    }

    auto screen_index = DefaultScreen(x_display_);
    auto w = DisplayWidth(x_display_, screen_index);
    auto h = DisplayHeight(x_display_, screen_index); 

    int32_t width, height;
    auto ret = hw_util::get_fb_res(width, height);
    if (ret != INS_OK)
    {
        width = w;
        height = h;
    }
    auto depth = DefaultDepth(x_display_, DefaultScreen(x_display_));

    LOGINFO("------screen width:%u height:%d depth:%d w:%d h:%d", width, height, depth, w, h);

    XSetWindowAttributes window_attributes;
    window_attributes.background_pixel = BlackPixel(x_display_, DefaultScreen(x_display_));
    window_attributes.override_redirect = 1;

    x_window_bg_ = XCreateWindow(x_display_,
                             DefaultRootWindow(x_display_), 0, 0, width, height,
                             0, depth, 
                             CopyFromParent,
                             CopyFromParent,
                             (CWBackPixel | CWOverrideRedirect),
                             &window_attributes);

    XSelectInput(x_display_, (int32_t)x_window_bg_, ExposureMask);
    XMapWindow(x_display_, (int32_t)x_window_bg_);
    XFlush(x_display_);

    return INS_OK;
}

void insx11::release_x()
{
    if (x_window_bg_)
    {
        XUnmapWindow(x_display_, (int32_t)x_window_bg_);
        XFlush(x_display_);
        XDestroyWindow(x_display_, (int32_t)x_window_bg_);
        x_window_bg_ = 0;
    }

    if (x_display_)
    {
        XCloseDisplay(x_display_);
        x_display_ = nullptr;
    }

    LOGINFO("----release x");
}