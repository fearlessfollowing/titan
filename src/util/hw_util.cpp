#include "hw_util.h"
#include <unistd.h> 
#include "ins_i2c.h"
#include "inslog.h"
#include "common.h"
#include "ins_util.h"
#include <sstream>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <system_properties.h>

bool hw_util::check_35mm_mic_on()
{
    std::string cmd = "amixer cget -c 1 name=\"Jack-state\" | grep \"values=\" | grep -v items | cut -d \"=\" -f 2";

    auto output = ins_util::system_call_output(cmd);

    auto ret = (output == "2" || output == "1")?true:false;

    //LOGINFO("--------output:%d %s", ret, output.c_str());

    return ret;
}

bool hw_util::check_usb_mic_on()
{
    if (access("/dev/snd/pcmC2D0p", 0))
    {
        return false;
    }
    else
    {
        return true;
    }
}

int32_t hw_util::get_temp(const std::string& property)
{
    if (property == "sys.cpu_temp" || property == "sys.gpu_temp")
    {
        auto s_temp = property_get(property);
        return atoi(s_temp.c_str())/1000;
    }
    else if (property == "sys.bat_temp")
    {
        auto s_exist = property_get("sys.bat_exist");
        if (s_exist != "true") return INT32_MIN;
        auto s_temp = property_get(property);
        return (int32_t)atof(s_temp.c_str());
    }
    else
    {
        LOGERR("no support temp property:%s", property.c_str());
        return INT32_MIN;
    }
}

// int hw_util::get_cpu_temp()
// {
//     std::string cmd = "cat /sys/class/thermal/thermal_zone1/temp";
//     std::string output = ins_util::system_call_output(cmd);
//     int32_t MCPU_temp = 0;
//     if (output != "") MCPU_temp = atoi(output.c_str())/1000;

//     //printf("CPU temp:%d\n", MCPU_temp);

//     return MCPU_temp;
// }

// int hw_util::get_gpu_temp()
// {
//     std::string cmd = "cat /sys/class/thermal/thermal_zone2/temp";
//     std::string output = ins_util::system_call_output(cmd);
//     int32_t GPU_temp = 0;
//     if (output != "") GPU_temp = atoi(output.c_str())/1000;

//     //printf("GPU temp:%d\n", GPU_temp);

//     return GPU_temp;
// }

void hw_util::switch_fan(bool b_open)
{
    ins_i2c i2c;
    
    auto ret = i2c.open(0, 0x77);
    if (ret != INS_OK) return;

    uint8_t value = 0;
    ret = i2c.read(0x07, value);
    if (ret != INS_OK) return;

    //LOGINFO("-------read:0x%x", value);

    if (b_open)
    {
        value &= 0x7f;
    }
    else
    {
        value |= 0x80;
    }

    //LOGINFO("-------write:0x%x", value);

    ret = i2c.write(0x07, value);
    if (ret != INS_OK) return;

    LOGINFO("switch fan:%d success", b_open);
}

// void hw_util::switch_fan(bool b_open)
// {
//     if (b_open)
//     {
//         system("echo 255 > /sys/kernel/debug/tegra_fan/target_pwm");
//     }
//     else
//     {
//         system("echo 0 > /sys/kernel/debug/tegra_fan/target_pwm");
//     }
// }

int32_t hw_util::set_volume(uint32_t volume)
{
    volume = std::max(0u, volume);
    volume = std::min(127u, volume);

    // echo 1c 7f7f > /sys/bus/i2c/devices/1-001c/codec_reg
    // echo 1d 7f7f > /sys/bus/i2c/devices/1-001c/codec_reg

    std::stringstream in1p;
    in1p << "echo 1c " << std::hex << volume << volume << " > /sys/bus/i2c/devices/1-001c/codec_reg"; //in1p
    
    std::stringstream in1n;
    in1n << "echo 1d " << std::hex << volume << volume << " > /sys/bus/i2c/devices/1-001c/codec_reg"; //in1n

    //外置音频:音量范围0-7  测试经验值：1的音量大小差不多对应到内置64
    uint32_t outer_value = 0;
    if (volume >= 64)
    {
        outer_value = (volume - 64)/9 + 1;
    }
    std::stringstream outer_mic;
    outer_mic << "echo 0e " << std::hex << outer_value << "00" << " > /sys/bus/i2c/devices/1-001c/codec_reg"; //outer

    //printf("in1p %s\n", in1p.str().c_str());
    //printf("in1n %s\n", in1n.str().c_str());
    //printf("outer mic %s\n", outer_mic.str().c_str());

    system(in1p.str().c_str());
    system(in1n.str().c_str());

    LOGINFO("set volume:%d", volume);

    return INS_OK;
}

int32_t hw_util::get_fb_res(int32_t& w, int32_t& h)
{
    auto fb = open("/dev/fb0", O_RDONLY);
    if (fb < 0)
    {
        LOGINFO("fb open fail"); 
        return INS_ERR;
    }

    struct fb_var_screeninfo fb_var;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) < 0)
    {
        LOGERR("ioctl FBIOGET_VSCREENINFO fail:%d %s", errno, strerror(errno));
        return INS_ERR;
    }

    w = fb_var.xres;
    h = fb_var.yres;
        
    close(fb);

    return INS_OK;
}   