

// 　　#ifndef _FBTOOLS_H_
// 　　#define _FBTOOLS_H_
// 　　#include <linux/fb.h>
// 　　//a framebuffer device structure;
// 　　typedef struct fbdev{
// 　　int fb;
// 　　unsigned long fb_mem_offset;
// 　　unsigned long fb_mem;
// 　　struct fb_fix_screeninfo fb_fix;
// 　　struct fb_var_screeninfo fb_var;
// 　　char dev[20];
// 　　} FBDEV, *PFBDEV;
// 　　//open & init a frame buffer
// 　　//to use this function,
// 　　//you must set FBDEV.dev="/dev/fb0"
// 　　//or "/dev/fbX"
// 　　//it's your frame buffer.
// 　　int fb_open(PFBDEV pFbdev);
// 　　//close a frame buffer
// 　　int fb_close(PFBDEV pFbdev);
// 　　//get display depth
// 　　int get_display_depth(PFBDEV pFbdev);
// 　　//full screen clear
// 　　void fb_memset(void *addr, int c, size_t len);
// 　　#endif 

#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
//#include <asm/page.h>

int main(int argc, char* argv[])
{
    auto fb = open("/dev/fb0", O_RDWR);
    if (fb < 0)
    {
        printf("fb open fail\n");
        return -1;
    }

    struct fb_var_screeninfo fb_var;
    auto ret = ioctl(fb, FBIOGET_VSCREENINFO, &fb_var);
    if (ret < 0)
    {
        printf("ioctl FBIOGET_VSCREENINFO fail\n");
        return -1;
    }

    struct fb_fix_screeninfo fb_fix;
    ret = ioctl(fb, FBIOGET_FSCREENINFO, &fb_fix);
    if (ret < 0)
    {
        printf("ioctl FBIOGET_FSCREENINFO fail\n");
        return -1;
    }

    printf("xres = %u, yres = %u.\n", fb_var.xres, fb_var.yres);
    printf("xres_virtual = %u, yres_virtual = %u.\n", fb_var.xres_virtual, fb_var.yres_virtual);
    printf("bpp = %u activate:0x%x\n", fb_var.bits_per_pixel, fb_var.activate);
    printf("smem_start = 0x%x, smem_len = %u.\n", fb_fix.smem_start, fb_fix.smem_len);

    int size = fb_var.xres * fb_var.yres * fb_var.bits_per_pixel / 8;
    printf("size:%d\n", size);

    

    //auto fb_mem_offset = (unsigned long)(fb_fix.smem_start) & (~PAGE_MASK);
    auto fb_mem = mmap(nullptr, fb_fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (fb_mem < 0)
    {
        printf("mmap fail\n");
        return -1;
    }
    else
    {
        printf("map addr:%x\n", fb_mem);
    }

    fb_var.activate |= FB_ACTIVATE_NOW; // | FB_ACTIVATE_FORCE;
    ret = ioctl(fb, FBIOPUT_VSCREENINFO, &fb_var);
    if (ret < 0)
    {
        printf("ioctl FBIOPUT_VSCREENINFO fail\n");
        return -1;
    }

    memset(fb_mem, 0x00, fb_fix.smem_len);

    printf("after set activate:0x%x\n", fb_var.activate);

    usleep(5000*1000);

    munmap(fb_mem, fb_fix.smem_len);
    close(fb);

    return 0;
}