#ifndef _EXIF_PARSER_H_
#define _EXIF_PARSER_H_

#include <string>
#include "metadata.h"

class exif_parser
{
public:
    int parse(std::string filename, jpeg_metadata& metadata);

    // bool latitude_north_ = true; 
    // bool longitude_east_ = true;
    // bool altitude_above_ = true;
    // jpeg_metadata metadata_;
};

#endif