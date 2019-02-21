#ifndef __TIMELAPSE_MGR_H__
#define __TIMELAPSE_MGR_H__

#include "cam_manager.h"
#include "access_msg_option.h"

class cam_img_repo;
class img_seq_composer;
class usb_sink;

class timelapse_mgr
{
public:
    ~timelapse_mgr();
    int32_t start(cam_manager* camera, const ins_video_option& option);

private:
    int32_t open_composer(std::string path, uint32_t interval);
    void open_usb_sink(std::string path);
    void print_option(const ins_video_option& option) const;
    cam_manager* camera_ = nullptr;
    std::shared_ptr<cam_img_repo> cam_repo_;
    std::shared_ptr<img_seq_composer> composer_;
    std::shared_ptr<usb_sink> usb_sink_;
};

#endif