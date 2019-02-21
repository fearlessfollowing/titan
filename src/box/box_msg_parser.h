#ifndef _BOX_MSG_PARSER_H_
#define _BOX_MSG_PARSER_H_

#include <memory>
#include <string>
#include "box_task_option.h"
#include "json_obj.h"

class box_msg_parser
{
public:
    static void print_task_entry(const std::shared_ptr<task_entry>& entry);
    int parse(const std::string& param, std::shared_ptr<task_entry>& entry);

private:
    int parse_input_option(const std::shared_ptr<json_obj>& obj, task_input_option& opt);
    int parse_output_option(const std::shared_ptr<json_obj>& obj, task_output_option& opt, const task_input_option& input);
    int parse_blend_option(const std::shared_ptr<json_obj>& obj, task_blend_option& opt, std::string type);
    int parse_gyro_option(const std::string& path, const std::shared_ptr<json_obj>& obj, task_gyro_option& opt);
    int create_dir(std::string filename);
};

#endif