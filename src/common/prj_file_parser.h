#ifndef _PRJ_FILE_PARSER_H_
#define _PRJ_FILE_PARSER_H_

#include <string>
#include "box_task_option.h"

class prj_file_parser
{
public:
    static std::string get_offset(std::string path, int width, int height);
    static int get_input_param(const std::string& path, task_input_option& opt);
    static int get_gyro_param(const std::string& path, task_gyro_option& opt);
};

#endif