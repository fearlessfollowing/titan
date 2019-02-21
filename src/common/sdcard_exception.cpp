
#include <sstream>
#include <map>
#include <vector>
#include "ins_util.h"
#include "inslog.h"
#include "sdcard_exception.h"

static int do_remount(std::string mnt_path, std::string dev_path)
{   
    //lsof
    auto cmd = std::string("lsof | grep ") + mnt_path + " | cut -d ' ' -f 1";
    auto process_s =  ins_util::system_call_output(cmd);
    LOGINFO("process:\n%s", process_s.c_str());
    std::vector<std::string> v_process;
    ins_util::split(process_s, v_process, "\n");
    for (auto& process : v_process)
    {
        if (process != "camerad" && process != "mountufsd" && process != "sdcard" 
            && process != "lsof" && process != "sh" && process != "cut" && process != "grep")
        {
            LOGINFO("kill process:%s", process.c_str());
            cmd = "pkill ";
            cmd += process;
            ins_util::system_call(cmd);
        }
    }

    cmd = std::string("lsof | grep ") + mnt_path + " | grep camerad | cut -d ' ' -f 14";
    auto fd_s =  ins_util::system_call_output(cmd);
    LOGINFO("fd:\n%s", fd_s.c_str());
    std::vector<std::string> v_fd;
    ins_util::split(fd_s, v_fd, "\n");
    for (auto& fd : v_fd)
    {
        int i_fd = atoi(fd.c_str());
        LOGINFO("close fd:%s %d", fd.c_str(), i_fd);
        ::close(i_fd);
    }

    //umount
    cmd = std::string("umount ") + mnt_path;
    int retry = 10;
    bool success = false;
    while (retry--)
    {
        usleep(500*1000);

        // auto out_s = system_call_output("df");
        // LOGINFO("df:%s", out_s.c_str());
        // if (out_s.find(mnt_path) == std::string::npos)
        // {
        //     LOGINFO("df cann't find mount path:%s, needn't mount", mnt_path.c_str());
        //     return -1;
        // }

        int ret = ins_util::system_call(cmd);
        if (!ret) 
        {
            success = true; break;
        }
    }
    if (success)
    {
        usleep(100*1000);
        LOGINFO("------umount success");
    }
    else
    {
        LOGERR("------umount timeout");
        return -1;
    }

    //mount
    cmd = std::string("mountufsd ") + dev_path + " " + mnt_path + " -o uid=1023,gid=1023,fmask=0700,dmask=0700,force";
    retry = 10;
    while (retry--)
    {
        usleep(500*1000);

        // auto out_s = system_call_output("ls /dev/block/vold");
        // LOGINFO("dev:%s", out_s.c_str());
        // if (out_s.find(dev_path.substr(strlen("/dev/block/vold/"), dev_path.length())) == std::string::npos)
        // {
        //     LOGINFO("cann't find dev:%s, needn't mount", dev_path.c_str());
        //     return -1;
        // }

        int ret = ins_util::system_call(cmd);
        if (!ret) 
        {
            success = true; break;
        }
    }

    if (success)
    {
        usleep(100*1000);
        LOGINFO("------mount success");
    }
    else
    {
        LOGERR("------mount timeout");
        return -1;
    }
	
	return 0;
}

int sdcard_exception::remount(const std::vector<std::string>& v_url)
{
    std::map<std::string, int> map_mnt_path;

    for (auto& url : v_url)
    {
        //mount path
        if (url.find("/mnt/media_rw/") != 0) 
        {
            LOGERR("url:%s not begin with /mnt/media_rw/", url.c_str());
            return -1;
        }

        auto pos = url.find("/", strlen("/mnt/media_rw/"));
        if (pos == std::string::npos)
        {
            LOGERR("url:%s cann't find mount path", url.c_str());
            return -1;
        }

        std::string mnt_path = url.substr(0, pos);

        if (map_mnt_path.find(mnt_path) != map_mnt_path.end()) 
        {
            LOGINFO("repeat mount path:%s", mnt_path.c_str());
            continue;
        }

        LOGINFO("mount path:%s", mnt_path.c_str());
        map_mnt_path.insert(std::make_pair(mnt_path,0));

        //dev path
        std::string cmd = std::string("mount | grep ")  + mnt_path + " | cut -d ' ' -f 1";
        std::string dev_path = ins_util::system_call_output(cmd);
        dev_path = dev_path.substr(0, dev_path.length() - 1);
        LOGINFO("device path:%s", dev_path.c_str());

        int ret = do_remount(mnt_path, dev_path);
        if (ret) return ret;
    }

    return 0;
}

