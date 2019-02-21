#ifndef _FILE_STITCHER_H_
#define _FILE_STITCHER_H_

#include "box_task_option.h"
#include <atomic>

class file_stitcher
{
public:
    virtual ~file_stitcher() {};
    virtual int open(const task_option& option) = 0;
    bool try_join(double& progress)
    {
        progress = progress_;
        if (running_th_cnt_ <= 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    int get_stitching_result() 
    { 
        return result_; 
    }

protected:
    int result_ = -1;
    std::atomic_int running_th_cnt_;
    double progress_ = 0;
};

#endif