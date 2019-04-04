
#ifndef _STORAGE_SPEED_TEST_H_
#define _STORAGE_SPEED_TEST_H_

#include <thread>
#include <mutex>
#include <memory>
#include <deque>
#include <vector>
#include <string>
#include "insbuff.h"

class storage_speed_test {
public:
    int     test(const std::string& path);

private:

    void    produce_task(int index);
    void    consume_task(int index);
    int     write_file(int index, const std::shared_ptr<page_buffer>& buff);
    void    queue_buff(int index, const std::shared_ptr<page_buffer>& buff);
    std::shared_ptr<page_buffer> dequeue_buff(int index);

    static const int    num_ = 9;

    std::thread         th_p_[num_];
    std::thread         th_c_[num_];
    std::mutex          mtx_[num_];

    std::vector<int>    fd_;
    std::vector<std::deque<std::shared_ptr<page_buffer>>> queue_;

    int                 result_ = 0;
    bool                quit_ = false;
};

#endif