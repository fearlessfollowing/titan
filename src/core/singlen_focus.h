#ifndef _SINGLE_LEN_FOCUS_H_
#define _SINGLE_LEN_FOCUS_H_

#include <memory>
#include <thread>

class one_cam_video_buff;
class nv_video_dec;
class NvBuffer;

class singlen_focus
{
public:
    ~singlen_focus();
    int32_t open(float x, float y);
    void set_video_repo(std::shared_ptr<one_cam_video_buff> repo)
    {
        repo_ = repo;
    };

private:
    int32_t dequeue_frame(NvBuffer* buff, int64_t& pts);
    void dec_task();
    void render_task(float x, float y);
    void print_fps_info() const;
    std::shared_ptr<nv_video_dec> dec_;
    std::thread th_dec_;
    std::thread th_render_;
    bool quit_ = false;
    bool dec_exit_ = false;
    std::shared_ptr<one_cam_video_buff> repo_;
};

#endif