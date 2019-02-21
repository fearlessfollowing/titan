#ifndef _FILE_IMG_STITCHER_H_
#define _FILE_IMG_STITCHER_H_

#include "file_stitcher.h"
#include <thread>
#include <string>

class ins_mat_3d;

class file_img_stitcher : public file_stitcher
{
public:
    virtual ~file_img_stitcher();
    virtual int open(const task_option& option);
private:
    void task(task_option option);
    std::thread th_;
    bool quit_ = false;
};

#endif