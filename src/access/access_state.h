#ifndef _ACCESS_STATE_H_
#define _ACCESS_STATE_H_

#include <string>
#include <sys/time.h>
#include "access_msg_option.h"
#include "json_obj.h"

class access_state
{
public:
    friend class access_msg_center;
    std::string state_to_string(int state) const;
    void get_state_param(int state, json_obj& root_obj) const;
    void set_time();
    long long get_elapse_usec();

private:
    std::string mode_to_str(int mode) const;
    ins_video_option preview_opt_;
	ins_video_option option_;
    struct timeval start_time_;
    bool b_live_rec_ = false;
};

#endif