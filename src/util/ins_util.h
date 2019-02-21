
#ifndef _INS_UTIL_H_
#define _INS_UTIL_H_

#include <string>
#include <vector>
#include <memory>
#include "insbuff.h"
#include "Arational.h"

class ins_util
{   
public:
    static int disk_available_size(const char* path);
    static int md5_file(std::string file_name, std::string& md5_value);
    static void split(const std::string& s, std::vector<std::string>& v, const std::string& c);
    static void sleep_s(long long time);
    static void sleep_s(long long time, int read_fd);
    static std::string create_name_by_mode(int mode);
    static std::shared_ptr<insbuff> read_entire_file(std::string file_name);
    //static void check_rtmpd();
    static int read_file(const std::vector<std::string>& file, std::vector<std::shared_ptr<page_buffer>>& data);
    static int file_cnt_under_dir(std::string filename);
    static std::string system_call_output(const std::string& cmd, bool del_rn = true);
    static int system_call(const std::string& cmd);
    static Arational to_real_fps(int framerate);
    static void check_create_dir(const std::string& filename);
};

#endif