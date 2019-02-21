
#include "upload_cal_file.h"
#include "inslog.h"
#include <sstream>
#include "common.h"
#include "json_obj.h"
#include "sys/wait.h"
#include <fstream>

int upload_cal_file::upload_file(std::string file_name, std::string uuid, std::string sensor_id, int* maxvalue)
{
	std::stringstream ss;
	std::string tmp_file = "/tmp/.tmp_cal_rst";

	if (maxvalue != nullptr)
	{
		ss << "python3.5 /usr/local/bin/httpRequest.py uploadSensorData "
		<< uuid << " "
		<< sensor_id << " "
		<< maxvalue[0] << ","
		<< maxvalue[1] << ","
		<< maxvalue[2] << ","
		<< maxvalue[3] << " "
		<< file_name
		<< " > "
		<< tmp_file;
	}
	else
	{
		ss << "python3.5 /usr/local/bin/httpRequest.py uploadSensorData "
		   << uuid << " "
		   << sensor_id << " "
		   << file_name
		   << " > "
		   << tmp_file;
	}

	LOGINFO("-----cmd:%s", ss.str().c_str());

	int status = system(ss.str().c_str());
	if (status == -1 || !WIFEXITED(status))
	{
		LOGERR("------upload fail status:%d %d", status, WIFEXITED(status));
		return INS_ERR;
	}
	std::ifstream file(tmp_file);
    if (!file.is_open())
    {
        LOGERR("file:%s open fail", tmp_file.c_str());
		return INS_ERR;
    }
    std::string line;
    getline(file, line);
	LOGINFO("%s", line.c_str());
	getline(file, line);
	LOGINFO("%s", line.c_str());
	json_obj root(line.c_str());
	int ret = INS_ERR;
	root.get_int("code", ret);
    file.close();
    remove(tmp_file.c_str());
	if (ret)
	{
		LOGERR("upload fail htpp return code:%d", ret);
		return INS_ERR;
	}

	return INS_OK;
}