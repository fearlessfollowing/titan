
#include "cam_manager.h"
#include "inslog.h"
#include "common.h"
#include <fstream>
#include <unistd.h>

#define VERSION_FILE "version.txt"
#define FW_FILE_1 "sys_dsp_rom.devfw"
#define FW_FILE_5 "sys_dsp_rom_5.devfw"

std::string read_version_from_file(std::string path)
{
	std::string url = path + "/" + VERSION_FILE;
	LOGINFO("version file:%s", url.c_str());
	std::ifstream file(url);
	if (!file.is_open()) 
	{
		LOGERR("fw vesion file open fail");
		return "";
	}
	std::string version;
	getline(file, version);
	return version;
}

static void split(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;

	while(std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2-pos1));
		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}

	if(pos1 != s.length()) v.push_back(s.substr(pos1));
}

static int parse_module_hw_version(const std::string& version)
{
	if (version[0] == 'V')
	{
		return 1;
	}

	std::vector<std::string> v;
	split(version, v, ".");

	if (v.size() <= 0) 
	{
		LOGERR("invalid version:%s", version.c_str());
		return -1;
	}

	return atoi(v[0].c_str());
}

static bool check_all_hw_version_same(std::vector<std::string>& version, int& hw_version)
{
	hw_version = -1;
	for (unsigned int i = 0; i < version.size(); i++)
	{
		auto ret = parse_module_hw_version(version[i]);
		if (ret < 0) return false;
		if (hw_version != -1 && hw_version != ret)
		{
			LOGERR("hw version:%d %d not same", hw_version, ret);
			return false;
		}
		else
		{
			hw_version = ret;
		}
	}

	return true;
}

static bool check_all_version_same(std::vector<std::string>& cur_version, std::string& new_version)
{	
	for (unsigned int i = 0; i < cur_version.size(); i++)
	{
		std::string version = cur_version[i];
		auto pos = version.find(".", 0);
		if (pos == std::string::npos) return false;
		version.erase(0, pos+1);
		if (version != new_version) return false;
	}

	return true;
}

static int upgrade(std::string path, std::string version)
{
	int ret;
	auto camera = std::make_shared<cam_manager>();
	ret = camera->open_all_cam();
	RETURN_IF_NOT_OK(ret);

	std::vector<std::string> cur_version;
	ret = camera->get_all_version(cur_version);
	RETURN_IF_NOT_OK(ret);

	//版本一致不用升级
	if (check_all_version_same(cur_version, version))
	{
		LOGINFO("module version now same with req version:%s", version.c_str());
		return 1;
	}

	//硬件版本是否一致
	int hw_version = 0;
	if (!check_all_hw_version_same(cur_version, hw_version))
	{
		LOGERR("module hw version not same");
		return INS_ERR;
	}

	std::string fw_filename = path + "/" + FW_FILE_1;

	LOGINFO("hw version:%d fw file:%s", hw_version, fw_filename.c_str());
	
	LOGINFO("begin to upgrade module");

	ret = camera->upgrade(fw_filename, version);
	if (ret == INS_OK)
	{
		LOGINFO("upgrade module success");
	}
	else
	{
		LOGERR("upgrade module fail:%d", ret);
	}

	return ret;
}

static int process(std::string path)
{
	//std::string fw_url = path + "/" + FW_FILE;
	std::string fw_version = read_version_from_file(path);
	if (fw_version == "")
	{
		LOGERR("cann't get fw version from file");
		return INS_ERR;
	}

	LOGINFO("fw path:%s fw version:%s", path.c_str(), fw_version.c_str());

	// cam_manager::power_init();
	// usleep(50*1000);
	cam_manager::power_off_all();
	usleep(50*1000);
	cam_manager::power_on_all();

	int ret = upgrade(path, fw_version);

	cam_manager::power_off_all();

	return ret;
}

int main(int argc, char* argv[])
{
	ins_log::init(INS_LOG_PATH, "log");
	
	LOGINFO("upgrade version:1.0.1");

	int32_t option;
	std::string fw_path;
	while ((option = getopt(argc, argv, "p:n:")) != -1) 
	{
		switch (option) 
		{
			case 'p':
				fw_path = optarg;
				break;
			case 'n':
				//if (atoi(optarg) == 8) INS_CAM_NUM = 8;
				break;
			default: 
				break;
		}
	}

	int ret = 0;
	if (fw_path == "")
	{
		LOGERR("please input fw path");
		ret = -1;
	}
	else
	{
		ret = process(std::string(fw_path));
	}

	LOGINFO("upgrade result:%d", ret);

	return ret;
}

