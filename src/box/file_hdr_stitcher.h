#ifndef __FILE_HDR_STITCHER_H__
#define __FILE_HDR_STITCHER_H__

#include "file_stitcher.h"
#include <thread>
#include <string>

class file_hdr_stitcher : public file_stitcher
{
public:
    virtual ~file_hdr_stitcher();
    virtual int open(const task_option& option);
private:
    void task(task_option option);
    std::thread th_;
    bool quit_ = false;
};

#endif