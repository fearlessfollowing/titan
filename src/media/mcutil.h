
#ifndef _MC_UTIL_H_
#define _MC_UTIL_H_

#include <string>
#include <media/stagefright/foundation/ABuffer.h>

class mcutil
{
public:
    static void threadpool_init();
    static android::sp<android::ABuffer> base64_decode(const char* s);
    static std::string base64_encode(const unsigned char* data, unsigned int size);
};

#endif