#ifndef _BOX_TASK_MGR_H_
#define _BOX_TASK_MGR_H_

#include "leveldb/db.h"
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <memory>
#include <condition_variable>
#include "json_obj.h"

class box_task_queue;

class box_task_mgr
{
public:
    ~box_task_mgr();
    int open();
    int start_task_list();
    int stop_task_list(bool quit = false);
    int query_task_list(std::string& lists, bool& b_list_start);
    std::map<std::string,int> add_task(const char* msg);
    std::map<std::string,int> update_task(const char* msg);
    std::map<std::string,int> delete_task(const char* msg);
    std::map<std::string,std::string> query_task(const char*, std::map<std::string,int>& option);
    std::map<std::string,int> start_task(const char* msg);
    std::map<std::string,int> stop_task(const char* msg); 

private:
    int db_updata_task(const std::string id, const std::string& state, json_obj* param = nullptr, int error = 0);
    int db_query_task(const std::string id, std::string& state, int* error = nullptr, std::string* param = nullptr);
    int db_open_db();
    void db_close_db();
    void do_task_finish(std::string id, int result);
    void update_task_stats(int total_cnt, unsigned suc_cnt = 0, unsigned fail_cnt = 0, bool b_timer = false);
    void stats_report_task();
    void send_stats_msg(unsigned total_cnt, unsigned suc_cnt, unsigned fail_cnt, bool b_task_over, double progress);

    leveldb::DB* db_ = nullptr;
    std::shared_ptr<box_task_queue> task_queue_;
    std::thread th_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::mutex mtx_cv_;
    bool task_list_quit_ = false;
    unsigned total_cnt_ = 0;
    unsigned successful_cnt_ = 0;
    unsigned failing_cnt_ = 0;
};

#endif