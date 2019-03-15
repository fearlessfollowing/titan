#ifndef _IMG_MGR_H_
#define _IMG_MGR_H_

#include <string>
#include <memory>
#include <map>
#include <stdint.h>
#include <thread>
#include "ins_frame.h"
#include "access_msg_option.h"
#include <future>

class cam_manager;
class cam_img_repo;
class usb_sink;
class stabilization;

class image_mgr {
public:
                            ~image_mgr();
    int32_t                 start(cam_manager* camera, const ins_picture_option& option, bool b_calibration = false);

private:
    void                    task();
    int                     process(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame);
    int32_t                 calibration(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame);
    std::future<int32_t>    open_camera_capture(int32_t& cnt);
    void                    open_usb_sink(std::string path);
    int32_t                 compose(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame, const ins_picture_option& option);
    int32_t                 do_compose(std::vector<ins_img_frame>& v_dec_img, const jpeg_metadata& ori_meta, const ins_picture_option& option);
    int32_t                 hdr_compose();
    int32_t                 save_origin(const std::map<uint32_t, std::shared_ptr<ins_frame>>& m_frame);
    void                    send_pic_finish_msg(int32_t err, const std::string& path) const;
    void                    print_option(const ins_picture_option& option) const;

    ins_picture_option              option_;
    cam_manager*                    camera_ = nullptr;		/* cam_manager管理所有的模组,由access_msg_center中传递过来 */
    std::shared_ptr<cam_img_repo>   img_repo_;
    std::shared_ptr<usb_sink>       usb_sink_;
    std::shared_ptr<stabilization>  stablz_;
    bool                            b_calibration_ = false;		/* 是否为拼接校准操作 */
    std::thread                     th_;
    bool                            quit_ = false;
    uint32_t                        jpeg_seq_ = 0;
    jpeg_metadata                   metadata_;
};

#endif