
#include "father_process.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "access_msg_buff.h"
#include "json_obj.h"
#include "inslog.h"
#include "access_msg_center.h"
#include "xml_config.h"
#include "ins_signal.h"
#include "ins_util.h"

bool g_quit = false;

void sig_handle(int sig)
{
    LOGINFO("father recv sig:%d", sig);
    g_quit = true;
}

void father_process::run()
{
    ins_log::init(INS_LOG_PATH, "flog");
    LOGINFO("----------camera father runing");
    LOGINFO("version:%s", camera_info::get_ver());

    struct sigaction sig_action;
    sig_action.sa_handler = sig_handle;
	sigaction(SIGINT, &sig_action, nullptr);
    sigaction(SIGQUIT, &sig_action, nullptr);
    sigaction(SIGTERM, &sig_action, nullptr);

    int check_cnt = 0;

	while (!g_quit)
	{
		if (child_pid_ == -1)
		{
            close_fifo();
			child_pid_ = fork();
			if (child_pid_ < 0)
			{
                LOGERR("fork error");
			}
			//in child process
			else if (child_pid_ == 0)
			{
                struct sigaction sig_action;
                memset(&sig_action, 0, sizeof(sig_action));
                sig_action.sa_handler = SIG_DFL;
                sigaction(SIGCHLD, &sig_action, nullptr);
                signal(SIGCHLD, SIG_DFL);
				child_process(0);
				return;
			}
			//in father process
			else
			{
                start_recv_fifo();
                start_send_fifo();
                struct sigaction sig_action;
                memset(&sig_action, 0, sizeof(sig_action));
                sig_action.sa_handler = SIG_IGN;
                sigaction(SIGCHLD, &sig_action, nullptr);
                signal(SIGCHLD, SIG_IGN);
                sigaction(SIGPIPE, &sig_action, nullptr);
                signal(SIGPIPE, SIG_IGN);
			}
		}
		else
		{
			if (kill(child_pid_, 0) != 0)
			{
                LOGINFO("child process die, restart");
				child_pid_ = -1;
                if (!b_child_kill_by_father_)
                {
                    send_reset_ind_to_client();
                }
                b_child_kill_by_father_ = false;
			}
			else
			{
				usleep(100*1000); //100ms
			}
		}

        // if (!(++check_cnt%30)) ins_util::check_rtmpd();
	}

    father_exit();
}

void father_process::child_process(int fd)
{
    ins_log::init(INS_LOG_PATH, "log");

    LOGINFO("----------camera child runing");

    // setpriority(PRIO_PROCESS, 0, -18);
    nice(1);

	auto child_process_entry = std::make_shared<access_msg_center>();

	if (child_process_entry->setup()) return;

	wait_terminate_signal();

    child_process_entry = nullptr;

    LOGINFO("----------camera child exit");
}

void father_process::close_fifo()
{
    if (reader_ != nullptr)
    {
        delete reader_;
        reader_ = nullptr;
    }

    if (writer_ != nullptr)
    {
        delete writer_;
        writer_ = nullptr;
    }
}

void father_process::father_exit()
{
    close_fifo();

    if (child_pid_ != -1)
    {
        waitpid(child_pid_, nullptr, 0);
        child_pid_ = -1;
    }

    LOGINFO("----------camera father exit");
}

void father_process::start_recv_fifo()
{
    auto func = [this](const char* buff, unsigned int size)
	{
		parse_msg(buff, size);
	};

	reader_ = new fifo_read;
	if (reader_->start(INS_FIFO_TO_SERVER_FATHER, func))
    {
        delete reader_;
        reader_ = nullptr;
    }
}

void father_process::start_send_fifo()
{
    writer_ = new fifo_write;
	if (writer_->start(INS_FIFO_TO_CLIENT_FATHER))
    {
        delete writer_;
        writer_ = nullptr;
    }
}

void father_process::parse_msg(const char* buff, unsigned int size)
{
    access_msg_head* head = (access_msg_head*)buff;
    head->sequece = ntohl(head->sequece);
    head->content_len = ntohl(head->content_len);

    auto msg = std::make_shared<access_msg_buff>(head->sequece, buff + sizeof(access_msg_head), head->content_len);

    LOGINFO("father process recv msg: sequece:%d %s", head->sequece, msg->content);

    auto root_obj = std::make_shared<json_obj>(msg->content);

	std::string cmd;
    root_obj->get_string(ACCESS_MSG_NAME, cmd);
    int rsp_code;
	
    if (cmd == ACCESS_CMD_RESET)
    {
        b_child_kill_by_father_ = true;
        kill(child_pid_, 9);
        rsp_code = INS_OK;
    } 
    else if (cmd == INTERNAL_CMD_FIFO_CLOSE)
    {
        delete writer_;
        writer_ = nullptr;
        start_send_fifo();
        return;
    }
    else
    {
        //LOGINFO("father process recv unsupport msg");
        rsp_code = INS_ERR;
    }

    send_rsp_msg(head->sequece, rsp_code, cmd);
}

void father_process::send_reset_ind_to_client()
{
    fifo_write sender;
	if (sender.start(INS_FIFO_TO_CLIENT_A))
    {
        LOGERR("start sender fifo fail");
        return;
    }

    LOGINFO("begin send reset indication msg to client");
    std::string content = std::string("{\"name\":\"") + ACCESS_CMD_RESET_INDICATION + "\"}";
    auto msg = std::make_shared<access_msg_buff>(0, content.c_str(), content.length());
    sender.send_msg_sync(msg);
}

void father_process::send_rsp_msg(unsigned int sequence, int rsp_code, const std::string& cmd)
{
	std::string state = (rsp_code == INS_OK)?"done":"error";
	std::string content = std::string("{\"name\":\"") + cmd + "\", \"state\":\"" + state + "\"}";

	auto msg = std::make_shared<access_msg_buff>(sequence, content.c_str(), content.length());
    if (writer_) writer_->queue_msg(msg);
}
