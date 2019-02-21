#ifndef _GYRO_FILE_H_
#define _GYRO_FILE_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <stdio.h>
#include "insbuff.h"

class gyro_file
{
public:
    ~gyro_file();
    int32_t open(std::string filename);
    void queue_gyro(const uint8_t* data, uint32_t size);

private:
    void task();
    void queue_buff(const std::shared_ptr<insbuff>& buff);
    std::shared_ptr<insbuff> deque_buff();
    std::shared_ptr<insbuff> buff_;
    std::deque<std::shared_ptr<insbuff>> queue_;
    FILE* fp_ = nullptr;
    std::mutex mtx_;
    std::thread th_;
    bool quit_;
};

#endif