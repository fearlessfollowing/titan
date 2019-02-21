#include "gps_file.h"
#include "inslog.h"
#include "common.h"

#define BUFF_SIZE 4096*4

struct file_gps_data
{
    uint64_t sync = -1;
    int64_t timestamp = 0;
    int32_t fix_type = 0;
    double latitude = 0.0;
    double longitude = 0.0; 
    double altitude = 0.0;
    double speed_accuracy = 0.0;
};

gps_file::~gps_file()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
    if (fp_)
    {
        fclose(fp_);
        fp_ = nullptr;
        LOGINFO("gps data file:%s close", filename_.c_str());
    }
}

int32_t gps_file::open(std::string filename)
{
    filename_ = filename;
    fp_ = fopen(filename.c_str(), "wb");
    if (fp_ == nullptr)
    {
        LOGERR("file:%s open fail", filename.c_str());
        return INS_ERR_FILE_OPEN;
    }

    th_ = std::thread(&gps_file::task, this);
    
    LOGINFO("gps data file:%s open", filename.c_str());

    return INS_OK;
}

void gps_file::queue_gps(std::shared_ptr<ins_gps_data>& gps)
{
    if (quit_) return;
    
    file_gps_data buff;
    buff.timestamp = gps->pts;
    buff.fix_type = gps->fix_type;
    buff.latitude = gps->latitude;
    buff.longitude = gps->longitude;
    buff.altitude = gps->altitude;
    buff.speed_accuracy = gps->speed_accuracy;

    if (buff_ == nullptr)
    {
        buff_ = std::make_shared<page_buffer>(BUFF_SIZE);
        buff_->set_offset(0);
    }  

    if (buff_->size()-buff_->offset() < sizeof(buff))
    {
        memset(buff_->data() + buff_->offset(), 0xff, buff_->size()-buff_->offset());
        mtx_.lock();
        queue_.push_back(buff_);
        mtx_.unlock();
        buff_ = std::make_shared<page_buffer>(BUFF_SIZE);
        buff_->set_offset(0);
    }

    memcpy(buff_->data() + buff_->offset(), (void*)&buff, sizeof(buff));
    buff_->set_offset(buff_->offset()+sizeof(buff));
}

void gps_file::task()
{
    //write all buff to file when quit
    while (!quit_ || !queue_.empty()) 
    {
        auto buff = dequeue_buff();
        if (buff == nullptr)
        {
            usleep(50*1000);
        }
        else
        {
            fwrite(buff->data(), 1, buff->size(), fp_);
        }
    }
}

std::shared_ptr<page_buffer> gps_file::dequeue_buff()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) return nullptr;
    auto buff = queue_.front();
    queue_.pop_front();
    return buff;
}