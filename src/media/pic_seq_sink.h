#ifndef __PIC_SEQ_SINK_H__
#define __PIC_SEQ_SINK_H__
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <string>
#include <memory>
#include "sink_interface.h"

class pic_seq_sink {
public:
			pic_seq_sink(std::string url, bool is_key_sink);
			~pic_seq_sink();
    void 	queue_frame(const std::shared_ptr<ins_frame>& frame);

private:
    std::shared_ptr<ins_frame> deque_frame();
    void 	write_frame(const std::shared_ptr<ins_frame>& frame);
    void 	save_picture_exif(std::string url, const std::shared_ptr<ins_frame>& frame);
    void 	save_picture(std::string url, const std::shared_ptr<ins_frame>& frame);
    void 	task();
    bool 									is_key_sink_ = false;
    std::string 							url_;
    std::mutex 								mtx_;
    std::thread 							th_;
    std::deque<std::shared_ptr<ins_frame>> 	queue_;
    bool 									quit_ = false;
    static uint32_t 						cnt_;
    static std::mutex 						mtx_s_;
    static std::condition_variable 			cv_;
};

#endif