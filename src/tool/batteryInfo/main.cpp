#include "ins_battery.h"
#include <iostream>
//#include "inslog.h"
//#include <signal.h>
#include <unistd.h>
//#include<sys/select.h>

// bool _quit = false;

// void sig_handle(int32_t sig)
// {
// 	std::cout << "recv sig:" << sig << std::endl;
//     _quit = true;
// }

int main()
{
    // struct sigaction sig_action;
    // sig_action.sa_handler = sig_handle;
    // sigaction(SIGINT, &sig_action, nullptr);
    //sigaction(SIGQUIT, &sig_action, nullptr);
    //sigaction(SIGTERM, &sig_action, nullptr);

    //ins_log::init("/tmp", "log");
 
    double temp;
    while (1)
    {
        ins_battery battery;
        if (battery.read_temp(temp))
        {
            std::cout << "i2c read fail" << std::endl;
        }
        else
        {
            std::cout << "temperature:" << temp << std::endl;
        }
        
        usleep(3*1000*1000);
    }

    return 0;
}