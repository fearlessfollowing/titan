#ifndef _IMG_GYRO_PARSER_H_
#define _IMG_GYRO_PARSER_H_

#include <memory>
#include <string>
#include "ins_mat.h"
#include "metadata.h"
#include "gyro_data_parser.h"
//#include "GyroStabilizer.h"

class rotation_mat_calculator
{
public:
    int open(const std::string& filename, std::shared_ptr<ins_accel_data>& accel_offset);
    std::shared_ptr<ins_mat_3d> get_rotation_mat(double timestamp);
    void close_file();
    int reopen_file();
    void set_start_timestamp(double timestamp);

private:
    int feed_a_gyro_frame();
    gyro_data_parser data_parser_;
    //std::shared_ptr<ins::Stabilizer::GyroStabilizer> stabilizer_;
    std::shared_ptr<ins_accel_data> accel_offset_;
    double start_timestamp_ = 0.0;
};

#endif