#ifndef _INS_BASE64_H_
#define _INS_BASE64_H_

#include <memory>
#include "insbuff.h"

class ins_base64
{
public:
    static std::shared_ptr<insbuff> decode(const std::string& in);
    static std::string encode(std::shared_ptr<insbuff>& in);
};

#endif