#ifndef _INS_QUEUE_
#define _INS_QUEUE_

#include <mutex>
#include <deque>
#include <queue>
#include <limits.h>

template <class T>
class safe_deque
{
public:
    void push_back(const T& item)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.size() > max_size_) return;
        queue_.push_back(item);
    }

    bool pop_front(T& item)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return false;
        item = queue_.front();
        queue_.pop_front();
        return true;
    }

    void set_max_size(uint32_t size)
    {
        max_size_ = size;
    }

   // T front() //不要频繁访问
    // {
    //     std::lock_guard<std::mutex> lock(mtx_);
    //     return queue_.front();
    // }

    // bool empty() //不要频繁访问
    // {
    //     std::lock_guard<std::mutex> lock(mtx_);
    //     return queue_.empty();
    // }

    uint32_t size()
    {
        return queue_.size();
    }

private:
    std::mutex mtx_;
    std::deque<T> queue_;
    uint32_t max_size_ = UINT_MAX;
};

template <class T>
class safe_queue
{
public:
    void push(const T& item)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.size() > max_size_) return;
        queue_.push(item);
    }

    bool pop(T& item)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (queue_.empty()) return false;
        item = queue_.front();
        queue_.pop();
        return true;
    }

    void set_max_size(uint32_t size)
    {
        max_size_ = size;
    }

    T& front()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.front();
    }

    bool empty() //不要频繁访问
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    uint32_t size() //不加锁
    {
        return queue_.size();
    }

private:
    std::mutex mtx_;
    std::queue<T> queue_;
    uint32_t max_size_ = UINT_MAX;
};

#endif