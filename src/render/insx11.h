#ifndef __INS_X11_H__
#define __INS_X11_H__

#include <X11/Xlib.h>
#include <stdint.h>

class insx11
{
public:
    static int32_t setup_x();
    static void release_x();
    Window create_x_window(int32_t& width, int32_t& height);
    int32_t release_x_window();
    Display* get_x_display() { return x_display_; };
private:
    Window x_window_ = 0;
    static Window x_window_bg_;
    static Display* x_display_;
};

#endif

