
#include "cam_manager.h"
#include "usb_camera.h"
#include "inslog.h"
#include "xml_config.h"
#include "ins_util.h"
#include "usb_device.h"
#include "json_obj.h"
#include <fcntl.h>
#include "gps_mgr.h"
#include "camera_info.h"
#include "access_msg_center.h"
#include "system_properties.h"

#define MASTER_PID INS_CAM_NUM		/* Master的PID */

int cam_manager::nv_amba_delta_usec_ = 0;
std::mutex cam_manager::mtx_;


int32_t cam_manager::exception()
{
	for (auto& it : map_cam_) {
		auto ret = it.second->exception();
		RETURN_IF_NOT_OK(ret);	
	}

	return INS_OK;
}


int cam_manager::power_on_all()
{
	LOGINFO("power on module");

	system("power_manager power_on");	/* 给所有的模组上电 */
	property_set("module.power", "on");	/* 设置属性"module.power" = "on" */

	usb_device::create();	/* 构建一个usb_device对象(实现USB传输 libusb) */

	return INS_OK;
}

int cam_manager::power_off_all()
{
	LOGINFO("power off module");
	system("power_manager power_off");
	property_set("module.power", "off");

	while (usb_device::get() && !usb_device::get()->is_all_close()) {
		usleep(50*1000);
		system("power_manager power_off");	/* 调用一次power_off,usb可能掉不了电,会卡死在这,故每次循环都调用一次 */
	}

	usb_device::destroy();
	return INS_OK;
}

cam_manager::~cam_manager() 
{
	LOGINFO("cam_manager destroy begin");
	stop_sync_time();
	stop_all_timelapse_ex();
	close_all_cam(); 
	LOGINFO("cam_manager destroy");
};

void cam_manager::stop_sync_time()
{
	sync_quit_ = true;
	INS_THREAD_JOIN(th_time_sync_); 
}

cam_manager::cam_manager()
{		
	pid_.clear();
	for (unsigned int i = 0; i < cam_num_; i++) {
		pid_.push_back(cam_num_ - i);
	}

	for (unsigned int i = 0; i < pid_.size(); i++) {
		if (pid_[i] == MASTER_PID) {
			master_index_ = i;
			break;
		}
	}
}


bool cam_manager::is_all_open()
{
	return usb_device::get()->is_all_open();
}

bool cam_manager::is_all_close()
{
	return usb_device::get()->is_all_close();
}

int cam_manager::open_all_cam()
{
	std::vector<unsigned int> index;
	for (unsigned int i = 0; i < cam_num_; i++) {
		index.push_back(i);
	}

	return open_cam(index);
}

int cam_manager::open_cam(std::vector<unsigned int>& index)
{
	int ret = do_open_cam(index);
	if (ret != INS_OK) {
		close_all_cam();
		return INS_ERR_CAMERA_OPEN;
	} else {
		return INS_OK;
	}
}

int cam_manager::do_open_cam(std::vector<unsigned int>& index)
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (!map_cam_.empty()) {
		LOGERR("camera have been open");
		return INS_ERR;
	}

	for (unsigned int i = 0; i < index.size(); i++) {
		if (index[i] >= cam_num_) return INS_ERR;
	}

	// wait A12 power on
	bool b_all_open = true;
	int loop_cnt = 400;

	while (--loop_cnt > 0) {
		b_all_open = true;
		for (unsigned int i = 0; i < index.size(); i++) {
			if (!usb_device::get()->is_open(pid_[index[i]])) {
				b_all_open = false; 
				break;
			}
		}

		if (b_all_open) 
			break;
		else 
			usleep(50*1000);
	}

	if (!b_all_open) {
		LOGERR("not all camera open");
		return INS_ERR;
	} else {
		LOGINFO("all camera open");
	}

	usleep(1000*1000); // wait A12 init ...

	/*
	 * 为每个模组创建一个usb_camera对象,加入到map_cam_ map中
	 */
	for (unsigned int i = 0; i < index.size(); i++) {
		auto cam = std::make_shared<usb_camera>(pid_[index[i]], index[i]);
		map_cam_.insert(std::make_pair(index[i], cam));
	}

	/* 获取模组版本号 */
	std::vector<std::string> v_version;
	int ret = get_one_version(master_index_, v_version);
	RETURN_IF_NOT_OK(ret);

	/* 跟新模组的版本号到camera_info中 */
	camera_info::set_m_version(v_version[0]);

	/* 设置N/P制,模组起来默认是P制, 0=PAL 1=NTSC */
	auto flicker = camera_info::get_flicker();
	if (!flicker) 
		set_all_options("flicker", flicker);

	return INS_OK;
}


void cam_manager::close_all_cam()
{
	std::lock_guard<std::mutex> lock(mtx_);

	for (auto& it : map_cam_) {	/* 调用usb_camera的close方法 */
		it.second->close();
	}

	map_cam_.clear();
}



int cam_manager::start_video_rec(
						unsigned int index, 
						const cam_video_param& param, 
						const cam_video_param* sec_param, 
						const std::shared_ptr<cam_video_buff_i>& queue)
{
	std::lock_guard<std::mutex> lock(mtx_);
	
	int ret;
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR;

	ret = it->second->set_camera_time();
	RETURN_IF_NOT_OK(ret);

	ret = it->second->set_video_param(param, sec_param);
	RETURN_IF_NOT_OK(ret);

	auto f = it->second->wait_cmd_over();
	ret = f.get();
	RETURN_IF_NOT_OK(ret);

	reset_base_ref();

	ret = it->second->start_video_rec(queue);
	RETURN_IF_NOT_OK(ret);

	auto f1 = it->second->wait_cmd_over();
	ret = f1.get();
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}



int32_t cam_manager::set_all_video_param(const cam_video_param& param, const cam_video_param* sec_param)
{
	std::lock_guard<std::mutex> lock(mtx_);
	int ret = INS_OK;

	for (auto& it : map_cam_) {
		auto r = it.second->set_video_param(param, sec_param);
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto& it : map_cam_) {
		auto f = it.second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	return ret;
}



int32_t cam_manager::start_all_video_rec(const std::map<int,std::shared_ptr<cam_video_buff_i>>& queue)
{
	std::lock_guard<std::mutex> lock(mtx_);
	int ret = INS_OK;

	/* 由于A12系统时间不准，每次录像前对一次时 */
	for (auto& it : map_cam_) {
		ret = it.second->set_camera_time();
		if (ret != INS_OK) {
			LOGERR("pid:%d set_camera_time fail:%d", pid_[it.first], ret);
			return ret;
		}
	}

	reset_base_ref();

	for (auto& it : map_cam_) {
		std::shared_ptr<cam_video_buff_i> q;
		auto queue_it = queue.find(it.first);
		if (queue_it != queue.end()) q = queue_it->second;
		auto r = it.second->start_video_rec(q);
		if (r != INS_OK && ret != INS_ERR_M_NO_SDCARD && ret != INS_ERR_M_NO_SPACE && ret != INS_ERR_M_STORAGE_IO) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto& it : map_cam_) {
		auto f = it.second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK && ret != INS_ERR_M_NO_SDCARD && ret != INS_ERR_M_NO_SPACE && ret != INS_ERR_M_STORAGE_IO) ret = r;
	}

	RETURN_IF_NOT_OK(ret);

	/*
	 * 录像时启动对时任务
	 */
	sync_quit_ = false;
	th_time_sync_ = std::thread(&cam_manager::time_sync_task, this, true);

	return INS_OK;
}


/*
 * Amba-NV对时
 */
void cam_manager::time_sync_task(bool b_record)
{
	auto it = map_cam_.find(master_index_);
	if (it == map_cam_.end()) {
		LOGERR("cann't find master index:%d in time sync task", master_index_);
		return;
	}

	nv_amba_delta_usec_ = 0;

	int delta_time = 0;
	int cnt = 0;
	
	while (!sync_quit_) {	/* 时间同步任务非退出状态 */

		if (cnt++ < 600) {	/* 600 * 100 * 1000 us = 60000ms = 60s */
			usleep(100*1000);
			continue;
		}

		cnt = 0;	
		
		mtx_.lock();
		int ret = it->second->get_camera_time(delta_time);	/* 获取Amba的Master的系统时间(单位为us) */
		mtx_.unlock();

		if (ret != INS_OK) 	/* 获取失败,进入下一次循环 */
			continue;
		
		nv_amba_delta_usec_ = delta_time;

		unsigned int delay_cnt = 30;
		if (!b_record) 
			delay_cnt = 2;

		unsigned int sequence = it->second->get_sequence() + delay_cnt; //delay 

		for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
			it->second->set_delta_time(sequence, delta_time);
		}
	}

	nv_amba_delta_usec_ = 0;
}



int cam_manager::stop_video_rec(unsigned int index)
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR;

	int ret = it->second->stop_video_rec();
	RETURN_IF_NOT_OK(ret);

	auto f = it->second->wait_cmd_over();
	ret = f.get();
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int cam_manager::stop_all_video_rec()
{
	std::lock_guard<std::mutex> lock(mtx_);

	sync_quit_ = true;
	INS_THREAD_JOIN(th_time_sync_);

	int ret = INS_OK;
	for (auto& it : map_cam_) {
		auto r = it.second->stop_video_rec();
		if (r != INS_OK) ret = r;
	}

	std::string s_unspeed_pid;
	std::map<uint32_t, std::future<int32_t>> map_f;
	for (auto& it : map_cam_) {
		auto f = it.second->wait_cmd_over();
		map_f.insert(std::make_pair(it.first, std::move(f)));
	}

	for (auto& it : map_f) {
		auto r = it.second.get();
		if (r != INS_OK) ret = r;
		if (r == INS_ERR_M_UNSPEED_STORAGE) {
			if (s_unspeed_pid != "") s_unspeed_pid += "_";
			s_unspeed_pid += std::to_string(pid_[it.first]);
		}
	}

	if (s_unspeed_pid != "") {
		property_set("module.unspeed", s_unspeed_pid);
		LOGINFO("set module.unspeed = %s", s_unspeed_pid.c_str());
	}

	return ret;
}



int cam_manager::get_video_param(cam_video_param& param, std::shared_ptr<cam_video_param>& sec_param)
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(master_index_);
	if (it == map_cam_.end()) {
		LOGERR("cann't find master");
		return INS_ERR;
	}

	return it->second->get_video_param(param, sec_param);
}



int cam_manager::start_all_timelapse_ex(const cam_photo_param& param, const std::shared_ptr<cam_img_repo>& img_repo)
{
	std::lock_guard<std::mutex> lock(mtx_);

	int ret = INS_OK;
	for (auto& it : map_cam_) {		/* 给模组设置拍照参数 */
		auto r = it.second->set_photo_param(param);
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto& it : map_cam_) {
		auto f = it.second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	RETURN_IF_NOT_OK(ret);

	/* 启动拍照线程 */
	timelapse_quit_ = false;
	th_timelapse_ = std::thread(&cam_manager::timelapse_task, this, param, img_repo);

	return INS_OK;
}



void cam_manager::timelapse_task(cam_photo_param param, const std::shared_ptr<cam_img_repo>& img_repo)
{
	int ret;
	int sequence = 0;

	struct timeval tm_start;
	gettimeofday(&tm_start, nullptr);

	while (!timelapse_quit_) {	
		
		struct timeval tm;
		gettimeofday(&tm, nullptr);
		long long past = tm.tv_sec * 1000 * 1000 + tm.tv_usec - tm_start.tv_sec * 1000 * 1000 - tm_start.tv_usec;
		long long wait = (long long)sequence * 1000 * param.interval;

		if (past <= wait) {
			long long delta = wait - past;
			std::unique_lock<std::mutex> lock(mtx_cv_);
			cv_.wait_for(lock, std::chrono::microseconds(delta));
		} else {
			LOGINFO("timelapse take photo had been delay:%lld", past - wait);
		}

		if (timelapse_quit_) break;

		sequence++;
		param.sequence = sequence;
		ret = timelapse_take_pic(param, img_repo);	/* 发送拍timelapse命名 */
		if (ret != INS_OK) {	
			json_obj obj;
			obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
			obj.set_int("code", ret);
			access_msg_center::queue_msg(0, obj.to_string());
			break;
		}
	}

	LOGINFO("timelapse task finish");
}



int cam_manager::timelapse_take_pic(const cam_photo_param& param, const std::shared_ptr<cam_img_repo>& img_repo)
{
	std::lock_guard<std::mutex> lock(mtx_);
	int ret = INS_OK;

	LOGINFO("take timelapse url: %s", tmp_param.file_url.c_str());

	/* 给模组设置时间 */
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		ret = it->second->set_camera_time();
		RETURN_IF_NOT_OK(ret);
	}

	/* 发送拍照命令 */	
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {	 	
		auto r = it->second->start_still_capture(param, img_repo);
		if (r != INS_OK && ret != INS_ERR_M_NO_SDCARD && ret != INS_ERR_M_NO_SPACE && ret != INS_ERR_M_STORAGE_IO) 
			ret = r;
	}

	std::string s_unspeed_pid;
	std::map<uint32_t, std::future<int32_t>> map_f;
	for (auto& it : map_cam_) {
		auto f = it.second->wait_cmd_over();
		map_f.insert(std::make_pair(it.first, std::move(f)));
	}

	for (auto& it : map_f) {
		auto r = it.second.get();
		if (r != INS_OK && ret != INS_ERR_M_NO_SDCARD && ret != INS_ERR_M_NO_SPACE && ret != INS_ERR_M_STORAGE_IO) 
			ret = r;

		if (r == INS_ERR_M_UNSPEED_STORAGE) {	/* 存储速度不足 */
			if (s_unspeed_pid != "") 
				s_unspeed_pid += "_";
			s_unspeed_pid += std::to_string(pid_[it.first]);
		}
	}

	if (s_unspeed_pid != "") {
		property_set("module.unspeed", s_unspeed_pid);
		LOGINFO("set module.unspeed = %s", s_unspeed_pid.c_str());
	}

	return ret;
}


int cam_manager::stop_all_timelapse_ex()
{
	//std::lock_guard<std::mutex> lock(mtx_); /* 不锁，不然会和timelapse_take_pic里的锁形成死锁 */

	timelapse_quit_ = true;		/* cam_manager::timelapse_task退出, 不给Module发照片命令 */
	cv_.notify_all();
	INS_THREAD_JOIN(th_timelapse_);

	return INS_OK;
}


int32_t cam_manager::set_all_capture_mode(uint32_t mode)
{
	std::lock_guard<std::mutex> lock(mtx_);
	int ret = INS_OK;

	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto r = it->second->set_capture_mode(mode);
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto f = it->second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	return ret;
}

int cam_manager::start_still_capture(unsigned int index, const cam_photo_param& param)
{
	std::lock_guard<std::mutex> lock(mtx_);

	int ret;
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR;

	/* 设置本次拍照的参数, 并等待命令完成 */
	ret = it->second->set_photo_param(param);
	RETURN_IF_NOT_OK(ret);

	auto f = it->second->wait_cmd_over();
	ret = f.get();
	RETURN_IF_NOT_OK(ret);

	/* 发送拍照命令, 并等待命令完成 */
	ret = it->second->start_still_capture(param);
	RETURN_IF_NOT_OK(ret);

	auto f1 = it->second->wait_cmd_over();
	ret = f1.get();
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

std::future<int32_t> cam_manager::start_all_still_capture(const cam_photo_param& param, const std::shared_ptr<cam_img_repo>& img_repo)
{
	return std::async(std::launch::async, &cam_manager::still_capture_task, this, param, img_repo);
}


int32_t cam_manager::still_capture_task(const cam_photo_param& param, std::shared_ptr<cam_img_repo> img_repo)
{
	std::lock_guard<std::mutex> lock(mtx_);
	int ret = INS_OK;

	/* 1.设置模组的系统时间(由于A12系统时间不准，每次录像前对一次时) */
	for (auto& it : map_cam_) {	/* 设置模组时间失败,直接返回 */
		ret = it.second->set_camera_time();
		RETURN_IF_NOT_OK(ret);
	}

	/* 2.设置拍照参数 */
	for (auto& it : map_cam_) {
		auto r = it.second->set_photo_param(param);
		if (r != INS_OK) ret = r;
	}	

	std::vector<std::future<int32_t>> v_f;
	for (auto& it : map_cam_) {
		auto f = it.second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	RETURN_IF_NOT_OK(ret);

	/* 3.发送拍照命令 */
	for (auto& it : map_cam_) {
		auto r = it.second->start_still_capture(param, img_repo);
		if (r != INS_OK && ret != INS_ERR_M_NO_SDCARD && ret != INS_ERR_M_NO_SPACE && ret != INS_ERR_M_STORAGE_IO) ret = r;
	}
	
	std::map<uint32_t, std::future<int32_t>> map_f;
	std::string s_unspeed_pid;
	for (auto& it : map_cam_) {
		auto f = it.second->wait_cmd_over();
		map_f.insert(std::make_pair(it.first, std::move(f)));
	}

	for (auto& it : map_f) {
		auto r = it.second.get();
		if (r != INS_OK && ret != INS_ERR_M_NO_SDCARD && ret != INS_ERR_M_NO_SPACE && ret != INS_ERR_M_STORAGE_IO) 
			ret = r;

		if (r == INS_ERR_M_UNSPEED_STORAGE) {
			if (s_unspeed_pid != "") 
				s_unspeed_pid += "_";
			s_unspeed_pid += std::to_string(pid_[it.first]);
		}
	}

	if (s_unspeed_pid != "") {
		property_set("module.unspeed", s_unspeed_pid);
		LOGINFO("set module.unspeed = %s", s_unspeed_pid.c_str());
	}

	return ret;
}


int cam_manager::set_options(int index, std::string property, int value, int value2)
{
	int ret = INS_ERR_BUSY;
	int loop_cnt = 20;
	
	while (--loop_cnt > 0) {
		if (!mtx_.try_lock()) {
			usleep(50*1000);
			continue;
		} else {
			if (index == INS_CAM_ALL_INDEX) {
				ret = set_all_options(property, value, value2);
			} else {
				ret = set_one_options(index, property, value, value2);
			}

			mtx_.unlock();
			break;
		}
	}

	return ret;
}

int cam_manager::set_one_options(int index, std::string property, int value, int value2)
{
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = it->second->set_options(property, value);
	RETURN_IF_NOT_OK(ret);

	auto f = it->second->wait_cmd_over();
	ret = f.get();
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int cam_manager::set_all_options(std::string property, int value, int value2)
{
	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto r = it->second->set_options(property, value);
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {	
		auto f = it->second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	return ret;
}


int cam_manager::get_options(int index, std::string property, std::string& value)
{
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = INS_ERR_BUSY;
	int loop_cnt = 20;
	while (--loop_cnt > 0) {
		if (!mtx_.try_lock()) {
			usleep(200*1000);
			continue;
		} else {
			ret = it->second->get_options(property, value);
			mtx_.unlock();
			break;
		}
	}

	return ret;
}

int cam_manager::get_all_options(std::string property, std::vector<std::string>& v_value)
{
	int ret = INS_ERR_BUSY;
	int loop_cnt = 20;
	while (--loop_cnt > 0) {
		if (!mtx_.try_lock()) {
			usleep(200*1000);
			continue;
		} else {
			for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
				std::string value;
				ret = it->second->get_options(property, value);
				BREAK_IF_NOT_OK(ret);
				v_value.push_back(value);
			}
			mtx_.unlock();
			break;
		}
	}

	return ret;
}


int cam_manager::send_all_file_data(std::string file_name, int type, int timeout)
{
	LOGINFO("send file:%s to module", file_name.c_str());

	std::shared_ptr<insbuff> buff;
	unsigned char* data = nullptr;
	unsigned int size = 0;
	if (file_name != "") {
		buff = ins_util::read_entire_file(file_name);
		if (buff == nullptr) return INS_ERR;
		data = buff->data();
		size = buff->size();
	}

	std::lock_guard<std::mutex> lock(mtx_);

	int ret = INS_OK;
	int i = 0;
	std::thread th[cam_num_];

	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++, i++) {
		th[i] = std::thread([this, it, data, size, type, timeout, &ret]() {
			int result = it->second->send_buff_data(data, size, type, timeout);
			if (result != INS_OK) ret = result;
		});
	}

	for (unsigned int i = 0; i < map_cam_.size(); i++) {
		INS_THREAD_JOIN(th[i]);
	}
	
	return ret;
}

int cam_manager::send_all_buff_data(const unsigned char* data, unsigned int size, int type, int timeout)
{
	std::lock_guard<std::mutex> lock(mtx_);

	LOGINFO("send data to module size:%d", size);

	int ret = INS_OK;
	int i = 0;
	std::thread th[cam_num_];

	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++, i++) {
		th[i] = std::thread([this, it, data, size, type, timeout, &ret]() {
			int result = it->second->send_buff_data(data, size, type, timeout);
			if (result != INS_OK) ret = result;
		});
	}

	for (unsigned int i = 0; i < map_cam_.size(); i++) {
		INS_THREAD_JOIN(th[i]);
	}
	
	return ret;
}

int cam_manager::get_version(int index, std::vector<std::string>& version)
{
	std::lock_guard<std::mutex> lock(mtx_);

	//当获取所有模组的版本号的时候，不管module_version是否有值，都要重新获取一次
	if (index == INS_CAM_ALL_INDEX) {
		return get_all_version(version);
	} else {
		return get_one_version(index, version);
	}
}

int cam_manager::get_one_version(int index, std::vector<std::string>& version)
{
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	std::string ver;
	int ret = it->second->get_version(ver);	/* usb_camera->get_version */
	RETURN_IF_NOT_OK(ret);
	
	version.push_back(ver);

	// auto f = it->second->wait_cmd_over();
	// ret = f.get();
	// RETURN_IF_NOT_OK(ret);

	// auto result = it->second->get_result();
	// json_obj root(result.c_str());
	// std::string tmp;
	// root.get_string("version", tmp);
	// version.push_back(tmp);
	// LOGINFO("index:%d firmware version:%s", it->first, tmp.c_str());

	return INS_OK;
}

int cam_manager::get_all_version(std::vector<std::string>& version)
{
	if (map_cam_.empty()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		std::string ver;
		auto r = it->second->get_version(ver);
		if (r != INS_OK) {
			ret = r;
		} else {
			version.push_back(ver);
		}
	}

	// std::map<uint32_t, std::future<int32_t>> v_f;
	// for (auto& it : map_cam_)
	// {
	// 	auto f = it.second->wait_cmd_over();
	// 	v_f.insert(std::make_pair(it.first, std::move(f)));
	// }

	// for (auto& it : v_f)
	// {
	// 	auto r = it.second.get();
	// 	if (r == INS_OK)
	// 	{
	// 		auto res = map_cam_[it.first]->get_result();
	// 		json_obj root(res.c_str());
	// 		std::string tmp;
	// 		root.get_string("version", tmp);
	// 		version.push_back(tmp);
	// 		LOGINFO("index:%d firmware version:%s", it.first, tmp.c_str());
	// 	} 
	// 	else
	// 	{
	// 		ret = r;
	// 	}
	// }

	return ret;
}

int cam_manager::format_flash(int index)
{
	if (index == INS_CAM_ALL_INDEX) {
		return format_all_flash();
	} else {
		return format_one_flash(index);
	}
}

int cam_manager::format_one_flash(int index)
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = it->second->format_flash();
	RETURN_IF_NOT_OK(ret);

	auto f = it->second->wait_cmd_over();
	ret = f.get();
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int cam_manager::format_all_flash()
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (map_cam_.empty()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto r = it->second->format_flash(); 
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto f = it->second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	return ret;
}

int cam_manager::change_all_usb_mode()
{
	std::lock_guard<std::mutex> lock(mtx_);

	for (auto& it : map_cam_) {
		auto ret = it.second->change_usb_mode();
		RETURN_IF_NOT_OK(ret);
		auto f = it.second->wait_cmd_over();
		ret = f.get();
		RETURN_IF_NOT_OK(ret);
	}

	return INS_OK;
}

int cam_manager::test_spi(int index)
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = it->second->test_spi();
	RETURN_IF_NOT_OK(ret);

	auto f = it->second->wait_cmd_over();
	ret = f.get();
	RETURN_IF_NOT_OK(ret);

	auto result = it->second->get_result();
	json_obj root_obj(result.c_str());
	int value = 0;
	root_obj.get_int("value", value);

	if (value) {
		return INS_ERR;
	} else {
		return INS_OK;
	}
}

int cam_manager::get_log_file(std::string file_name)
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (map_cam_.empty()) return INS_ERR_CAMERA_NOT_OPEN;

	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto r = it->second->get_log_file(file_name); 
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto f = it->second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	return ret;
}

int64_t cam_manager::get_start_pts(int32_t index)
{
	std::lock_guard<std::mutex> lock(mtx_);
	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) {
		return INS_PTS_NO_VALUE;
	} else {
		return it->second->get_start_pts();
	}
}

//index指拼接顺序，要转换成pid
// std::shared_ptr<ins_frame> cam_manager::get_still_pic_frame(unsigned int index)
// {
// 	auto it = map_cam_.find(index);
// 	if (it == map_cam_.end()) return nullptr;

// 	return it->second->deque_pic();
// }

void cam_manager::reset_base_ref()
{
	usb_camera::base_ref_pts_ = INS_PTS_NO_VALUE;
}

int cam_manager::upgrade(std::string file_name, std::string version)
{
	std::lock_guard<std::mutex> lock(mtx_);

	int ret = INS_OK;
	std::string md5_value;
	ret = ins_util::md5_file(file_name, md5_value);
	RETURN_IF_NOT_OK(ret);

	std::thread th[cam_num_];
	int result[cam_num_];
	for (unsigned int i = 0; i < cam_num_; i++) {
		th[i] = std::thread([this, i, &file_name, &result, &md5_value]() {	
			result[i] = INS_ERR;
			int loop = 3;	// 如果升级失败尝试3次
			while (loop-- > 0) {
				if (INS_OK == map_cam_[i]->upgrade(file_name, md5_value)) {
					result[i] = INS_OK;
					break;
				}
			}
		});
	}

	for (unsigned int i = 0; i < cam_num_; i++) {
		INS_THREAD_JOIN(th[i]);
		if (result[i] != INS_OK) ret = INS_ERR;
	}

	RETURN_IF_NOT_OK(ret);

	ret = reboot_all_camera();
	RETURN_IF_NOT_OK(ret);

	map_cam_.clear();

	ret = check_fw_version(version);
	RETURN_IF_NOT_OK(ret);

	return INS_OK;
}

int cam_manager::check_fw_version(std::string version)
{
	int loop_cnt = 1000;
	while (--loop_cnt > 0) {
		if (!is_all_open()) {
			LOGINFO("module start reboot");
			break;
		}
		usleep(50*1000);
	}

	loop_cnt = 1000;
	while (--loop_cnt > 0) {
		if (is_all_open()) {
			LOGINFO("all module reboot success");
			break;
		}

		usleep(50*1000);
	}

	if (!is_all_open()) {
		LOGERR("not all module reboot success");
		return INS_ERR;
	}

	usleep(100*1000);

	for (uint32_t i = 0; i < pid_.size(); i++) {
		auto cam = std::make_shared<usb_camera>(pid_[i], i);
		map_cam_.insert(std::make_pair(i, cam));
	}

	std::vector<std::string> v_version;
	get_all_version(v_version);
	for (unsigned int i = 0; i < v_version.size(); i++) {
		auto pos = v_version[i].find(".", 0);
		if (pos == std::string::npos) return INS_ERR;
		std::string tmp = v_version[i].substr(pos+1);
		if (version != tmp) {
			LOGERR("version check fail, module version:%d %s after upgrade != req version:%d %s", 
				tmp.length(), tmp.c_str(), version.length(), version.c_str());
			return INS_ERR;
		}
	}

	return INS_OK;
}

int cam_manager::reboot_all_camera()
{
	int ret = INS_OK;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto r = it->second->reboot();
		if (r != INS_OK) ret = r;
	}

	std::vector<std::future<int32_t>> v_f;
	for (auto it = map_cam_.begin(); it != map_cam_.end(); it++) {
		auto f = it->second->wait_cmd_over();
		v_f.push_back(std::move(f));
	}

	for (auto& it : v_f) {
		auto r = it.get();
		if (r != INS_OK) ret = r;
	}

	return ret;
}

int cam_manager::set_uuid(unsigned int index, std::string uuid)
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	return it->second->set_uuid(uuid);
}

int cam_manager::get_uuid(unsigned int index, std::string& uuid, std::string& sensor_id)
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	return it->second->get_uuid(uuid, sensor_id);
}

int cam_manager::calibration_awb(unsigned int index, int x, int y, int r, std::string& sensor_id, int* maxvalue, std::shared_ptr<insbuff>& buff)
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(index);
	if (it == map_cam_.end()) return INS_ERR_CAMERA_NOT_OPEN;

	return it->second->calibration_awb_req(x, y, r, sensor_id, maxvalue, buff);
}

int32_t cam_manager::calibration_bpc_all()
{
	std::lock_guard<std::mutex> lock(mtx_);

	int32_t ret = INS_OK;
	std::future<int32_t> futures[INS_CAM_NUM];

	for (int32_t i = 0; i < INS_CAM_NUM; i++) {
		futures[i] = std::async(std::launch::async, [this, i] {
			auto it = map_cam_.find(i);
			if (it == map_cam_.end()) {
				return INS_ERR_CAMERA_NOT_OPEN;
			} else {
				return it->second->calibration_bpc_req();
			}
		});
	}

	for (int32_t i = 0; i < INS_CAM_NUM; i++) {
		auto r = futures[i].get();
		if (r != INS_OK) ret = r;
	}

	LOGINFO("bpc calibration result:%d", ret);

	return ret;
}

int cam_manager::calibration_blc_all(bool b_reset)
{
	std::lock_guard<std::mutex> lock(mtx_);
	int32_t ret = INS_OK;
	std::future<int32_t> futures[INS_CAM_NUM];

	for (int i = 0; i < INS_CAM_NUM; i++) {
		futures[i] = std::async(std::launch::async, [this, i, b_reset] {
			auto it = map_cam_.find(i);
			if (it == map_cam_.end())  return INS_ERR_CAMERA_NOT_OPEN;
			if (b_reset) {
				return it->second->calibration_blc_reset();
			} else {
				return it->second->calibration_blc_req();
			}
		});	
	}

	for (int32_t i = 0; i < INS_CAM_NUM; i++) {
		auto r = futures[i].get();
		if (r != INS_OK) ret = r;
	}

	LOGINFO("blc calibration result:%d", ret);

	return ret;
}

int32_t cam_manager::storage_speed_test(std::map<int32_t, bool>& map_res)
{
	std::lock_guard<std::mutex> lock(mtx_);
	std::future<int32_t> futures[INS_CAM_NUM];

	for (int i = 0; i < INS_CAM_NUM; i++) {
		futures[i] = std::async(std::launch::async, [this, i] {
			auto it = map_cam_.find(i);
			if (it == map_cam_.end()) {
				return INS_ERR_CAMERA_NOT_OPEN;
			} else {
				return it->second->storage_speed_test();
			}
		});	
	}

	for (int32_t i = 0; i < INS_CAM_NUM; i++) {
		auto r = futures[i].get();
		map_res.insert(std::make_pair(pid_[i], (r==INS_OK)));
	}

	return INS_OK;
}

int cam_manager::gyro_calibration()
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(master_index_);
	if (it == map_cam_.end()) {
		return INS_ERR_CAMERA_NOT_OPEN;
	} else {
		return it->second->gyro_calibration();
	}
}

int32_t cam_manager::magmeter_calibration()
{
	std::lock_guard<std::mutex> lock(mtx_);

	auto it = map_cam_.find(master_index_);
	if (it == map_cam_.end()) {
		return INS_ERR_CAMERA_NOT_OPEN;
	} else {
		return it->second->magmeter_calibration();
	}
}

int32_t cam_manager::start_magmeter_calibration()
{
	auto it = map_cam_.find(master_index_);
	if (it == map_cam_.end()) {
		return INS_ERR_CAMERA_NOT_OPEN;
	} else {
		return it->second->start_magmeter_calibration();
	}
}

int32_t cam_manager::stop_magmeter_calibration()
{
	auto it = map_cam_.find(master_index_);
	if (it == map_cam_.end()) {
		return INS_ERR_CAMERA_NOT_OPEN;
	} else {
		return it->second->stop_magmeter_calibration();
	}
}

int32_t cam_manager::vig_min_change()
{
	std::lock_guard<std::mutex> lock(mtx_);

	int32_t min_value = INT_MAX;
	for (auto& it : map_cam_) {
		int32_t value = 0;
		auto ret = it.second->get_vig_min_value(value);
		RETURN_IF_NOT_OK(ret);
		min_value = std::min(min_value, value);
	}

	for (auto& it : map_cam_) {
		it.second->set_vig_min_value(min_value);
	}

	return INS_OK;
}

int32_t cam_manager::delete_file(std::string dir)
{
	std::lock_guard<std::mutex> lock(mtx_);
	std::future<int32_t> futures[INS_CAM_NUM];

	for (int i = 0; i < INS_CAM_NUM; i++) {
		futures[i] = std::async(std::launch::async, [this, i, dir]
		{
			auto it = map_cam_.find(i);
			if (it == map_cam_.end()) {
				return INS_ERR_CAMERA_NOT_OPEN;
			} else {
				return it->second->delete_file(dir);
			}
		});	
	}

	int32_t ret = INS_OK;
	for (int32_t i = 0; i < INS_CAM_NUM; i++) {
		if (futures[i].valid()) {
			auto r = futures[i].get();
			if (r != INS_OK) ret = r;
		}
	}

	return ret;
}

int cam_manager::get_module_hw_version(int& hw_version)
{
	hw_version = 3;
	// std::vector<std::string> v_version;
	// int ret = get_firmware_version(master_index_, v_version);
	// RETURN_IF_NOT_OK(ret);

	// std::string hw_ver;
	// ret = parse_module_version(v_version[0], hw_ver);
	// RETURN_IF_NOT_OK(ret);	

	// hw_version = atoi(hw_ver.c_str());
	// LOGINFO("hw version:%d", hw_version);

	// int module_hw_version = 0;
	// xml_config::get_value(INS_CONFIG_OPTION, INS_CONFIG_MODULE_HW_VERSION, module_hw_version);
	// if (module_hw_version > 0)
	// {
	// 	hw_version = module_hw_version;
	// 	LOGINFO("use config's module hw version:%d", module_hw_version);
	// }

	return INS_OK;
}

int cam_manager::parse_module_version(const std::string& version, std::string& hw_ver)
{
	if (version[0] == 'V') {
		hw_ver = "1";
		return INS_OK;
	}

	std::vector<std::string> v;
	ins_util::split(version, v, ".");

	if (v.size() <= 0) {
		LOGERR("invalid version:%s", version.c_str());
		return INS_ERR;
	}

	hw_ver = v[0];

	return INS_OK;
}

void cam_manager::load_config_from_storage(std::string path)
{
	if (path == "") return;

	std::string filename = path + "/cam_config.xml";
	FILE* fp = fopen(filename.c_str(), "r");
	if (fp == nullptr) {
		LOGINFO("-------------file:%s open fail", filename.c_str());
		return;
	}
	
	char buff[64] = {0};
	fgets(buff, sizeof(buff), fp);
	if (buff[strlen(buff)-1] == '\n') 
		buff[strlen(buff)-1] = 0; //去掉换行符
	fclose(fp);

	LOGINFO("----------%s config pid:%s", filename.c_str(), buff);
	xml_config::set_value(INS_CONFIG_OPTION, INS_CONFIG_PID, buff);
}

int32_t cam_manager::send_data(uint32_t index, uint8_t* data, uint32_t size)
{
	//send data 这里不用加锁， usb_camera 里会加锁
	auto it = map_cam_.find(index);
	if (it != map_cam_.end()) {
		it->second->send_data(data, size);
	} else {
		return INS_ERR_CAMERA_NOT_OPEN;
	}
}

