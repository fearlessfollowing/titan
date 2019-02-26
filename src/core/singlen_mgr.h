#ifndef _SINGLE_LEN_MGR_H_
#define _SINGLE_LEN_MGR_H_

#include "cam_manager.h"
#include "access_msg_option.h"

class singlen_focus;

class singlen_mgr {
public:
            ~singlen_mgr() { 
                stop_focus(); 
            };
    int32_t start_focus(cam_manager* camera, const ins_video_option& opt);
    int32_t stop_focus();

private:
    int32_t     get_circle_pos(int32_t index, float& x, float& y);
    void        print_option(const ins_video_option& option) const;
    std::shared_ptr<singlen_focus>  focus_;
    cam_manager*                    camera_ = nullptr;
};

#endif