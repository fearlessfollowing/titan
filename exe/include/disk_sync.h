#ifndef _DISK_SYNC_H_
#define _DISK_SYNC_H_

enum {
	DISK_SYNC_ONCE,
	DISK_SYNC_PERIOD,
	DISK_SYNC_MAX
};


/*
 * 磁盘同步对象,进程内唯一
 */
class disk_sync {
public:

	static disk_sync* create() {
        if (!disk_sync_mgr_) 
			disk_sync_mgr_ = new disk_sync();
        return disk_sync_mgr_;
    };

    static void destroy() {
        if (disk_sync_mgr_) {
            delete disk_sync_mgr_;
            disk_sync_mgr_ = nullptr;
        }
    };

	static disk_sync* get() {
        assert(disk_sync_mgr_);
        return disk_sync_mgr_;
    };		

	int start_disk_sync(int type, std::string path);
	void wait_disk_sync_complete();


private:
	int					sync_type_;			/* 磁盘同步类型: 单次同步或周期同步 */
	std::thread			sync_th_;			/* 同步线程 */
	int					period_t;			/* 周期同步的周期值 */
	bool				is_sync = false;			/* 同步线程是否正在运行中 */
	bool				sync_quit_;
	std::string			sync_path_;
	std::mutex			sync_lock_;
	
    static  disk_sync*	disk_sync_mgr_;

	void disk_sync_task();
	
};

#endif /* _DISK_SYNC_H_ */


