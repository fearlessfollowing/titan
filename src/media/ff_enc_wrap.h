#ifndef _FF_ENC_WRAP_H_
#define _FF_ENC_WRAP_H_

#include <memory>
#include <thread>
#include <mutex>
#include "ffh264enc.h"
#include "ins_queue.h"
#include "ins_frame.h"
#include "sink_interface.h"
#include "ins_enc_option.h"
#include "insbuff.h"

class ff_enc_wrap
{
public:
    ~ff_enc_wrap();
    int open(const ins_enc_option& option);
    int encode(const std::shared_ptr<ins_frame>& frame);
    void add_sink(unsigned index, const std::shared_ptr<sink_interface>& sink);
    void del_sink(unsigned index);
    void eos_and_flush();

private:
    void task();
    void write_frame(const unsigned char* data, unsigned size, long long pts, int flag);
    void set_sink_video_param(const std::shared_ptr<sink_interface>& sink);

    std::shared_ptr<ffh264enc> enc_;
    safe_queue<std::shared_ptr<ins_frame>> queue_;
    std::map<unsigned,std::shared_ptr<sink_interface>> sink_;
    std::thread th_;
    std::mutex mtx_;
    bool quit_ = false;
    std::shared_ptr<insbuff> config_;
    bool b_eos_ = false;
    ins_enc_option option_;
    unsigned max_queue_size_ = 10;
};

#endif