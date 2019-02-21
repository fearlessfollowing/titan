#include "exposure_file.h"
#include "inslog.h"
#include "common.h"

#define EXPOSURE_BLOCK_SIZE 4096

exposure_file::~exposure_file()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);

    if (fp_) 
    {
        fclose(fp_);
        fp_ = nullptr;
    }
}

int32_t exposure_file::open(std::string filename)
{
    fp_ = fopen(filename.c_str(), "wb");
    if (!fp_)
    {
        LOGERR("file:%s open fail", filename.c_str());
        return INS_ERR_FILE_OPEN;
    }
    else
    {
        th_ = std::thread(&exposure_file::task, this);
        LOGINFO("file:%s open success", filename.c_str());
        return INS_OK;
    }
}

void exposure_file::task()
{
    while (1)
    {
        auto buff = deque_buff();
        if (!buff)
        {
            if (quit_) break;
            usleep(50*1000);
            continue;
        }
        else
        {
            fwrite(buff->data(), 1, buff->offset(), fp_);
        }
    }
}

void exposure_file::queue_expouse(const uint8_t* data, uint32_t size)
{
    if (!fp_) return;

    if (!buff_ || buff_->offset() + size > buff_->size())
    {
        if (buff_) queue_buff(buff_);
        buff_ = std::make_shared<insbuff>(EXPOSURE_BLOCK_SIZE);
        buff_->set_offset(0);
    }

    memcpy(buff_->data()+ buff_->offset(), data, size);
    buff_->set_offset(buff_->offset()+size);
}

void exposure_file::queue_buff(const std::shared_ptr<insbuff>& buff)
{
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.push_back(buff);
}

std::shared_ptr<insbuff> exposure_file::deque_buff()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return nullptr;
    auto buff = queue_.front();
    queue_.pop_front();
    return buff;
}