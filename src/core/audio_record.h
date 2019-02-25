#ifndef _AUDIO_RECORD_H_
#define _AUDIO_RECORD_H_

#include <thread>
#include <mutex>
#include <condition_variable>

class audio_record {
public:
            ~audio_record();
    int32_t start();

private:
    void        sleep_seconds(uint32_t seconds);
    void        task();
    std::thread th_;
    std::mutex  mtx_;
    std::condition_variable cv_;
    bool        quit_ = false;
    int32_t     origin_gain_ = 0;
    
};

#endif