
#include "box_task_mgr.h"
#include "common.h"
#include "inslog.h"
#include <sstream>
#include "box_task_queue.h"
#include "box_task_option.h"
#include "box_msg_parser.h"
#include <sys/stat.h> 
#include <sstream>
#include "access_msg_center.h"
#include <chrono>
#include <unistd.h>
#include "access_msg_center.h"

#define TASK_STATE_IDLE "idle"
#define TASK_STATE_PENDING "pending"
#define TASK_STATE_RUNING "running"
#define TASK_STATE_FINISH "finish"
#define TASK_STATE_ERROR "error"

using namespace leveldb;

box_task_mgr::~box_task_mgr()
{
    stop_task_list(true);
    db_close_db();
    LOGINFO("task mgr close");
}

int box_task_mgr::open()
{
    return db_open_db();
}

int box_task_mgr::query_task_list(std::string& lists, bool& b_list_start)
{
    double progress = 0;
    std::string runing_task_id;
    if (task_queue_) 
    {
        runing_task_id = task_queue_->get_runing_task_state(progress);
        b_list_start = true;
    }
    else
    {
        b_list_start = false;
    }

    std::stringstream ss;
    Iterator* it = db_->NewIterator(ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) 
    {
        if (ss.str() != "")  ss << ",";
        if (it->key().ToString() == runing_task_id)
        {
            json_obj root(it->value().data());
            root.set_string("state", TASK_STATE_RUNING);
            root.set_double("progress", progress);
            ss << root.to_string();
        }
        else
        {
            ss << it->value().ToString();
        }
    }

    lists = ss.str();
    delete it;

    return INS_OK;
}

int box_task_mgr::stop_task_list(bool quit)
{
    task_list_quit_ = true;
    cv_.notify_all();
    INS_THREAD_JOIN(th_);
    if (!quit) update_task_stats(0, 0, 0); //如果是析构了说明退出拼接机了，就不用发消息了
    task_queue_ = nullptr;//放到update_task_stats后面执行
    return INS_OK;
}

int box_task_mgr::start_task_list()
{
    std::function<void(std::string, int)> cb = [this](std::string id, int result)
    {
        do_task_finish(id, result);
    };

    if (task_queue_) return INS_OK;

    task_list_quit_ = false;
    task_queue_ = std::make_shared<box_task_queue>();
    task_queue_->set_task_entry_result_cb(cb);

    int pending_cnt = 0;
    Iterator* it = db_->NewIterator(ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) 
    {
        json_obj root(it->value().data());
        std::string state;
        root.get_string("state", state);
        if (state == TASK_STATE_PENDING)
        {
            std::string param;
            root.get_string(ACCESS_MSG_PARAM, param);
            if (param == "")
            {
                LOGERR("task:%s no param", it->key().ToString().c_str());
                continue;
            }
            auto entry = std::make_shared<task_entry>();
            box_msg_parser parser; 
            int ret = parser.parse(param, entry);
            if (ret == INS_OK) 
            {
                task_queue_->queue_task_entry(entry);
                pending_cnt++;
            }
            else
            {
                db_updata_task(it->key().ToString(), TASK_STATE_ERROR, nullptr, ret);
            }
        }
    }
    delete it;

    update_task_stats(pending_cnt);

    th_ = std::thread(&box_task_mgr::stats_report_task, this);

    LOGINFO("task list open success, total pending task cnt:%u", pending_cnt);

    return INS_OK;
}

std::map<std::string,int> box_task_mgr::add_task(const char* msg)
{
    std::map<std::string,int> result;
    json_obj root(msg);
    auto v_task = root.get_string_array(ACCESS_MSG_PARAM);

    int task_cnt = 0;
    for (unsigned i = 0; i < v_task.size(); i++)
    {
        auto entry = std::make_shared<task_entry>();
        box_msg_parser parser;
        int ret = parser.parse(v_task[i], entry);
        if (ret != INS_OK) 
        {
            result.insert(std::make_pair(entry->id, ret));
            continue;
        }

        std::string value;
        Status s = db_->Get(ReadOptions(), entry->id, &value);
        if (s.ok()) 
        {
            LOGERR("task:%s already exist", entry->id.c_str());
            result.insert(std::make_pair(entry->id, INS_ERR_TASK_ALREADY_EXIST));
            continue;
        }

        result.insert(std::make_pair(entry->id, INS_OK));

        //add task to queue
        if (entry->state == TASK_STATE_PENDING && task_queue_) 
        {
            task_cnt++;
            task_queue_->queue_task_entry(entry);
        }

        //add task to db

        json_obj param_obj(v_task[i].c_str());
        auto input_obj = param_obj.get_obj("input");
        if (input_obj)
        {
            input_obj->set_string("type", entry->option.input.type.c_str());
        }
        else
        {
            LOGERR("no input obj");
        }

        auto output_obj = param_obj.get_obj("output");
        if (output_obj)
        {
            output_obj->set_int64(ACCESS_MSG_OPT_FILE_SIZE_KB, entry->option.output.file_size);
        }
        else
        {
            LOGERR("no output obj");
        }

        json_obj opt_obj;
        opt_obj.set_string("uuid", entry->id);
        opt_obj.set_string("state", entry->state);
        opt_obj.set_obj(ACCESS_MSG_PARAM, &param_obj);

        s = db_->Put(WriteOptions(), entry->id, opt_obj.to_string());
        if (!s.ok()) 
        {
            LOGERR("db add task:%s fail:%s", entry->id.c_str(), s.ToString().c_str());
        }
        else
        {
            LOGINFO("db add task:%s", entry->id.c_str());
        }
    }

    if (task_cnt) update_task_stats(task_cnt);

    return result;
}

std::map<std::string,int> box_task_mgr::update_task(const char* msg)
{
    std::map<std::string,int> result;
    json_obj root(msg);
    auto v_task = root.get_string_array(ACCESS_MSG_PARAM);

    for (unsigned i = 0; i < v_task.size(); i++)
    {
        //parse task
        auto entry = std::make_shared<task_entry>();
        box_msg_parser parser;
        int ret = parser.parse(v_task[i], entry);
        if (ret != INS_OK) 
        {
            result.insert(std::make_pair(entry->id, ret));
            continue;
        }

        //query state in db
        std::string state;
        ret = db_query_task(entry->id, state);
        if (ret != INS_OK)
        {
            result.insert(std::make_pair(entry->id, ret));
            continue;
        }

        if (state == TASK_STATE_PENDING && task_queue_) //如果是运行队列中的，还要更新队列
        {
            ret = task_queue_->update_task_entry(entry);
            if (ret != INS_OK) 
            {
                result.insert(std::make_pair(entry->id, ret));
                continue;
            }
        }

        result.insert(std::make_pair(entry->id, INS_OK));

        json_obj param_obj(v_task[i].c_str());
        auto input_obj = param_obj.get_obj("input");
        if (input_obj)
        {
            input_obj->set_string("type", entry->option.input.type.c_str());
        }
        else
        {
            LOGERR("no input obj");
        }

        auto output_obj = param_obj.get_obj("output");
        if (output_obj)
        {
            output_obj->set_int64(ACCESS_MSG_OPT_FILE_SIZE_KB, entry->option.output.file_size);
        }
        else
        {
            LOGERR("no output obj");
        }

        db_updata_task(entry->id, state, &param_obj);
    }

    return result;
}

//不可以删除正在运行的任务
std::map<std::string,int> box_task_mgr::delete_task(const char* msg)
{
    std::map<std::string,int> result;
    json_obj root(msg);
    auto v_task_id = root.get_string_array(ACCESS_MSG_PARAM);
    int pending_cnt_del = 0;

    //delete all
    if (!v_task_id.empty() && v_task_id[0] == "-1")
    {
        if (task_queue_ && task_queue_->task_entry_count()) //pending & runing list must emtpy
        {
            LOGINFO("having runing task, cann't delete all");
            result.insert(std::make_pair(v_task_id[0], INS_ERR_TASK_IS_RUNNING));
            return result;
        }
        db_close_db();
        Options options;
        Status s = DestroyDB(INS_TASK_LIST_DB_PATH, options);
        if (!s.ok()) 
        {
            LOGINFO("destroy db fail:%s", s.ToString().c_str());
            result.insert(std::make_pair(v_task_id[0], INS_ERR_TASK_DATABASE_DAMAGE));
            return result;
        }
        db_open_db();
        result.insert(std::make_pair(v_task_id[0], INS_OK));
    }
    else
    {
        for (unsigned i = 0; i < v_task_id.size(); i++)
        {
            if (task_queue_)
            {
                int ret = task_queue_->delete_task_entry(v_task_id[i]);
                if (ret != INS_OK && ret != INS_ERR_TASK_NOT_FOND) 
                {
                    result.insert(std::make_pair(v_task_id[i], ret));
                    continue;
                }
                if (ret == INS_OK) pending_cnt_del++;
            }
            
            Status s = db_->Delete(WriteOptions(), v_task_id[i]);
            if (!s.ok()) 
            {
                result.insert(std::make_pair(v_task_id[i], INS_ERR_TASK_DATABASE_DAMAGE));
                LOGERR("db delete task:%s fail:%s", v_task_id[i].c_str(), s.ToString().c_str());
            } 
            else
            {
                result.insert(std::make_pair(v_task_id[i], INS_OK));
                LOGINFO("db delete task:%s", v_task_id[i].c_str());
            }
        }
    }

    if (pending_cnt_del) update_task_stats(-pending_cnt_del);

    return result;
}

std::map<std::string,std::string> box_task_mgr::query_task(const char* msg, std::map<std::string,int>& option)
{
    std::map<std::string,std::string> result;
    json_obj root(msg);
    auto v_task_id = root.get_string_array(ACCESS_MSG_PARAM);

    std::string runing_task_id;
    double progress = 0;
    if (task_queue_) runing_task_id = task_queue_->get_runing_task_state(progress);

    for (unsigned i = 0; i < v_task_id.size(); i++)
    {
        if (v_task_id[i] == runing_task_id)
        {
            result.insert(std::make_pair(v_task_id[i], TASK_STATE_RUNING));
            option.insert(std::make_pair(v_task_id[i], (unsigned)progress));
        }
        else
        {
            std::string state;
            int error_tmp = INS_OK;
            int ret = db_query_task(v_task_id[i], state, &error_tmp);
            if (ret != INS_OK)
            {
                result.insert(std::make_pair(v_task_id[i], TASK_STATE_ERROR));
                option.insert(std::make_pair(v_task_id[i], INS_ERR_TASK_NOT_FOND));
            }
            else
            {
                result.insert(std::make_pair(v_task_id[i], state));
                option.insert(std::make_pair(v_task_id[i], error_tmp));
            }
        }   
    }

    return result;
}

//pending/runnig task -> ok   else -> pending
std::map<std::string,int> box_task_mgr::start_task(const char* msg)
{
    std::map<std::string,int> result;
    json_obj root(msg);
    auto v_task_id = root.get_string_array(ACCESS_MSG_PARAM);
    int pending_cnt_start = 0;

    for (unsigned i = 0; i < v_task_id.size(); i++)
    {
        std::string param;
        std::string state;
        int ret = db_query_task(v_task_id[i], state, nullptr, &param);
        if (ret != INS_OK)
        {
            result.insert(std::make_pair(v_task_id[i], ret));
            continue;
        }

        if (state == TASK_STATE_PENDING) //已经添加到待允许队列了
        {
            LOGINFO("task:%d already in pending state", v_task_id[i].c_str());
            result.insert(std::make_pair(v_task_id[i], INS_OK));
            continue;
        }

        auto entry = std::make_shared<task_entry>();
        box_msg_parser parser; 
        ret = parser.parse(param, entry);
        result.insert(std::make_pair(v_task_id[i], ret));
        if (ret != INS_OK) 
        {
            db_updata_task(v_task_id[i], TASK_STATE_ERROR, nullptr, ret);
            continue;
        }
        else
        {
            db_updata_task(v_task_id[i], TASK_STATE_PENDING);
        }

        if (task_queue_) task_queue_->queue_task_entry(entry);

        pending_cnt_start++;
    }

    if (pending_cnt_start) update_task_stats(pending_cnt_start);

    return result;
}

//可以停止正在运行的任务
std::map<std::string,int> box_task_mgr::stop_task(const char* msg)
{
    std::map<std::string,int> result;
    json_obj root(msg);
    auto v_task_id = root.get_string_array(ACCESS_MSG_PARAM);
    int cnt = 0;
    std::string running_task_id;

    if (task_queue_) cnt = task_queue_->delete_task_entry(v_task_id, running_task_id);

    for (unsigned i = 0; i < v_task_id.size(); i++)
    {   
        if (running_task_id != v_task_id[i])
        {
            db_updata_task(v_task_id[i], TASK_STATE_IDLE);
        }
        result.insert(std::make_pair(v_task_id[i], INS_OK));
    }

    //runing的被停止当成完成，pending的停止总数-1
    if (cnt) update_task_stats(-cnt, 0, 0); 

    return result;
}

void box_task_mgr::do_task_finish(std::string id, int result)
{
    if (result == INS_OK)
    {
        db_updata_task(id, TASK_STATE_FINISH);
        update_task_stats(0, 1, 0);
    }
    else
    {
        db_updata_task(id, TASK_STATE_ERROR, nullptr, result);
        update_task_stats(0, 0, 1);
    }
}

void box_task_mgr::update_task_stats(int total_cnt, unsigned suc_cnt, unsigned fail_cnt, bool b_timer)
{
    if (!task_queue_) return; //队列没有开始的时候不用统计

    mtx_.lock();
    total_cnt_ += total_cnt;
    successful_cnt_ += suc_cnt;
    failing_cnt_ += fail_cnt;

    bool b_task_over;
    double progress = 0;
    if (!task_queue_->task_entry_count() || task_list_quit_) //退出任务队列的时候作为结束发一次
    {
        b_task_over = true;
    }
    else
    {
        b_task_over = false;
        task_queue_->get_runing_task_state(progress);
    }

    unsigned t_cnt = total_cnt_;
    unsigned s_cnt = successful_cnt_;
    unsigned f_cnt = failing_cnt_;

    mtx_.unlock();

    if (!b_timer || !b_task_over)
    {
        send_stats_msg(t_cnt, s_cnt, f_cnt, b_task_over, progress);    
    }
}

void box_task_mgr::send_stats_msg(unsigned total_cnt, unsigned suc_cnt, unsigned fail_cnt, bool b_task_over, double progress)
{
    json_obj root;
    root.set_int("total_cnt", total_cnt);
    root.set_int("successful_cnt", suc_cnt);
    root.set_int("failing_cnt", fail_cnt);
    root.set_bool("task_over", b_task_over);
    root.set_double("runing_task_progress", progress);

    access_msg_center::send_msg(ACCESS_CMD_S_TASK_STATS_, &root);
}

int box_task_mgr::db_updata_task(const std::string id, const std::string& state, json_obj* param, int error)
{
    std::shared_ptr<json_obj> obj;
    if (param == nullptr)
    {
        std::string value;
        Status s = db_->Get(ReadOptions(), id, &value);
        if (!s.ok()) 
        {
            LOGERR("db query task:%s fail:%s", id.c_str(), s.ToString().c_str());
            return INS_ERR_TASK_NOT_FOND;
        } 
        json_obj root(value.c_str());
        obj = root.get_obj(ACCESS_MSG_PARAM);
        param = obj.get();
    }

    Status s = db_->Delete(WriteOptions(), id);
    if (!s.ok()) 
    {
        LOGERR("db delete task:%s fail:%s", id.c_str(), s.ToString().c_str());
        return INS_ERR;
    }

    json_obj root;
    root.set_string("uuid", id);
    root.set_string("state", state);
    root.set_obj(ACCESS_MSG_PARAM, param);

    if (state == TASK_STATE_ERROR)
    {
        json_obj err_obj;
        err_obj.set_int("code", error);
        err_obj.set_string("description", inserr_to_str(error));
        root.set_obj("error", &err_obj);
    }

    s = db_->Put(WriteOptions(), id, root.to_string());
    if (!s.ok()) 
    {
        LOGERR("db add task:%s fail:%s", id.c_str(), s.ToString().c_str());
        return INS_ERR;
    }

    LOGINFO("db update task:%s state:%s error:%s", id.c_str(), state.c_str(), inserr_to_str(error).c_str());

    return INS_OK;
}

int box_task_mgr::db_query_task(const std::string id, std::string& state, int* error, std::string* param)
{
    std::string value;
    Status s = db_->Get(ReadOptions(), id, &value);
    if (!s.ok()) 
    {
        LOGERR("db query task:%s fail:%s", id.c_str(), s.ToString().c_str());
        return INS_ERR_TASK_NOT_FOND;
    } 
    json_obj root(value.c_str());
    root.get_string("state", state);
    if (state == "") 
    {
        LOGERR("task:%s no state", value.c_str());
        return INS_ERR_TASK_DATABASE_DAMAGE;
    }

    if (error != nullptr)
    {
        int err = 0;
        auto obj = root.get_obj("error");
        if (obj) obj->get_int("code", err);
        *error = err;
    }

    if (param != nullptr)
    {
        std::string param_tmp;
        root.get_string(ACCESS_MSG_PARAM, param_tmp);
        if (param_tmp == "") 
        {
            LOGERR("task:%s no param", value.c_str());
            return INS_ERR_TASK_DATABASE_DAMAGE;
        }
        *param = param_tmp;
    }

    return INS_OK;
}

int box_task_mgr::db_open_db()
{
    if (db_) return INS_OK;

    if (access(INS_TASK_LIST_DB_DIR, 0)) mkdir(INS_TASK_LIST_DB_DIR, 0755);

    Options options;
    options.create_if_missing = true;
    Status s = DB::Open(options, INS_TASK_LIST_DB_PATH, &db_);
    if (!s.ok()) 
    {
        LOGINFO("task list db open fail:%s destroy db", s.ToString().c_str());

        Options opts;
        s = DestroyDB(INS_TASK_LIST_DB_PATH, opts);
        if (!s.ok())
        {
            LOGERR("destroy db fail:%s", s.ToString().c_str());
            return INS_ERR_TASK_DATABASE_DAMAGE;
        }
        s = DB::Open(options, INS_TASK_LIST_DB_PATH, &db_);
        if (!s.ok())
        {
            LOGERR("open db after destroy fail:%s", s.ToString().c_str());
            return INS_ERR_TASK_DATABASE_DAMAGE;
        }
    }

    return INS_OK;
}

void box_task_mgr::db_close_db()
{
    if (db_)
    {
        delete db_;
        db_ = nullptr;
    }
}

void box_task_mgr::stats_report_task()
{
    while (!task_list_quit_)
    {
        std::unique_lock<std::mutex> lock(mtx_cv_);
        cv_.wait_for(lock, std::chrono::seconds(5));
    
        if (task_list_quit_) break;
        
        update_task_stats(0, 0, 0, true);
    }

    LOGINFO("stats report task finish");
}
