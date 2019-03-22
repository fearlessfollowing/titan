#ifndef _CAMERA_INFO_H_
#define _CAMERA_INFO_H_

#include <string>

class camera_info {
public:
    static void         setup();

	static std::string  get_sn() { return sn_; }
    static std::string  get_fw_ver() { return fw_version_; }
    static std::string  get_ver() { return camerad_version_; }
    
    /* 获取/设置模组的版本号 */
    static std::string  get_m_ver() { return module_version_; }
    static void         set_m_version(std::string version);
    
    static int32_t      get_flicker() { return flicker_; }
    static void         set_flicker(int32_t value) { flicker_ = value; }

    static int32_t      set_volume(uint32_t volume);
    static int32_t      get_volume()   {  return volume_; };

    static void         set_gyro_orientation(uint32_t orientation) {
        gyro_orientation_ = orientation;
    }
    
    static uint32_t     get_gyro_orientation() {
        return gyro_orientation_;
    }

	static void     set_stablz_type(uint32_t type) {
        stablz_type_ = type;
    }

	static uint32_t get_stablz_type() {
        return stablz_type_;
    }
	
    static bool     get_gamma_default() {
        return gamma_default_;
    }	
    static void     set_gamma_default(bool b_default) {
        gamma_default_ = b_default;
    }

	static void     sync_time() { b_sync_time_ = true; }
    static bool     is_sync_time() { return b_sync_time_; }
	

    // static int32_t set_iq_type(std::string value, bool update = true);
    // static std::string get_iq_type()
    // {
    //     return iq_type_;
    // }
    // static std::string get_iq_index()
    // {
    //     return iq_index_;
    // }

    // static int32_t get_hdmi_width()
    // {
    //     return hdmi_width_;
    // }
    // static int32_t get_hdmi_height()
    // {
    //     return hdmi_height_;
    // }
    // static void set_hdmi_resolution(int32_t width, int32_t height)
    // {
    //     hdmi_width_ = width;
    //     hdmi_height_ = height;
    //     printf("set hdmi w:%d h:%d\n", width, height);
    // }

private:
    static std::string  sn_;						/* 序列号 */
    static std::string  fw_version_;				/* 固件版本 */
    static std::string  camerad_version_;		    /* camerad版本 */
    static std::string  module_version_;			/* 模组版本 */
    static int32_t      flicker_; 					/* 1:PAL 0:NTSC */
    static uint32_t     volume_;					/* 音频的音量 */
    static uint32_t     gyro_orientation_;			/* 陀螺仪的放置 0:竖放 1:横放 */
    static int32_t      hdmi_width_;                /* HDMI输出的宽度 */
    static int32_t      hdmi_height_;               /* HDMI输出的高度 */
    static int32_t      stablz_type_;				/* 防抖类型 */
    static bool         gamma_default_;             /* gamma默认值 */
    static bool         b_sync_time_;               /* 系统是否已经同步过时间 */

    // static std::string iq_type_;
    // static std::string iq_index_;
};

#endif