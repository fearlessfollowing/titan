#ifndef _SDCARD_EXCEPTION_H_
#define _SDCARD_EXCEPTION_H_

#include <string>

class sdcard_exception
{
public:
    static int remount(const std::vector<std::string>& v_url);
};

#endif