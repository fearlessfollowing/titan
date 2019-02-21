
#ifndef _UPLOAD_CALITRATION_FILE_
#define _UPLOAD_CALITRATION_FILE_

#include <string>

class upload_cal_file
{
public:
    static int upload_file(std::string file_name, std::string uuid, std::string sensor_id, int* maxvalue);
};

#endif