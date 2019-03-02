
#ifndef _ACCESS_MSG_SNEDER_H_
#define _ACCESS_MSG_SNEDER_H_

#include <memory>
#include <unordered_map>
#include "fifo_write.h"
#include "json_obj.h"

class access_msg_sender {
public:
    int             setup();
    void            re_setup();

	void            send_ind_msg(std::string cmd, int rsp_code, const json_obj* res_obj = nullptr);
	void            send_ind_msg_(std::string cmd, const json_obj* param);
	void            send_rsp_msg(unsigned int sequence, int rsp_code, const std::string& cmd, const json_obj* res_obj = nullptr, bool b_sync = false) const;
    unsigned int    get_ind_msg_sequece(std::string cmd);
	void            set_ind_msg_sequece(std::string cmd, unsigned int sequence);

private:
    std::shared_ptr<fifo_write> msg_sender_;
	std::shared_ptr<fifo_write> msg_sender_a_;
    std::unordered_map<std::string, unsigned int> map_msg_seq_;
    unsigned int sequence_ = 0;
};

#endif