
#include <memory>
#include <iostream>
#include "gps_dev.h"
#include "inslog.h"
#include "ins_signal.h"

int main()
{
    ins_log::init("/tmp", "log");
    auto dev = std::make_unique<gps_dev>();
    dev->open();

    wait_signal_terminate();

    dev = nullptr;

    return 0;
}