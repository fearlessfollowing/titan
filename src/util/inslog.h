
#ifndef _IM_LOG_H_
#define _IM_LOG_H_

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <mutex>

#define IMLOG_LEVEL_ERR  0 
#define IMLOG_LEVEL_INFO 1
#define IMLOG_LEVEL_DBG  2

class ins_log
{
public:
	ins_log(std::string path, std::string name, bool stdout = true);
	~ins_log();
	void log(unsigned char level, const char* file, int line, const char* fmt, ...);
	static void init(std::string path, std::string name, bool stdout = true) { logger_ = std::make_shared<ins_log>(path, name, stdout); };
	static std::shared_ptr<ins_log> logger_;

private:
	void log_file_change();
	void erase_log_file();
	unsigned int disk_available_size(const char* path);
	unsigned int file_size_ = 0;
	unsigned char level_ = IMLOG_LEVEL_INFO;
	FILE* fp_ = nullptr;
	std::string path_;
	std::string name_;
	std::mutex mtx_;
	bool stdout_ = true;
};


#define LOGERR(fmt, ...)  ins_log::logger_->log(IMLOG_LEVEL_ERR,  __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGINFO(fmt, ...) ins_log::logger_->log(IMLOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGDBG(fmt, ...)  ins_log::logger_->log(IMLOG_LEVEL_DBG,  __FILE__, __LINE__, fmt, ##__VA_ARGS__);

#endif

