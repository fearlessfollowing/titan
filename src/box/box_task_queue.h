#ifndef _BOX_TASK_QUEUE_H_
#define _BOX_TASK_QUEUE_H_

#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <list>
#include "box_task_option.h"

class box_task_queue
{
public:
    box_task_queue();
    ~box_task_queue();
    void queue_task_entry(const std::shared_ptr<task_entry>& entry);
    int update_task_entry(const std::shared_ptr<task_entry>& entry);
    int delete_task_entry(std::string id); //不会删除运行中的
    int delete_task_entry(const std::vector<std::string>& id, std::string& running_task_id); //会删除运行中的
    int task_entry_count()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (runnig_task_id_ == "")
        {
            return task_list_.size();
        }
        else
        {
            return task_list_.size() + 1;
        }
    }
    void set_task_entry_result_cb(std::function<void(std::string,int)>& cb)
    {
        task_entry_result_cb_ = cb;
    }
    std::string get_runing_task_state(double& progress) 
    { 
        if (runnig_task_id_ == "") 
        {
            progress = 0;
        }
        else
        {
            progress = r_progress_;
        }
        return runnig_task_id_; 
    }

private:
    void task();
    void run_task_entry(const std::shared_ptr<task_entry>& entry);
    std::shared_ptr<task_entry>dequeue_task_entry();
    std::function<void(std::string,int)> task_entry_result_cb_;
    std::list<std::shared_ptr<task_entry>> task_list_;
    std::thread th_;
    std::mutex mtx_;
    bool quit_ = false;
    double r_progress_ = 0;
    bool force_quit_ = false;
    std::string runnig_task_id_; //要lock mtx_
};

#endif