#ifndef _FILE_RAW_STITCHER_H_
#define _FILE_RAW_STITCHER_H_

#include "file_stitcher.h"
#include <thread>
#include <string>

class file_raw_stitcher : public file_stitcher
{
public:
    virtual ~file_raw_stitcher();
    virtual int open(const task_option& option);

private:
    void task(task_option option);
    bool quit_ = false;
    std::thread th_;
};

#endif