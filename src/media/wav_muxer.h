#ifndef _WAV_MUXER_H_
#define _WAV_MUXER_H_

#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <deque>
#include <ins_frame.h>
#include <atomic>

class wav_muxer
{
public:
    wav_muxer(std::string url, int samplerate, int channels);
    ~wav_muxer();
    void queue_frame(const std::shared_ptr<ins_pcm_frame>& buff);

    static std::atomic_llong start_pts_;

private:
    void task();
    std::shared_ptr<insbuff> dequeue_frame();
    void write_frame(unsigned char* data, unsigned int size);
    FILE* fp = nullptr;
    std::string url_;
    int samplerate_ = 0;
    int channels_ = 0;
    int frames_ = 0;
    std::deque<std::shared_ptr<ins_pcm_frame>> queue_;
    std::mutex mtx_;
    std::thread th_;
    bool quit_ = false;
    FILE* fp_ = nullptr;

};

#endif