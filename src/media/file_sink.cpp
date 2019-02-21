
#include "file_sink.h"
#include "inslog.h"
#include "common.h"
#include "ins_util.h"
#include <fcntl.h> 
#include <dirent.h>
#include <sys/wait.h>

file_sink::~file_sink()
{
	quit_ = true;
	INS_THREAD_JOIN(th_);
	mux_ = nullptr;
	//pending_tail(url_);
	LOGINFO("sink close url:%s", url_.c_str());
}

// for (unsigned i = 0; i < v_dev.size(); i++)
	// {
	// 	int retry = 10;
	// 	bool b_ok = false;
	// 	int cnt = 1000/v_dev.size();
		
	// 	while (retry--)
	// 	{
	// 		sync();
	// 		usleep(cnt*1000);
			
	// 		int fd = ::open(v_dev[i].c_str(), O_RDWR | O_SYNC | O_DIRECT);
	// 		if (fd < 0)
	// 		{
	// 			LOGERR("device:%s open fail：%d %s", v_dev[i].c_str(), errno, strerror(errno));
	// 			continue;
	// 		}

	// 		sync();
	// 		usleep(cnt*1000);
			
	// 		if (fsync(fd) < 0) 
	// 		{
	// 			LOGERR("device:%s fsync fail：%d %s", v_dev[i].c_str(), errno, strerror(errno));
	// 			close(fd);
	// 			continue;
	// 		}

	// 		struct stat st;
	// 		if (fstat(fd, &st) < 0) 
	// 		{
	// 			LOGERR("device:%s fstat fail：%d %s", v_dev[i].c_str(), errno, strerror(errno));
	// 			close(fd);
	// 			continue;
	// 		}

	// 		close(fd);
	// 		sync();
	// 		usleep(cnt*1000);

	// 		LOGINFO("device:%s sync ok", v_dev[i].c_str());
	// 		b_ok = true;
	// 		break;
	// 	}

	// 	if (!b_ok) LOGINFO("device:%s sync fail", v_dev[i].c_str());
	// }

// void file_sink::pending_tail(const std::string& url)
// {
// 	if (url.find("/mnt/media_rw/") != 0) return;

// 	auto pos = url.find("/", strlen("/mnt/media_rw/"));
// 	if (pos == std::string::npos) return;

// 	std::string g_url = url.substr(0, pos) + "/.gift";
// 	int fd = ::open(g_url.c_str(), O_WRONLY|O_CREAT, 0666);
// 	if (fd >= 0)
// 	{
// 		LOGINFO("write pending file:%s", g_url.c_str());
// 		auto buff = std::make_shared<insbuff>(128*1024);
// 		srand(time(0));
// 		char num = rand();
// 		memset(buff->data(), num, buff->size());
// 		for (int i = 0; i < 1024 * 8; i++)
// 		{
// 			::write(fd, buff->data(), buff->size());
// 		}
// 		fsync(fd);
// 		::close(fd);
// 		sync(); sync();
// 	}
// 	else
// 	{
// 		LOGERR("file:%s open fail when pending tail", g_url.c_str());
// 	}
// }

file_sink::file_sink(std::string url, bool b_audio, bool b_video, bool b_camm)
	: url_(url)
	, b_audio_(b_audio)
	, b_video_(b_video)
	, b_camm_(b_camm)
{
	//system("echo 0 > /proc/sys/vm/dirty_background_ratio");
	LOGINFO("sink:%s a:%d v:%d camm:%d", url_.c_str(), b_audio_, b_video_, b_camm_);
}

void file_sink::set_audio_param(const ins_audio_param& param)
{
	if (!b_audio_) return; 

	audio_param_ = std::make_shared<ins_audio_param>();
	*(audio_param_.get()) = param;

	LOGINFO("sink:%s set audio param", url_.c_str());

	if (open_mux())
	{
		th_ = std::thread(&file_sink::task, this);
	}
}

void file_sink::set_video_param(const ins_video_param& param)
{
	if(!b_video_) return;

	video_param_ = std::make_shared<ins_video_param>();
	*(video_param_.get()) = param;

	LOGINFO("sink:%s set video param", url_.c_str());

	if (open_mux())
	{
		th_ = std::thread(&file_sink::task, this);
	}
}

void file_sink::task()
{
	//long long video_frames = 0;
	while (1)
	{	
		if (b_flush_) 
		{
			if (mux_) mux_->flush();
			b_flush_ = false;
		}
		
		auto frame = dequeue_frame();
		if (frame == nullptr)
		{
			if (quit_) break; //结束的时候将队列里的写到文件
			usleep(20*1000);
		}
		else
		{
			write_mux(frame);
			// if (frame->media_type == INS_MEDIA_VIDEO) 
			// {
			// 	if (!(++video_frames%2000))
			// 	{
			// 		sync(); 
			// 		usleep(100*1000);
			// 		LOGINFO("sync queue:%d", video_queue_.size());
			// 	}
			// }
		}
	}
}

bool file_sink::open_mux()
{
	if (b_audio_ && audio_param_ == nullptr) return false;

	if (b_video_ && video_param_ == nullptr) return false;

	ins_mux_param param;
	if (video_param_)
	{
		param.has_video = true;
		param.video_mime = video_param_->mime;
		param.width = video_param_->width;
		param.height = video_param_->height;
		param.v_timebase = video_param_->v_timebase;
		param.v_config = video_param_->config->data();
		param.v_config_size = video_param_->config->size();
		param.fps = video_param_->fps;
		param.v_bitrate = video_param_->bitrate;
		if (video_param_->spherical_mode == INS_MODE_PANORAMA)
		{
			param.spherical_mode = INS_SPERICAL_MONO;
		}
		else if (video_param_->spherical_mode == INS_MODE_3D_TOP_RIGHT || video_param_->spherical_mode == INS_MODE_3D_TOP_LEFT)
		{
			param.spherical_mode = INS_SPERICAL_TOP_BOTTOM;
		}
	}
	if (audio_param_)
	{
		param.has_audio = true;
		param.audio_mime = audio_param_->mime;
		param.samplerate = audio_param_->samplerate;
		param.channels = audio_param_->channels;
		param.a_timebase = audio_param_->a_timebase;
		param.a_config = audio_param_->config->data();
		param.a_config_size = audio_param_->config->size();
		param.audio_bitrate = audio_param_->bitrate;
		param.b_spatial_audio = audio_param_->b_spatial;
	}

	param.io_options = mux_io_opt_;

	if (b_camm_) param.camm_tracks = 1;

	mux_ = std::make_shared<ins_mux>();
	if (INS_OK != mux_->open(param, url_.c_str()))
	{
		mux_ = nullptr;
		return false;
	}

	return true;
}

int file_sink::write_mux(const std::shared_ptr<ins_frame>& frame)
{
	if (mux_ == nullptr) return INS_OK;

	if (check_disk_space()) return INS_OK;

	ins_mux_frame mux_frame;
	if (frame->media_type == INS_MEDIA_CAMM || frame->media_type == INS_MEDIA_AUDIO)
	{
		mux_frame.data = frame->buf->data();
		mux_frame.size = frame->buf->size();
	}
	else
	{
		mux_frame.data = frame->page_buf->data();
		mux_frame.size = frame->page_buf->offset();
	}

	mux_frame.pts = frame->pts;
	mux_frame.dts = frame->dts;
	mux_frame.duration = frame->duration;
	mux_frame.media_type = frame->media_type;
	mux_frame.b_key_frame = frame->is_key_frame;

	if (INS_OK!=  mux_->write(mux_frame))
	{
		mux_ = nullptr;
		if (event_cb_) event_cb_(INS_ERR_MUX_WRITE);
		return INS_ERR;
	}
	
	return INS_OK;
}

bool file_sink::check_disk_space()
{
	int ret = ins_util::disk_available_size(url_.c_str());

	if (ret == -1 ||  ret > INS_MIN_SPACE_MB - 100) return false;
	
	LOGERR("no disk space left:%d", ret);
	mux_ = nullptr; 

	if (event_cb_) event_cb_(INS_ERR_NO_STORAGE_SPACE);

	return true;
}

void file_sink::queue_frame(const std::shared_ptr<ins_frame>& frame)
{
	//LOGINFO("meida:%d pts:%lld", frame->media_type, frame->pts);

	std::lock_guard<std::mutex> lock(mtx_);

	if (frame->media_type == INS_MEDIA_VIDEO)
	{
		if (b_video_) 
		{
			if (video_queue_.size() > 200)
			{
				LOGERR("sink:%s queue size:%d", url_.c_str(), video_queue_.size());
				if (event_cb_) event_cb_(INS_ERR_UNSPEED_STORAGE);
				return;
			}
			else
			{
				video_queue_.push_back(frame);
			}
		}
	}
	else if (frame->media_type == INS_MEDIA_AUDIO)
	{
		if (b_audio_) audio_queue_.push_back(frame);
	}
	else if (frame->media_type == INS_MEDIA_CAMM)
	{
		if (b_camm_) camm_queue_.push_back(frame);
	}
}

std::shared_ptr<ins_frame> file_sink::dequeue_frame()
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (video_queue_.empty() && audio_queue_.empty() && camm_queue_.empty()) return nullptr;

	long long camm_pts = INS_MAX_INT64;
	long long video_pts = INS_MAX_INT64;
	long long audio_pts = INS_MAX_INT64;

	if (!video_queue_.empty()) video_pts = video_queue_.front()->pts;
	if (!audio_queue_.empty()) audio_pts = audio_queue_.front()->pts;
	if (!camm_queue_.empty()) camm_pts = camm_queue_.front()->pts;

	if (video_pts <= audio_pts && video_pts <= camm_pts)
	{
		auto frame = video_queue_.front();
		video_queue_.pop_front();
		return frame;
	}
	else if (audio_pts <= video_pts && audio_pts <= camm_pts)
	{
		auto frame = audio_queue_.front();
		audio_queue_.pop_front();
		return frame;
	}
	else
	{
		auto frame = camm_queue_.front();
		camm_queue_.pop_front();
		return frame;
	}
}
