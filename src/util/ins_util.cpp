
#include <sys/statfs.h>
#include "inslog.h"
#include <fstream>
#include "ins_util.h"
#include "common.h"
#include <sstream>
#include <dirent.h>
#include <sys/wait.h>

//return MByte
int ins_util::disk_available_size(const char* path)
{
	struct statfs diskInfo;  
    if (statfs(path, &diskInfo))
    {
        LOGERR("statfs fail");
        return -1; //代表出错
    }  

    unsigned long long block_size = diskInfo.f_bsize; 
    //unsigned long long total_size = diskInfo.f_blocks*block_size; 
	//unsigned long long free_size = diskInfo.f_bfree*block_size;
    unsigned long long available_size = diskInfo.f_bavail*block_size;   

    //LOGINFO("----space:%u", available_size>>20);

    return (available_size>>20);
}

int ins_util::md5_file(std::string file_name, std::string& md5_value)
{   
    std::string cmd = "md5sum -b " + file_name;
    md5_value = system_call_output(cmd);
    return INS_OK;
}

void ins_util::split(const std::string& s, std::vector<std::string>& v, const std::string& c)
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

void ins_util::sleep_s(long long time)
{
    struct timeval tm;
    tm.tv_sec = time/1000000;
    tm.tv_usec = time%1000000;
    select(0,nullptr,nullptr,nullptr,&tm);
}

void ins_util::sleep_s(long long time, int read_fd)
{
    struct timeval tm;
    tm.tv_sec = time/1000000;
    tm.tv_usec = time%1000000;
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(read_fd,&read_set);
    select(read_fd+1,&read_set,nullptr,nullptr,&tm);
}

std::string ins_util::create_name_by_mode(int mode)
{
	if (mode == INS_MODE_PANORAMA)
	{
		return "pano";
	}
	else
	{
		return "3d";
	}
}

std::shared_ptr<insbuff> ins_util::read_entire_file(std::string file_name)
{
    std::fstream s(file_name, s.binary | s.in);
    if (!s.is_open())
    {
        LOGERR("file:%s open fail", file_name.c_str());
        return nullptr;
    }

    s.seekg(0, std::ios::end);
    int size = (int)s.tellg();
    //LOGINFO("----file size:%d", size);
    s.seekg(0, std::ios::beg);

	auto buff = std::make_shared<insbuff>(size);
    if (!s.read((char*)buff->data(), buff->size()))
    {
        LOGERR("file:%s read fail", file_name.c_str());
        return nullptr;
    }

    return buff;
}

int ins_util::read_file(const std::vector<std::string>& file, std::vector<std::shared_ptr<page_buffer>>& data)
{
    for (unsigned i = 0; i < file.size(); i++)
    {
        std::fstream s(file[i], s.binary | s.in);
        if (!s.is_open())
        {
            LOGERR("file:%s open fail", file[i].c_str());
            return INS_ERR_FILE_OPEN;
        }

        s.seekg(0, std::ios::end);
        int size = (int)s.tellg();
        s.seekg(0, std::ios::beg);

        auto buff = std::make_shared<page_buffer>(size);
        if (!s.read((char*)buff->data(), size))
        {
            LOGERR("file:%s read fail, req size:%d read size:%d", file[i].c_str(), size, s.gcount());
            return INS_ERR_FILE_IO;
        }
        data.push_back(buff);
    }

    return INS_OK;
}

int ins_util::file_cnt_under_dir(std::string filename)
{
    auto dir = opendir(filename.c_str());
    if (dir == nullptr)
    {
        LOGERR("opendir:%s fail", filename.c_str());
        return -1; //代表错误
    }

    int file_cnt = 0;
    struct dirent *ptr;
    while ((ptr = readdir(dir)) != nullptr)
    {
        if (ptr->d_type == DT_REG) file_cnt++;
    }

    closedir(dir);

    return file_cnt;
}

void ins_util::check_create_dir(const std::string& filename)
{
	auto pos = filename.rfind("/");
	if (pos == std::string::npos) return;	

    auto dir = filename.substr(0, pos);
    if (access(dir.c_str(), 0))
    {
        std::string cmd = "mkdir -p " + dir;
        system(cmd.c_str());
    }
}

Arational ins_util::to_real_fps(int framerate)
{
    Arational fps;

	switch (framerate)
	{
		case 15:
            fps.num = 15000;
            fps.den = 1001;
			break;
		case 24:
            fps.num = 24000;
            fps.den = 1001;
			break;
		case 30:
            fps.num = 30000;
            fps.den = 1001;
			break;
		case 60:
            fps.num = 60000;
            fps.den = 1001;
			break;
		case 120:
            fps.num = 120000;
            fps.den = 1001;
			break;
		case 240:
            fps.num = 240000;
            fps.den = 1001;
			break;
		default:
            fps.num = framerate;
            fps.den = 1;
			break;
	}

    return fps;
}

int ins_util::system_call(const std::string& cmd) 
{
    int ret = 0;
    auto old_handle = signal(SIGCHLD, SIG_DFL); //如果SIGCHLD被设置为了SIG_IGN,system函数会返回失败

    int r = system(cmd.c_str());
    if (r == -1) 
    {
        LOGERR("cmd:%s fail 1, err:%d %s", cmd.c_str(), errno, strerror(errno));
        ret = -1;
    } 
    else 
    {
        if (WIFEXITED(r)) 
        {
            if (WEXITSTATUS(r))
            {
                LOGERR("cmd:%s fail 3", cmd.c_str());
                ret = -1;
            }
            else
            {
                ret = 0;
            }
        } 
        else 
        {
            LOGERR("cmd:%s fail 2", cmd.c_str());
            ret = -1;
        }
    }
    
    signal(SIGCHLD, old_handle);

    return ret;
}

std::string ins_util::system_call_output(const std::string& cmd, bool del_rn) 
{
    auto fp = popen(cmd.c_str(), "r");
    if (fp == nullptr)
    {
        LOGERR("cmd:%s popen fail", cmd.c_str());
        return "";
    }

    char buff[1024] = {0};
    auto ret = fread(buff, 1, sizeof(buff), fp);
	fclose(fp);
    if (del_rn) buff[ret-1] = 0;

	return std::string(buff);
}


