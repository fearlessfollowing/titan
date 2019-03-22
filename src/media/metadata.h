#ifndef _METADATA_H_
#define _METADATA_H_

#include "pool_obj.h"

struct ins_gyro_data
{
    long long   pts = 0;
    float       x = 0.0;
    float       y = 0.0;
    float       z = 0.0;
};

struct ins_accel_data
{
    long long   pts = 0;
    float       x = 0.0;
    float y = 0.0;
    float z = 0.0;
};

struct ins_g_a_data
{
    long long pts = 0;
    float accel_x = 0.0;
    float accel_y = 0.0;
    float accel_z = 0.0;
    float gyro_x = 0.0;
    float gyro_y = 0.0;
    float gyro_z = 0.0;
};

struct gps_sv_info { 
    size_t 	size = 0;
    int32_t prn = 0;         /** Pseudo-random number for the SV. */ 
    float   snr = 0;         /** Signal to noise ratio. */
    float   elevation = 0;   /** Elevation of SV in degrees. */
    float   azimuth = 0;     /** Azimuth of SV in degrees. */
};

#define INS_MAX_SV_NUM 32

struct gps_sv_status {
    uint32_t sv_num = 0;
    gps_sv_info sv_list[INS_MAX_SV_NUM];
};

class ins_gps_data : public pool_obj<ins_gps_data>
{
public:
    int64_t 		pts = 0; //us
    int 			fix_type  = 0;  // -1: poll failed 0/1: 未定位, 2: 二维定位, 3: 三维定位
    double 			latitude = 0.0;
    double 			longitude = 0.0; 
    float 			altitude = 0.0;
    float 			h_accuracy = 0.0;
    float 			v_accuracy = 0.0;
    float 			velocity_east = 0.0;
    float 			velocity_north = 0.0;
    float 			velocity_up = 0.0;
    float 			speed_accuracy = 0.0;
    gps_sv_status 	sv_status;
};

#pragma pack(1) //自定义1字节对齐
struct amba_gyro_data
{
	uint64_t pts = 0; //us
	double accel[3];
	double gyro[3];
#ifdef GYRO_EXT
    double position[3];
#endif
};

#pragma pack() 

struct exposure_times : public pool_obj<exposure_times>
{
    int64_t pts = 0; //us
    int32_t cnt = 0;
    int32_t expousre_time_ns[8];
};

struct ins_euler_angle_data
{
    long long pts = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct ins_gyro_info
{
    long long pts = 0;
    double gyro_x = 0.0;
    double gyro_y = 0.0;
    double gyro_z = 0.0;
    double accel_x = 0.0;
    double accel_y = 0.0;
    double accel_z = 0.0;
    float mat[16];
    double raw_mat[9];
};

struct exif_info
{
    int exp_num = 0;
    int exp_den = 0;
    int f_num = 0;
    int f_den = 0;
    int ev_num = 0;
    int ev_den = 0;
    //int exp_mode = 0;
    int iso = 0;
    int awb = 0;
    int meter = 0;
    int flash = 0;
    int lightsource = 0;
    int shutter_speed_num = 0;
    int shutter_speed_den = 0;
    int focal_length_num = 0;
    int focal_length_den = 0;
    int contrast = 0;
    int sharpness = 0;
    int saturation = 0;
    int brightness_num = 0;
    int brightness_den = 0;
    int exposure_program = 0;
};

struct jpeg_metadata
{
    long long pts = 0;
    bool raw = false;
    
    int width = 4000;
    int height = 3000;
    
    bool b_gyro = false;
    ins_gyro_info gyro;
    bool b_accel_offset = false;
    ins_accel_data accel_offset;

    bool b_exif = false;
    exif_info exif;

    bool b_gps = false;
    ins_gps_data gps;

    bool b_spherical = false;
    int map_type = 0;
};

struct ins_camm_data {
    bool b_gyro = false;
    ins_gyro_data gyro;

    bool b_accel = false;
    ins_accel_data accel;

    bool b_gps = false;
    ins_gps_data gps;
};

#endif