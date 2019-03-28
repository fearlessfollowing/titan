#include "pic_seq_sink.h"
#include <sstream>
#include <fcntl.h>
#include "inslog.h"
#include "access_msg_center.h"
#include "ins_util.h"
#include "jpeg_muxer.h"

uint32_t pic_seq_sink::cnt_ = 0;
std::mutex pic_seq_sink::mtx_s_;
std::condition_variable pic_seq_sink::cv_;

pic_seq_sink::pic_seq_sink(std::string url, bool is_key_sink): url_(url), is_key_sink_(is_key_sink)
{
    th_ = std::thread(&pic_seq_sink::task, this);
}

pic_seq_sink::~pic_seq_sink()
{
    quit_ = true;
    cv_.notify_all();
    INS_THREAD_JOIN(th_);
    LOGINFO("pic sequence destroy");
}

void pic_seq_sink::queue_frame(const std::shared_ptr<ins_frame>& frame)
{
    if (quit_) return; 
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.size() > 30) {
        LOGINFO("%s queue:%d full", url_.c_str(), queue_.size());
        if (is_key_sink_) {
            json_obj obj;
            obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
            obj.set_int("code", INS_ERR_UNSPEED_STORAGE);
            access_msg_center::queue_msg(0, obj.to_string());
        }
        quit_ = true;
    } else {
        queue_.push_back(frame);
    }
}

std::shared_ptr<ins_frame> pic_seq_sink::deque_frame()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) 
		return nullptr;

    auto frame = queue_.front();
    queue_.pop_front();
    return frame;
}

void pic_seq_sink::task()
{
    std::shared_ptr<ins_frame> frame;
    while (!quit_) {
        frame = deque_frame();
        if (frame == nullptr) {
            usleep(30*1000);
            continue;
        }

		LOGINFO("pid [%d] write timeplase seq: %d", frame->pid, frame->sequence);
        write_frame(frame);

        /* 同步各个镜头,保证一组照片存完后再存下一组 */

        std::unique_lock<std::mutex> lock(mtx_s_);
        if (++cnt_ >= INS_CAM_NUM) {	/* Master */
            cnt_ = 0;
            cv_.notify_all();
        } else {
            cv_.wait(lock);
        }
		LOGINFO("wakeup: pic_seq_sink::task");
    }
}


void pic_seq_sink::write_frame(const std::shared_ptr<ins_frame>& frame)
{
    if (is_key_sink_ && !(frame->sequence % 3)) {	/* 每3张,检测一次磁盘空间,不要检测太频繁 */
        if (ins_util::disk_available_size(url_.c_str()) < INS_MIN_SPACE_MB - 100) {
            quit_ = true; 	/* 磁盘满退出   */
            LOGERR("pid:%d no disk space left", frame->pid);
            std::stringstream ss;
		    ss << "{\"name\":\"" << INTERNAL_CMD_SINK_FINISH << "\",\"code\":" << INS_ERR_NO_STORAGE_SPACE << "}";
		    access_msg_center::queue_msg(0, ss.str());
            return;
        }
    }

    std::stringstream ss;
    ss << url_ << "/origin_" << frame->sequence << "_" << frame->pid;
    if (frame->metadata.raw) 
		ss << INS_RAW_EXT;
    else 
		ss << INS_JPEG_EXT;

    jpeg_muxer muxer;
    muxer.create(ss.str(), frame->page_buf->data(), frame->page_buf->size(), &frame->metadata);
}


