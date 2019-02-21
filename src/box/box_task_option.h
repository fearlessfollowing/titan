
#ifndef _BOX_TASK_OPTION_H_
#define _BOX_TASK_OPTION_H_

#include <string>
#include <vector>

// struct file_video_input_option
// {
//     std::string path;
//     std::string offset;
//     double start_time; //s
//     double duration; //s
// };

// struct file_video_output_option
// {
//     std::string path;
//     std::string mine;
//     int mode;
//     int map;
//     int width;
//     int height;
//     int bitrate; //kb
//     double framerate;
// };

// struct file_video_option
// {
//     file_video_input_option input;
//     file_video_output_option output;
// };

struct task_input_option
{
    std::string type; // photo/burst/raw/hdr/timelapse/video
    std::vector<std::vector<std::string>> file;
    double duration = 0; //video
    long long total_frames = 0; //video
    bool b_camm = false;
    bool b_audio = false;
    std::string audio_dev;
    int width = 0;
    int height = 0;
    double framerate = 0;
    int count; //hdr/burst/timelapse的张数
    bool b_spatial_audio = true;
};

struct task_output_option
{
    std::string file;
    int speed = 0;
    long long file_size = 0; //MByte
    int width = 0;
    int height = 0;
    //double framerate = 0;
    int bitrate = 0; //kb

    int audio_type = 0; //none/spatial/normal
};

struct task_gyro_option
{
    bool enable = false;
    int version = 2;
    std::string file;
    double time_offset = 0;

    bool b_gravity_offset = false;
    double gravity_x_offset = 0;
    double gravity_y_offset = 0;
    double gravity_z_offset = 0;
};

struct task_blend_option
{
    int mode = 0;
    //int map = 0;
    int len_ver = 3;
    std::string offset;
    int cap_index = -1;
    double cap_time = 0;
};

struct task_option
{
    task_input_option input;
    task_output_option output;
    task_blend_option blend;
    task_gyro_option gyro;
};

struct task_entry
{
    std::string id;
    std::string state;
    int priority = 0;
    task_option option;
};

#endif