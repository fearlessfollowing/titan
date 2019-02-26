#ifndef _QRCODE_SCAN_H_
#define _QRCODE_SCAN_H_

#include <thread>
#include <mutex>
#include <vector>
#include "insbuff.h"

class cam_manager;
class nv_video_dec;
class one_cam_video_buff;
class NvBuffer;
class insbuff;

class qr_scanner {
public:
            ~qr_scanner();
    int32_t open(std::shared_ptr<cam_manager> camera);

private:
    bool    parse_scann_result(void* img);
    int32_t dequeue_frame(NvBuffer* buff, int64_t& pts, uint32_t index);
    void    dec_task(uint32_t index);
    void    scan_task(uint32_t index);
    void    print_fps_info() const;
    std::shared_ptr<cam_manager>                        camera_;
    std::vector<std::shared_ptr<nv_video_dec>>          dec_;
    std::vector<std::shared_ptr<one_cam_video_buff>>    repo_;
    std::thread                                         th_dec_[2];
    std::thread                                         th_scan_[4];
    std::mutex                                          mtx_[2];
    bool                                                quit_ = false;
    std::vector<bool>                                   dec_exit_;
    std::vector<std::shared_ptr<float>>                 uv_; //不能用uniptr,因为不能拷贝就不能push_back;
};

#endif