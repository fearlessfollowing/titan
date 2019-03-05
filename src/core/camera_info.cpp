#include "camera_info.h"
#include "common.h"
#include "inslog.h"
#include "xml_config.h"
#include "ins_util.h"
#include "hw_util.h"
#include "system_properties.h"

std::string camera_info::sn_;
std::string camera_info::fw_version_;
std::string camera_info::camerad_version_;
std::string camera_info::module_version_;
int32_t camera_info::flicker_ = 1; 			// PAL 
uint32_t camera_info::volume_ = 0;
uint32_t camera_info::gyro_orientation_ = INS_GYRO_ORIENTATION_VERTICAL; //0:竖放 1:横放
int32_t camera_info::stablz_type_ 		= INS_STABLZ_TYPE_Z;
bool camera_info::gamma_default_ 		= true;

// std::string camera_info::iq_type_ = "normal";
// std::string camera_info::iq_index_ = "0";
// bool camera_info::hdmi_on_ = false;
// int32_t camera_info::hdmi_width_ = 640;
// int32_t camera_info::hdmi_height_ = 480;
bool camera_info::b_sync_time_ = false;

void camera_info::setup()
{ 
    camerad_version_ = CAMERA_VERSION;
    camerad_version_ += ".";
    camerad_version_ += COMPILE_TIME; 
    
	xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_MODULE_VERSION, module_version_);
    xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_STABLZ_TYPE, stablz_type_);

    std::string cmd = "cat /home/nvidia/insta360/etc/.sys_ver";
    fw_version_ = ins_util::system_call_output(cmd, false);

    cmd = "cat /home/nvidia/insta360/etc/sn";
    sn_ = ins_util::system_call_output(cmd);

    // auto s_iq = property_get("persist.IQType");
    // set_iq_type(s_iq, false);

	LOGINFO("versions fw:%s camerad:%s module:%s sn:%s stype:%d", 
		    fw_version_.c_str(), 
		    camerad_version_.c_str(), 
		    module_version_.c_str(), 
		    sn_.c_str(), 
		    stablz_type_);
}


void camera_info::set_m_version(std::string version)
{
    module_version_ = version;
    xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_MODULE_VERSION, module_version_);
}

int32_t camera_info::set_volume(uint32_t volume) 
{ 
    volume = std::max(0u, volume);
    volume = std::min(127u, volume);
    volume_ = volume;
    hw_util::set_volume(volume_); 
    return INS_OK;
}



// int32_t camera_info::set_iq_type(std::string value, bool update)
// {
//     std::vector<std::string> v;
//     ins_util::split(value, v, "_");
//     if (v.size() == 2) {
//         iq_type_ = v[0];
//         iq_index_ = v[1];
//         if (update) property_set("persist.IQType", value);
//         return INS_OK;
//     } else {
//         LOGERR("invalid iq type:%s", value.c_str());
//         return INS_ERR;
//     }
// }