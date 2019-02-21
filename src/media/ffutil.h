
#ifndef _FF_UTIL_H_
#define _FF_UTIL_H_

#include <string>

#define FFERR2STR(err) (ffutil::err2str(err).c_str())

class ffutil
{
public:
    static void init();
    static std::string err2str(int err);
};

#endif