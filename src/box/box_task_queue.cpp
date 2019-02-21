#include "box_task_queue.h"
#include "common.h"
#include "inslog.h"
#include "file_stitcher_factory.h"
#include "box_msg_parser.h"
#include "ins_util.h"
#include <sstream>

box_task_queue::~box_task_queue()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

box_task_queue::box_task_queue()
{
    th_ = std::thread(&box_task_queue::task, this);
}

void box_task_queue::task()
{
    while (!quit_)
    {
        auto entry = dequeue_task_entry();
        if (entry == nullptr)
        {
            usleep(30*1000);
            continue;
        }

        run_task_entry(entry);
    }

    LOGINFO("task queue finish");
}

void box_task_queue::run_task_entry(const std::shared_ptr<task_entry>& entry)
{
    LOGINFO("task:%s running", entry->id.c_str());
    box_msg_parser::print_task_entry(entry);

    int ret;
    std::shared_ptr<file_stitcher> stitcher;

    do {
        auto pos = entry->option.output.file.rfind("/");
        std::string url = entry->option.output.file.substr(0, pos);
        auto space = ins_util::disk_available_size(url.c_str());
        if (space <= INS_MIN_SPACE_MB) 
        {
            LOGERR("no disk space left:%u", space);
            ret = INS_ERR_NO_STORAGE_SPACE;
            break;
        }

        stitcher = file_stitcher_factory::create_stitcher(entry->option.input.type);
        if (stitcher == nullptr) 
        {
            ret = INS_ERR; break;
        }

        ret = stitcher->open(entry->option);
        BREAK_IF_NOT_OK(ret);

        while (!quit_ && !force_quit_)
        {
            if (stitcher->try_join(r_progress_))
            {
                ret = stitcher->get_stitching_result(); break;
            }
            else
            {
                usleep(30*1000); continue;
            }
        }
    } while (0);

    mtx_.lock();
    runnig_task_id_ = "";//要放到callback前
    mtx_.unlock();

    //强制退出外层已经处理了状态，不用回调task_entry_result_cb_
    if (!quit_) task_entry_result_cb_(entry->id, ret);
    force_quit_ = false;
    
    stitcher = nullptr;

    if (entry->option.input.type == INS_VIDEO_TYPE && ret == INS_OK)
    {
        std::vector<std::string> v_url;
        v_url.push_back(entry->option.output.file);
    }

    LOGINFO("task:%s finish", entry->id.c_str());
}

void box_task_queue::queue_task_entry(const std::shared_ptr<task_entry>& entry)
{
    std::lock_guard<std::mutex> lock(mtx_);
    task_list_.push_back(entry);
    LOGINFO("queue add task:%s, current cnt:%d", entry->id.c_str(), task_list_.size());
}

std::shared_ptr<task_entry>box_task_queue::dequeue_task_entry()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (task_list_.empty()) return nullptr;
    auto entry = task_list_.front();
    task_list_.pop_front();

    runnig_task_id_ = entry->id;
    r_progress_ = 0;

    return entry;
}

int box_task_queue::update_task_entry(const std::shared_ptr<task_entry>& entry)
{
    std::lock_guard<std::mutex> lock(mtx_);
    
    if (runnig_task_id_ == entry->id) return INS_ERR_TASK_IS_RUNNING;

    for (auto it = task_list_.begin(); it != task_list_.end(); it++)
    {
        if ((*it)->id == entry->id)
        {
            (*it)->option = entry->option;
            LOGINFO("queue update task:%s", entry->id.c_str());
            break;
        }
    }

    return INS_OK;
}

int box_task_queue::delete_task_entry(std::string id)
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (runnig_task_id_ == id)
    {
        LOGERR("task:%s is runnig, cann't be deleted", id.c_str());
        return INS_ERR_TASK_IS_RUNNING;
    }

    for (auto it = task_list_.begin(); it != task_list_.end(); it++)
    {
        if ((*it)->id == id)
        {
            LOGINFO("queue delete task:%s, current cnt:%d", id.c_str(), task_list_.size());
            task_list_.erase(it);
            return INS_OK;
        }
    }

    return INS_ERR_TASK_NOT_FOND;
}

int box_task_queue::delete_task_entry(const std::vector<std::string>& id, std::string& running_task_id)
{
    int count = 0;
    std::lock_guard<std::mutex> lock(mtx_);

    for (unsigned i = 0; i < id.size(); i++)
    {
        for (auto it = task_list_.begin(); it != task_list_.end(); it++)
        {
            if ((*it)->id == id[i])
            {
                LOGINFO("queue delete task:%s, current cnt:%d", id[i].c_str(), task_list_.size());
                task_list_.erase(it);
                count++;
                break;
            }
        }

        if (runnig_task_id_ == id[i])
        {
            LOGINFO("queue delete runing task");
            force_quit_ = true;
            running_task_id = runnig_task_id_;
        }
    }

    return count;
}

