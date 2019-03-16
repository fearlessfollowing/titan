#ifndef _INS_CLOCK_H_
#define _INS_CLOCK_H_

#include <chrono>

using namespace std::chrono;

class ins_clock {
public:
    ins_clock() {
        start_ = steady_clock::now();
    };

    double elapse() {
        auto end = steady_clock::now();
        auto elapse = duration_cast<duration<double>>(end - start_);
        return elapse.count();
    };

    double elapse_and_reset() {
        auto end = steady_clock::now();
        auto elapse = duration_cast<duration<double>>(end - start_);
        start_ = end;
        return elapse.count();
    };

private:
    steady_clock::time_point start_;
};

#endif