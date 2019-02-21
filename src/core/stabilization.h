
#ifndef _STABILIZATION_H_
#define _STABILIZATION_H_

#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include "insbuff.h"
#include "ins_mat.h"
#include "gyro_sink.h"

namespace ins
{
    namespace Stabilizer
    {
        class GyroStabilizer;
    }
    class FootageMotionFilter;
}

class stabilization : public gyro_sink
{
public:
    stabilization(int32_t delay = 0);
    ~stabilization();
    uint32_t get_rotation_mat(long long pts, ins_mat_4f& out_mat);
    uint32_t get_cpu_rotation_mat(long long pts, ins_mat_3d& out_mat);
    virtual void queue_gyro(std::shared_ptr<insbuff>& data, int64_t delta_ts) override;

private:
    void task();
    std::shared_ptr<insbuff> deque_gyro();
    std::thread th_;
    std::mutex mtx_;
    std::mutex mtx_s_;
    bool quit_ = false;
    int64_t delta_ts_ = 0;
    int64_t delay_ = 0;
    std::queue<std::shared_ptr<insbuff>> queue_;
    std::unique_ptr<ins::Stabilizer::GyroStabilizer> stabilizer_;
    std::unique_ptr<ins::FootageMotionFilter> motionFilter_;
};

#endif