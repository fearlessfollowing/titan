#include "disk_sync.h"


void disk_sync::disk_sync_task()
{
	if (sync_type_ == DISK_SYNC_ONCE) {
		LOGINFO("disk sync task: type is DISK_SYNC_ONCE");
		std::stringstream ss;
		ss << "sync -f " << sync_path_;
		system(ss.str().c_str());
	} else if (sync_type_ == DISK_SYNC_PERIOD) {
		LOGINFO("disk sync task: type is DISK_SYNC_PERIOD");

		if (period_t == 0) {
			period_t = 3;
		}

		while (!sync_quit_) {			
			sleep(period_t);			
			std::stringstream ss;
			ss << "sync -f " << sync_path_;
			system(ss.str().c_str());
		}
	} else {
		LOGERR("invalid disk sync type!");
	}
	
	is_sync = false;
}



int disk_sync::start_disk_sync(int type, std::string path, int sync_period)
{
    std::lock_guard<std::mutex> lock(sync_lock_);
	if (is_sync) {
		LOGERR("sync task is running, return");
		return -1;
	}

	sync_type_ = type;
	if (path == "") {
		path = "/mnt/SD0";
	}
	
	sync_path_ = path;
	sync_quit_ = false;
	is_sync	= true;
	period_t = sync_period;
	sync_th_ = std::thread(&disk_sync::disk_sync_task, this);
}

void disk_sync::wait_disk_sync_complete()
{
	if (sync_type_ == DISK_SYNC_ONCE) {
		if (sync_th_.joinable()) 
			sync_th_.join();
	} else if (sync_type_ == DISK_SYNC_PERIOD) {
		sync_quit_ = true;
		if (sync_th_.joinable()) 
			sync_th_.join();
	}
}


