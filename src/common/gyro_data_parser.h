#ifndef _GYRO_DATA_PARSER_H_
#define _GYRO_DATA_PARSER_H_

#include <string>
#include <mutex>
//#include <fstream>
#include "metadata.h"

class gyro_data_parser
{
public:
    ~gyro_data_parser();
    int open(const std::string& filename);
    int get_next(ins_g_a_data& data);
    void close();
    int reopen();

private:
    int read_data(char* buff, unsigned size);
    int seek_to_frame_start();
    bool is_start_flag(char* data, unsigned size);
    //std::fstream fs_;
    int fd_ = -1;
    bool eof_ = false;
    long pos_ = 0;
    std::string filename_;
    std::mutex mtx_;
};

#endif