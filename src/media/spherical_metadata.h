
#ifndef _SPHERICAL_METADATA_H_
#define _SPHERICAL_METADATA_H_

#include <string>
#include "metadata.h"

class spherical_metadata
{
public:
    //static std::string exif_gpano(int width , int height, int map_type);
    static std::string video_spherical(std::string mode);
    static std::string gps_xmp(const ins_gps_data& data);
};

#endif