#ifndef _FILE_STITCHER_FACTORY_H_
#define _FILE_STITCHER_FACTORY_H_

#include "file_stitcher.h"
#include <memory>

class file_stitcher_factory
{
public:
    static std::shared_ptr<file_stitcher> create_stitcher(std::string type);
};

#endif