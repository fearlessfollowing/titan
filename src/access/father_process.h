#ifndef _FATHER_PROCESS_H_
#define _FATHER_PROCESS_H_

#include "fifo_read.h"
#include "fifo_write.h"

class father_process
{
public:
    void run();

private:
    void child_process(int fd);
    void father_exit();
    void close_fifo();
    void send_reset_ind_to_client();
    void send_rsp_msg(unsigned int sequence, int rsp_code, const std::string& cmd);
    void start_recv_fifo();
    void start_send_fifo();
    void parse_msg(const char* buff, unsigned int size);
    fifo_read* reader_ = nullptr;
    fifo_write* writer_ = nullptr;
    pid_t child_pid_ = -1;
    //int fd_[2] = {-1, -1};
    bool b_child_kill_by_father_ = false;
};

#endif