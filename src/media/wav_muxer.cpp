
#include "wav_muxer.h"
#include "common.h"
#include "inslog.h"
#include "xml_config.h"

struct wav_header 
{
    unsigned int riff_id;
    unsigned int riff_sz;
    unsigned int riff_fmt;
    unsigned int fmt_id;
    unsigned int fmt_sz;
    unsigned short audio_format;
    unsigned short num_channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample;
    unsigned int data_id;
    unsigned int data_sz;
};

std::atomic_llong wav_muxer::start_pts_(-1);

wav_muxer::~wav_muxer()
{
    if (fp_ == nullptr) return;
    
    quit_ = true;
    INS_THREAD_JOIN(th_);

    wav_header header;
    header.riff_id = 0x46464952;
    header.riff_sz = 0;
    header.riff_fmt = 0x45564157;
    header.fmt_id = 0x20746d66;
    header.audio_format = 1;
    header.fmt_sz = 16;
    header.bits_per_sample = 16;
    header.num_channels = channels_;
    header.sample_rate = samplerate_;
    header.data_id = 0x61746164;
    header.byte_rate = (header.bits_per_sample / 8) * channels_ * samplerate_;
    header.block_align = channels_ * (header.bits_per_sample / 8) * 1024;
    header.data_sz = frames_ * header.block_align;
    header.riff_sz = header.data_sz + sizeof(header) - 8;

    fseek(fp_, 0, SEEK_SET);
    fwrite(&header, 1, sizeof(wav_header), fp_);
    fclose(fp_);
    fp_ = nullptr;

    LOGINFO("wav sink:%s close frames:%d", url_.c_str(), frames_);
}

wav_muxer::wav_muxer(std::string url, int samplerate, int channels)
    : url_(url)
    , samplerate_(samplerate)
    , channels_(channels)
{
    fp_ = fopen(url_.c_str(), "wb");
    if (fp_ == nullptr)
    {
        LOGERR("file:%s open fail", url_.c_str());
        return;
    }

    fseek(fp_, sizeof(wav_header), SEEK_SET);

    //start_pts_ = -1;

    th_ = std::thread(&wav_muxer::task, this);

    LOGINFO("wav sink:%s open samplerate:%d channels:%d", url_.c_str(), samplerate_, channels_);
}

void wav_muxer::task()
{
    while(!quit_)
    {
        auto frame = dequeue_frame();
        if (frame == nullptr)
        {
            usleep(20*1000);
            continue;
        }

        write_frame(frame->data(), frame->size());
    }
}

void wav_muxer::write_frame(unsigned char* data, unsigned int size)
{
    if (fp_ == nullptr) return;
    frames_++;
    int ret = fwrite(data, 1, size, fp_);
    if (ret != (int)size)
    {
        LOGERR("file:%s write error", url_.c_str());
    }
}

void wav_muxer::queue_frame(const std::shared_ptr<ins_pcm_frame>& buff)
{
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.push_back(buff);
    if (queue_.size() < 200) return;
    
    while (queue_.size() > 190)
    {
        queue_.pop_front();
    }

    LOGINFO("wav:%s discard:10", url_.c_str());
}

std::shared_ptr<insbuff> wav_muxer::dequeue_frame()
{
    //if (start_pts_ == -1) return nullptr;

    std::lock_guard<std::mutex> lock(mtx_);

    // int cnt = 0;
    // while (!queue_.empty())
    // {
    //     if (queue_.front()->pts < start_pts_)
    //     {
    //         queue_.pop_front();
    //         cnt++;
    //     }
    //     else
    //     {
    //         break;
    //     }
    // }

    // if (cnt > 0) LOGINFO("-----wav:%s discard:%d", url_.c_str(), cnt);

    if (queue_.empty()) return nullptr;
    auto frame = queue_.front();
    queue_.pop_front();
    return frame->buff;
}