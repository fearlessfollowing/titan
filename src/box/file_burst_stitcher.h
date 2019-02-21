#ifndef __FILE_BURST_STITCHER_H__
#define __FILE_BURST_STITCHER_H__

#include "file_stitcher.h"
#include <thread>
#include <string>
#include "metadata.h"

class rotation_mat_calculator;
class ins_mat_3d;

class file_burst_stitcher : public file_stitcher
{
public:
    virtual ~file_burst_stitcher();
    virtual int open(const task_option& option);
private:
    void task(task_option option);
    int get_offset(const task_option& option, std::string& offset);
    void create_mat_calculator(const task_gyro_option& option);
    std::shared_ptr<ins_mat_3d> get_rotation_mat(const task_gyro_option& option, const jpeg_metadata& metadata);
    std::shared_ptr<rotation_mat_calculator> mat_calc_;
    std::thread th_;
    bool quit_ = false;
};

#endif