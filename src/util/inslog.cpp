

#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sstream> 
#include <vector>
#include <sys/statfs.h>
#include "inslog.h"
 #include <dirent.h>
 #include <unistd.h>
 #include <algorithm>

#define LOG_SIZE 20

std::shared_ptr<ins_log> ins_log::logger_ = nullptr;

ins_log::ins_log(std::string path, std::string name, bool stdout)
{	
	file_size_ = LOG_SIZE *1024 * 1024;  /* M uint */
	
	path_ = path;
	name_ = name;
	stdout_ = stdout;

	if (access(path_.c_str(), 0))
	{
		std::string cmd = "mkdir -p " + path_;
        system(cmd.c_str());
	}

	std::string file_name = path_ +  "/" + name_;
	
	fp_ = fopen(file_name.c_str(), "a+");
	if (!fp_)
	{
		printf("open log file fail\n");
	}
}

ins_log::~ins_log()
{	
	if (fp_)
	{
		fclose(fp_);
		fp_ = nullptr;
	}

	//printf("-----pid:%d log destroy\n", getpid());
}

void ins_log::log(unsigned char level, const char* file, int line, const char* fmt, ...)
{	
	static char LogLevelString[][10] = {"ERROR", "INFO ", "DEBUG"};

	if (level > level_)
	{
		return;
	}

	va_list arg;

	std::lock_guard<std::mutex> lock(mtx_);
	
	/* to stdout */
	struct timeval tv;  
	gettimeofday(&tv, nullptr);
	struct tm *tmt = localtime(&tv.tv_sec);

	if (stdout_)
	{
		printf("%04d-%02d-%02d %02d:%02d:%02d:%06ld %s ", 
			tmt->tm_year + 1900,
			tmt->tm_mon + 1,
			tmt->tm_mday,
			tmt->tm_hour,
			tmt->tm_min,
			tmt->tm_sec,
			tv.tv_usec,
			LogLevelString[level]);

		va_start(arg, fmt);	
		vprintf(fmt, arg);
		va_end(arg);
		
		printf(" %s:%d\n", file, line);
	}

	/* to log file */
	if (fp_)
	{
		fprintf(fp_, "%04d-%02d-%02d %02d:%02d:%02d:%06ld %s ",
			tmt->tm_year + 1900,
			tmt->tm_mon + 1,
			tmt->tm_mday,
			tmt->tm_hour,
			tmt->tm_min,
			tmt->tm_sec,
			tv.tv_usec,
			LogLevelString[level]);

		/* log content */
		va_start(arg, fmt);
		vfprintf(fp_, fmt, arg);
		va_end(arg);

		/* log file */
		fprintf(fp_, " %s:%d\n", file, line);

		fflush(fp_);

		log_file_change();
	}
}

void ins_log::log_file_change()
{
	std::string origfile = path_ + "/" + name_;

	struct stat statbuff;  
	if (stat(origfile.c_str(), &statbuff) < 0)
	{  
		return;  
	}

	if (statbuff.st_size  < (int)file_size_)
	{
		return;
	}

	fclose(fp_);
	fp_ = nullptr;

	time_t timer = time(nullptr);
	struct tm *tmt = localtime(&timer);

	std::stringstream newfile;

	newfile << path_ 
			<< "/"
			<< name_ << "_"
			<< tmt->tm_year + 1900 << "_"
			<< tmt->tm_mon + 1 << "_"
			<< tmt->tm_mday << "_" 
			<< tmt->tm_hour << "_"
			<< tmt->tm_min << "_"
			<< tmt->tm_sec;

	rename(origfile.c_str(), newfile.str().c_str());

	fp_ = fopen(origfile.c_str(), "a+");
	if (!fp_)
	{
		printf("open log file fail\n");
	}

	erase_log_file();
}

void ins_log::erase_log_file()
{
	if (disk_available_size(path_.c_str()) > 1024) return;

	DIR* dir = opendir(path_.c_str());
	if (dir == nullptr)
	{
		printf("open dir fail\n");
		return;
	}

	std::string prefix = name_ + "_";
	std::vector<std::string> v_e;
	struct dirent *ptr = nullptr;
	while ((ptr = readdir(dir)) != nullptr)
	{
		std::string e = ptr->d_name;
		if (e.find(prefix, 0) == 0)
		{
			v_e.push_back(e);
		}
	}

	closedir(dir);

	if (v_e.empty()) return;

	sort(v_e.begin(), v_e.end());

	for (unsigned int i = 0; (i < v_e.size())&&(i < 2); i++)
	{
		std::string file_name = path_ + "/" + v_e[i];
		remove(file_name.c_str());
		printf("remove log file:%s\n", file_name.c_str());
	}
}

unsigned int ins_log::disk_available_size(const char* path)
{
	struct statfs diskInfo;  
    if (statfs(path, &diskInfo))
    {
        LOGERR("statfs fail");
        return 0;
    }  

    unsigned long long block_size = diskInfo.f_bsize; 
    //unsigned long long total_size = diskInfo.f_blocks*block_size; 
	//unsigned long long free_size = diskInfo.f_bfree*block_size;
    unsigned long long available_size = diskInfo.f_bavail*block_size;   

    return (available_size>>20);
}

