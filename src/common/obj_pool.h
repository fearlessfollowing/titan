#ifndef _OBJ_POOL_H_
#define _OBJ_POOL_H_

#include <stdint.h>
#include <memory>
#include <deque>
#include <mutex>
#include <string>

template <class T>
void obj_pool_deleter(T* obj)
{
    if(!obj) return;
    obj->get_pool()->put_obj(obj);
}

template <class T>
class obj_pool : public std::enable_shared_from_this<obj_pool<T>>
{
public:
    obj_pool(int32_t num, std::string name) 
        :num_total_(num)
        ,name_(name)
    {
    };

    void alloc(uint32_t num)
    {
        for (uint32_t i = 0; i < num; i++)
        {
            num_cur_++;
            auto obj = std::shared_ptr<T>(new T, obj_pool_deleter<T>);
            obj->set_pool(obj_pool<T>::shared_from_this());
        }
    }

    ~obj_pool()
    {
        //printf("-----%s destroy\n", name_.c_str());
        for (auto it : queue_)
        {
            delete it;
        }

        queue_.clear();
    }

    std::shared_ptr<T> pop()
    {
        mtx_.lock();
        if (queue_.empty())
        {
            if (name_ != "gps") printf("-------%s no empty num:%d\n", name_.c_str(), num_cur_);
            mtx_.unlock();
            if (num_cur_ < num_total_ || num_total_ == -1)
            {
                alloc(16); //不足的时候一次分配多个
            }
            else
            {
                //LOGERR("%s no buff left", name_.c_str());
                return nullptr;
            }
        }
        else 
        {
            mtx_.unlock();
        }
    
        std::lock_guard<std::mutex> lock(mtx_);
        auto obj = queue_.front();
        obj->set_pool(obj_pool<T>::shared_from_this());
        queue_.pop_front();
        //printf("get obj queue size:%lu\n", queue_.size());
        return std::shared_ptr<T>(obj, obj_pool_deleter<T>);
    }

    void put_obj(T* obj)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        obj->set_pool(nullptr);
        obj->clean();
        queue_.push_back(obj);
        //printf("put obj queue size:%lu\n", queue_.size());
    }

private:
    std::deque<T*> queue_;
    std::mutex mtx_; 
    int32_t num_cur_ = 0;
    int32_t num_total_ = 0;
    std::string name_;
};

#endif