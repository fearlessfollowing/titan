
#include "stream_sink.h"
#include "inslog.h"
#include "common.h"
#include <fcntl.h>
#include "ins_util.h"
#include "access_msg_center.h"
#include <sstream>
#include <fstream>
#include "prj_file_mgr.h"
#include "spherical_metadata.h"
#include "camm_util.h"
#include "ffh264dec.h"
#include "tjpegenc.h"

#define MAX_DISCARD_CNT 	12

#define SINK_AUTO_FRAGMENT  0
#define SINK_MANU_FRAGMENT  1
#define SINK_NO_FRAGMENT    2


stream_sink::~stream_sink()
{
	b_release_ = true;
	INS_THREAD_JOIN(th_);		/* 停止stream_sink线程 */


	/* 如果只需要存储最后一个关键帧(老化模式) */
	if (b_just_last_frame_) 
		key_frame_to_jpeg(last_key_frame_);

	mux_ = nullptr;

	LOGINFO("stream sink close url:%s", url_.c_str());
}



stream_sink::stream_sink(std::string url) : url_(url)
{
	path_ = url_;
	int pos = path_.rfind("/", path_.length());
	path_.erase(pos + 1, path_.length() - pos - 1);
	//LOGINFO("sink:%s open", url_.c_str());
}


void stream_sink::set_fragment(bool frag, bool manu)
{
	if (!frag) {	/* 0代表不分段 */
		fragment_type_ = SINK_NO_FRAGMENT;
	} else if (manu) {
		fragment_type_ = SINK_MANU_FRAGMENT;
	} else {
		fragment_type_ = SINK_AUTO_FRAGMENT;
	} 
}


void stream_sink::start(int64_t start_pts)
{
	LOGINFO("%s start, pts:%lld", url_.c_str(), start_pts);
	start_pts_ = start_pts;

	if (!b_video_ && !b_audio_ && (camm_tracks())) {	/* 只有camm通道的时候，这里启动 */
		th_ = std::thread(&stream_sink::task, this);
	}
}

void stream_sink::set_audio_param(const ins_audio_param& param)
{
	if (!b_audio_) return; 

	audio_param_ = std::make_shared<ins_audio_param>();
	*(audio_param_.get()) = param;

	LOGINFO("%s set audio param", url_.c_str());

	if (!(b_video_ && video_param_ == nullptr)) {
		th_ = std::thread(&stream_sink::task, this);
	}
}

void stream_sink::set_video_param(const ins_video_param& param)
{
	if(!b_video_) return;

	video_param_ = std::make_shared<ins_video_param>();
	*(video_param_.get()) = param;

	LOGINFO("%s set video param", url_.c_str());

	if (!(b_audio_ && audio_param_ == nullptr)) {
		th_ = std::thread(&stream_sink::task, this);
	}
}


void stream_sink::task()
{
	if (open_mux()) {
		if (b_live_) 
			send_net_link_state_msg(true);	/* 启动流任务时,如果是直播模式,发送"connected"消息 */
	} else { 
		return;
	}

	int ret;
	while (!b_release_) {	
		auto ret = dequeue_and_write();
		if (ret < 0) {
			LOGERR("%s mux write error task exit");
			return;
		} else if (ret == SINK_NO_FRAME) {	/* 代表没有获取到帧，休眠再获取 */
			usleep(10*1000);
		}

	#if 0
		auto v_frame = dequeue_frame();
		if (v_frame.empty()) {
			usleep(10*1000);
		} else {
			for (uint32_t i = 0; i < v_frame.size(); i++) {
				if (INS_OK != write_mux(v_frame[i])) return;
			}
		}
	#endif
	}
}



bool stream_sink::open_mux()
{
	if (b_audio_ && audio_param_ == nullptr) return false;

	if (b_video_ && video_param_ == nullptr) return false;

	if (b_just_last_frame_) return false;

	ins_mux_param param;
	if (video_param_) {
		param.has_video = true;
		param.video_mime = video_param_->mime;
		param.width = video_param_->width;
		param.height = video_param_->height;
		param.v_timebase = video_param_->v_timebase;
		param.v_config = video_param_->config->data();
		param.v_config_size = video_param_->config->size();
		param.fps = video_param_->fps;
		param.v_bitrate = video_param_->bitrate;

		if (url_.find("pano", 0) != std::string::npos) {
			param.spherical_mode = INS_SPERICAL_MONO;
		} else if (url_.find("3d", 0) != std::string::npos) {
			param.spherical_mode = INS_SPERICAL_TOP_BOTTOM;
		}

		fps_ = video_param_->fps;
		assert(fps_ > 0.0);
	}


	if (audio_param_) {
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

	/* 直播和预览流文件不用写camm track */
	param.camm_tracks = camm_tracks();

	mux_ = std::make_shared<ins_mux>();
	if (INS_OK != mux_->open(param, url_.c_str())) {
		mux_ = nullptr;
		b_start_ = false;  /* 连的时候需要将这个值为false */
		LOGERR("mux open fail:%s", url_.c_str());
		do_mux_error(true);
		return false;
	} else {
		b_start_ = true;
	}

	return true;
}



int stream_sink::write_mux(const ins_frame* frame)
{
	if (mux_ == nullptr) 
		return INS_OK;

	/* camm gyro 要从视频开始前就存 */
	if ((frame->pts < start_pts_ || start_pts_ == INS_PTS_NO_VALUE) 
		&& frame->media_type != INS_MEDIA_CAMM_GPS 
		&& frame->media_type != INS_MEDIA_CAMM_GYRO 
		&& frame->media_type != INS_MEDIA_CAMM_EXP) {
		//LOGINFO("media type:%d pts:%lld < start pts:%lld", frame->media_type, frame->pts, start_pts_);
		return INS_OK;
	}

	ins_mux_frame mux_frame;
	if (frame->media_type == INS_MEDIA_CAMM_GPS 
		|| frame->media_type == INS_MEDIA_CAMM_GYRO 
		|| frame->media_type == INS_MEDIA_CAMM_EXP) {
		mux_frame.data = frame->buf->data();
		mux_frame.size = frame->buf->size();
	} else if (frame->media_type == INS_MEDIA_AUDIO) {
		mux_frame.data = frame->buf->data();
		mux_frame.size = frame->buf->offset();
	} else if (b_live_) {
		mux_frame.data = frame->page_buf->data();
		mux_frame.size = frame->page_buf->offset();
	} else {
		mux_frame.data = frame->page_buf->data();
		mux_frame.size = frame->page_buf->size();
	}
	
	mux_frame.pts = frame->pts - start_pts_;
	mux_frame.dts = frame->dts - start_pts_;
	mux_frame.duration = frame->duration;
	mux_frame.media_type = frame->media_type;
	mux_frame.b_key_frame = frame->is_key_frame;

	if (INS_OK !=  mux_->write(mux_frame)) {
		mux_ = nullptr;
		b_start_ = false; 		/* 重连的时候需要将这个值为false */
		LOGERR("mux write fail:%s", url_.c_str());
		return do_mux_error(false);
	}

	file_pos_ = mux_frame.position;

	if (frame->media_type == INS_MEDIA_VIDEO) 
		check_fragment(frame);

	/* 每100M检测一次， 检测剩余空间调用statfs，对于某些sd卡极慢，所以不要每一帧都检测 */
	if (!b_live_ && file_pos_ > next_check_size_) {
		next_check_size_ += 25*1024*1024;
		check_disk_space(frame->pts);
	}

	return INS_OK;
}


void stream_sink::check_fragment(const ins_frame* frame)
{	
	if (b_live_) 
		return;

	if (fragment_type_ == SINK_NO_FRAGMENT) {
		return;
	} else if (fragment_type_ == SINK_AUTO_FRAGMENT) {
		if (!(file_pos_ > 3900000000 
			&& frame->is_key_frame 
			&& frame->media_type == INS_MEDIA_VIDEO)) {
			return;
		}
	} else {
		if (!frame->b_fragment) return;
	}

	//以下为分段处理
	create_new_mux(frame->pts);

	if (b_origin_key_) 
		prj_file_mgr::add_origin_frag_file(path_, url_sequence_-1);
}



void stream_sink::check_disk_space(long long pts)
{
	if (b_live_) return;

	if (ins_util::disk_available_size(url_.c_str()) > INS_MIN_SPACE_MB - 200) return;
	
	LOGERR("no disk space left");
	mux_ = nullptr; 
	b_start_ = false; //卡满后,就不用在接收数据了

	if (b_override_) {	
		create_new_mux(pts);
	} else if (b_origin_key_ || b_stitching_) {
		send_rec_over_msg(INS_ERR_NO_STORAGE_SPACE);
	}
}

void stream_sink::create_new_mux(int64_t pts)
{
	//std::lock_guard<std::mutex> lock(mtx_);
	//分段的时候视频/音频一帧不丢
	if (audio_queue_.empty()) {
		start_pts_ = pts;
	} else {
		auto audio_frame = audio_queue_.front();
		start_pts_ = std::min(pts, audio_frame->pts);
		//LOGINFO("sink:%s fragment new audio pts:%lld video pts:%lld", url_.c_str(), audio_frame->pts, pts);
	}
	mux_ = nullptr;
	next_check_size_ = 25*1024*1024;
	create_new_url();
	open_mux();
}

bool stream_sink::av_sync()
{
	if (start_pts_ != INS_PTS_NO_VALUE) return true;
	if (!b_auto_start_) return false;

	std::lock_guard<std::mutex> lock(mtx_);

	if (b_video_) {	// if has video sync to video
		while (!video_queue_.empty()) {
			auto frame = video_queue_.front();
			if (!frame->is_key_frame) {	// video must start with key frame
				LOGINFO("-----not key frame discard");
				video_queue_.pop(); continue;
			}
			
			// if (b_audio_) {
			// 	if (audio_queue_.empty() || audio_queue_.front()->pts > frame->pts + 30000) {
			// 		LOGERR("???????????? %s audio queue:%d  discard video", url_.c_str(), audio_queue_.size());
			// 		video_queue_.pop(); continue;
			// 	}
			// }

			start_pts_ = frame->pts;
			break;
		}
	} else {
		if (!audio_queue_.empty()) {
			auto frame = audio_queue_.front();
			start_pts_ = frame->pts;
		}
	}

	if (start_pts_ != INS_PTS_NO_VALUE) {
		LOGINFO("%s start pts:%lld ", url_.c_str(), start_pts_);
		return true;
	} else {
		return false;
	}
}


void stream_sink::queue_gps(std::shared_ptr<ins_gps_data>& gps)
{
	if (!b_gps_) 
		return;

	if (b_release_) 
		return;

	if (!b_start_) 
		return;

	if (b_just_last_frame_) 
		return;

	std::lock_guard<std::mutex> lock(mtx_);
	if (gps_queue_.size() > 100) {
		LOGERR("%s discard 50 gps data", url_.c_str());
		for (int i = 0; i < 50; i++) 
			gps_queue_.pop();
		return;
	} else {
		gps_queue_.push(gps);
	}
}



void stream_sink::queue_gyro(std::shared_ptr<insbuff>& data, int64_t delta_ts)
{
	if (!b_gyro_) return;
	if (b_release_) return;
	if (!b_start_) return;
	if (b_just_last_frame_) return;

	gyro_delta_ts_ = delta_ts;

	std::lock_guard<std::mutex> lock(mtx_);
	if (gyro_queue_.size() > 300) {
		LOGERR("%s discard 100 gyro data start pts:%lld gps:%lu", url_.c_str(), start_pts_, gps_queue_.size());
		for (int i = 0; i < 100; i++) gyro_queue_.pop();
		return;
	} else {
		gyro_queue_.push(data);
	}
}

void stream_sink::queue_exposure_time(std::shared_ptr<exposure_times>& data)
{
	if (!b_exp_) return;
	if (b_release_) return;
	if (!b_start_) return;
	if (b_just_last_frame_) return;

	std::lock_guard<std::mutex> lock(mtx_);
	if (exp_queue_.size() > 300) {
		LOGERR("%s discard 100 expouse data start pts:%lld cur pts:%lld gps:%lu", url_.c_str(), start_pts_, data->pts, gps_queue_.size());
		for (int i = 0; i < 100; i++) exp_queue_.pop();
		return;
	} else {
		exp_queue_.push(data);
	}
}

void stream_sink::queue_frame(const std::shared_ptr<ins_frame>& frame)
{
	//LOGINFO("meida:%d pts:%lld", frame->media_type, frame->pts);

	if (b_release_) return;
	if (!b_start_) return;

	if (b_just_last_frame_) {
		if (frame->is_key_frame && frame->media_type == INS_MEDIA_VIDEO) last_key_frame_ = frame;
		return;
	}

	// if (frame->media_type == INS_MEDIA_AUDIO && audio_queue_.empty())
	// {
	// 	LOGINFO("-------%s audio pts:%lld", url_.c_str(), frame->pts);
	// }

	std::lock_guard<std::mutex> lock(mtx_);

	//if (mux_ == nullptr) return;
	//if (discard_frame_cnt_ > MAX_DISCARD_CNT) return; //丢帧不停止录像PRO2

	//LOGINFO("meida:%d pts:%lld asize:%d in", frame->media_type, frame->pts, audio_queue_.size());

	if (frame->media_type == INS_MEDIA_VIDEO) {
		do_discard_frame(frame->is_key_frame);
		if (b_video_) 
			video_queue_.push(frame);
	} else if (frame->media_type == INS_MEDIA_AUDIO) {
		if (b_audio_) 
			audio_queue_.push(frame);
	}
}

std::shared_ptr<ins_frame> stream_sink::dequeue_av_frame()
{
	std::lock_guard<std::mutex> lock(mtx_);
	
	if (b_audio_ && b_video_) {
		//队列同时有音视频帧存在，这样能保证音视频的时间间隔在最小，以便分段的时候音视频的起始时间相差最小
		if (!video_queue_.empty() && !audio_queue_.empty()) {
			auto audio_frame = audio_queue_.front();
			auto video_frame = video_queue_.front();
			if (audio_frame->pts <= video_frame->pts) {
				audio_queue_.pop();
				return audio_frame;
			} else {
				video_queue_.pop();
				return video_frame;
			}
		}
	} else if (b_video_) {
		if (!video_queue_.empty()) {
			auto frame = video_queue_.front();
			video_queue_.pop();
			return frame;
		}
	} else if (b_audio_) {
		if (!audio_queue_.empty()) {
			auto frame = audio_queue_.front();
			audio_queue_.pop();
			return frame;
		}
	}

	return nullptr;
}


int32_t stream_sink::dequeue_and_write()
{
	if (!av_sync()) return 0;

	int32_t ret = SINK_NO_FRAME;
	auto av_frame = dequeue_av_frame();
	if (av_frame) {
		ret = write_mux(av_frame.get());
		RETURN_IF_NOT_OK(ret);
	}
	
	/* gyro/gps/exposure可能来的比较早，所以等音视频同步确定开始时间后再写 */
	if (start_pts_ == INS_PTS_NO_VALUE) return ret;

	if (b_gps_) {
		auto d = deque_gps_data();
		if (d) {
			ret = write_gps_frame(d);
			RETURN_IF_NOT_OK(ret);
		}
	}
	
	// if (b_gyro_ && b_exp_) //GPS/GYRO/exposure 都有
	// {
	// 	mtx_.lock();
	// 	if (gps_queue_.empty() || gyro_queue_.empty() || exp_queue_.empty()) 
	// 	{
	// 		mtx_.unlock(); return ret;
	// 	}
	// 	int64_t min_gps = gps_queue_.front()->pts;
	// 	int64_t max_gps = gps_queue_.back()->pts;
	// 	int64_t min_exp = exp_queue_.front()->pts;
	// 	int64_t max_exp = exp_queue_.back()->pts;
	// 	amba_gyro_data* t = (amba_gyro_data*)gyro_queue_.front()->data();
	// 	int64_t min_gyro = t->pts - gyro_delta_ts_;
	// 	t = (amba_gyro_data*)(gyro_queue_.front()->data()+gyro_queue_.front()->offset()-sizeof(amba_gyro_data));
	// 	int64_t max_gyro = t->pts - gyro_delta_ts_;
	// 	//LOGINFO("gps:%ld %ld %d exp:%ld %ld %d gyro:%ld %ld %d",
	// 	//	min_gps, max_gps, gps_queue_.size(), min_exp, max_exp, exp_queue_.size(), min_gyro, max_gyro, gyro_queue_.size());
	// 	mtx_.unlock();

	// 	if (min_gps <= min_exp && min_gps <= min_gyro)
	// 	{	
	// 		return write_gps_frame();
	// 	}
	// 	else if (min_exp <= min_gps && min_exp <= min_gyro)
	// 	{
	// 		return write_exp_frame();
	// 	}
	// 	else if (max_gyro <= max_gps && max_gyro <= max_exp)
	// 	{
	// 		auto d = deque_gyro_data();
	// 		uint8_t* p = d->data();
	// 		uint32_t offset = 0;
			
	// 		while (offset + sizeof(amba_gyro_data) <= d->offset())
	// 		{
	// 			auto gyro = (amba_gyro_data*)(p+offset);
	// 			gyro->pts -= gyro_delta_ts_;
	// 			offset += sizeof(amba_gyro_data);

	// 			while (min_gps <= gyro->pts || min_exp <= gyro->pts)
	// 			{
	// 				if (min_gps <= min_exp)
	// 				{
	// 					ret = write_gps_frame(&min_gps);
	// 				}
	// 				else
	// 				{
	// 					ret = write_exp_frame(&min_exp);
	// 				}
	// 				RETURN_IF_NOT_OK(ret);
	// 			}

	// 			ret = write_gyro_frame(gyro);
	// 			RETURN_IF_NOT_OK(ret);
	// 		}
	// 	}
	// }

	if (b_gyro_ && b_exp_) {	// GYRO/exposure
		mtx_.lock();
		if (gyro_queue_.empty() || exp_queue_.empty()) {
			mtx_.unlock(); return ret;
		}
		
		int64_t min_exp = exp_queue_.front()->pts;
		int64_t max_exp = exp_queue_.back()->pts;
		amba_gyro_data* t = (amba_gyro_data*)gyro_queue_.front()->data();
		int64_t min_gyro = t->pts - gyro_delta_ts_;
		t = (amba_gyro_data*)(gyro_queue_.front()->data()+gyro_queue_.front()->offset()-sizeof(amba_gyro_data));
		int64_t max_gyro = t->pts - gyro_delta_ts_;
		mtx_.unlock();

		if (min_exp <= min_gyro) {
			return write_exp_frame();
		} else if (max_gyro <= max_exp) {
			auto d = deque_gyro_data();
			uint8_t* p = d->data();
			uint32_t offset = 0;
			int64_t gyro_pts = 0;
			
			while (offset + sizeof(amba_gyro_data) <= d->offset()) {
				auto gyro = (amba_gyro_data*)(p+offset);
				gyro_pts = gyro->pts - gyro_delta_ts_;
				offset += sizeof(amba_gyro_data);

				while (min_exp <= gyro_pts) {
					ret = write_exp_frame(&min_exp);
					RETURN_IF_NOT_OK(ret);
				}

				ret = write_gyro_frame(gyro);
				RETURN_IF_NOT_OK(ret);
			}
		}
	} else if (b_gyro_)  {	/* 只有gyro,拍照的是只存gyro数据 */
		auto d = deque_gyro_data();
		if (!d) return ret;

		uint8_t* p = d->data();
		uint32_t offset = 0;
		
		while (offset + sizeof(amba_gyro_data) <= d->offset()) {
			auto gyro = (amba_gyro_data*)(p+offset);
			offset += sizeof(amba_gyro_data);
			ret = write_gyro_frame(gyro);
			RETURN_IF_NOT_OK(ret);
		}
	}

	return ret;
}


#if 0
// std::vector<std::shared_ptr<ins_frame>> stream_sink::dequeue_frame()
// {
// 	std::lock_guard<std::mutex> lock(mtx_);

// 	std::vector<std::shared_ptr<ins_frame>> v_frame;

// 	if (!av_sync()) return v_frame;

// 	do 
// 	{
// 		if (b_audio_ && b_video_)
// 		{
// 			//队列同时有音视频帧存在，这样能保证音视频的时间间隔在最小，以便分段的时候音视频的起始时间相差最小
// 			if (video_queue_.empty() || audio_queue_.empty()) break;

// 			auto audio_frame = audio_queue_.front();
// 			auto video_frame = video_queue_.front();
// 			if (audio_frame->pts <= video_frame->pts)
// 			{
// 				audio_queue_.pop_front();
// 				v_frame.push_back(audio_frame);
// 			}
// 			else
// 			{
// 				video_queue_.pop_front();
// 				v_frame.push_back(video_frame);
// 			}
// 		}
// 		else if (b_video_)
// 		{
// 			if (video_queue_.empty()) break;
// 			auto frame = video_queue_.front();
// 			video_queue_.pop_front();
// 			v_frame.push_back(frame);
// 		}
// 		else if (b_audio_)
// 		{
// 			if (audio_queue_.empty()) break;
// 			auto frame = audio_queue_.front();
// 			audio_queue_.pop_front();
// 			v_frame.push_back(frame);
// 		}
// 	} while (0);

// 	if (!gps_queue_.empty() && start_pts_ != INS_PTS_NO_VALUE)
// 	{
// 		auto gps = gps_queue_.front();
// 		gps_queue_.pop_front();

// 		if (!gps_frame_) 
// 		{
// 			gps_frame_ = std::make_shared<ins_frame>();
// 			gps_frame_->buf = std::make_shared<insbuff>(camm_util::gps_size);
// 			gps_frame_->media_type = INS_MEDIA_CAMM;
// 		}
// 		camm_util::gen_camm_gps_packet(gps.get(), gps_frame_->buf);
// 		gps_frame_->dts = gps_frame_->pts = gps->timestamp*1000*1000;
// 		v_frame.push_back(gps_frame_);
// 	}

// 	if (!gyro_queue_.empty() && start_pts_ != INS_PTS_NO_VALUE)
// 	{
// 		auto gyro = gyro_queue_.front();
// 		gyro_queue_.pop_front();

// 		if (!gyro_frame_) 
// 		{
// 			gyro_frame_ = std::make_shared<ins_frame>();
// 			gyro_frame_->media_type = INS_MEDIA_CAMM_GYRO; //这是临时type，为了区分camm  gps
// 		}
// 		gyro_frame_->buf = gyro; //这个buff里放的和其他frame不一样，是没有处理的数据，还要处理一遍
// 		v_frame.push_back(gyro_frame_);
// 	}

// 	return v_frame;
// }
#endif



void stream_sink::do_discard_frame(bool b_keyframe)
{
	if (!b_keyframe) return;

	int32_t threshold = 0; 
	int32_t discard_cnt = 0;
	
	if (b_live_) {
		threshold = 2 * std::max((int32_t)fps_, 30); 	/* 最大缓存2s数据，超过开始丢帧 */
		if (video_queue_.size() > threshold) 
			discard_cnt = video_queue_.size() - threshold;
	} else {
		threshold = 5 * std::max((int32_t)fps_, 30); 	/* 最大缓存5s数据，超过开始丢帧 */
		if (video_queue_.size() > threshold) 
			discard_cnt = video_queue_.size() - threshold;
	}

	if (video_queue_.size() < threshold || discard_cnt <= 0)  return;

	for (int32_t i = 0; i < discard_cnt; i++) video_queue_.pop();

	LOGINFO("%s discard %d frames, left frames:%lu", url_.c_str(), discard_cnt, video_queue_.size());


#if 0
	/* 只丢视频帧不丢音频帧 */
	long long video_pts = video_queue_.front()->pts;

	while (!audio_queue_.empty()) {
		auto frame = audio_queue_.back();
		if (frame->pts <= video_pts) break;
		audio_queue_.pop_back();
	}

	if (!b_live_) {	//丢帧不停止录像PRO2
		if (++discard_frame_cnt_ > MAX_DISCARD_CNT) {
			LOGERR("%s discard frame times:%d, so stop record", url_.c_str(), discard_frame_cnt_);
			send_rec_over_msg(INS_ERR_UNSPEED_STORAGE);
		}
	}
#endif

}



void stream_sink::create_new_url()
{
	auto pos = url_.find(".mp4", 0);
    if (pos == std::string::npos) return;

	if (b_override_) {
		remove(url_.c_str());
	} else {
		if (url_sequence_ == -1) {
			url_sequence_ = 1;
			char str[5] = {0};
			sprintf(str, "_%03d", url_sequence_++);
			url_.replace(pos, 0, str);
		} else {
			char str[4] = {0};
			sprintf(str, "%03d", url_sequence_++);
			url_.replace(pos-3, 3, str);
		}
	}

	LOGINFO("new url:%s start_pts:%lld", url_.c_str(), start_pts_);
}


void stream_sink::do_net_disconnect()
{
	if (!b_live_) return;
	if (!b_auto_connect_) return;
	retry_cnt_++;
	if (auto_connect_cnt_ > 0 && retry_cnt_ > auto_connect_cnt_) {
		LOGINFO("reconnect cnt:%d fail close sink:%s", auto_connect_cnt_, url_.c_str());
		send_rec_over_msg(INS_ERR_NET_RECONECT);
		return;
	}

	LOGERR("waiting for reconnect sink:%s", url_.c_str());

	start_pts_ = INS_PTS_NO_VALUE;
	discard_frame_cnt_ = 0;

	if (timer_ == nullptr) {
		std::function<void()> func = [this]() 
		{
			do_reconnect();
		};
		timer_ = std::make_shared<ins_timer>(func, auto_connect_interval_/1000);
	} else {
		timer_->start(auto_connect_interval_/1000);
	}

	if (retry_cnt_ == 1) 
		send_net_link_state_msg(false);
}


void stream_sink::do_reconnect()
{
	/* 如果在析构了，就不要再重连了，会导致析构不了 */
	if (b_release_) return ;

	LOGINFO("reconnect %s ", url_.c_str());

	retry_cnt_ = 0;
	INS_THREAD_JOIN(th_);
	th_ = std::thread(&stream_sink::task, this);
}

int stream_sink::do_mux_error(bool b_open)
{
	if (b_open) {
		if (b_live_) {
			if (b_auto_connect_)  {
				do_net_disconnect();
				return INS_ERR;
			} else {
				send_rec_over_msg(INS_ERR_NET_CONNECT);
			}
		} else  {
			send_rec_over_msg(INS_ERR_MUX_OPEN);
		}
	} else {
		if (b_live_) {
			if (b_auto_connect_) {
				do_net_disconnect();
				return INS_ERR;
			}
			send_rec_over_msg(INS_ERR_NET_DISCONECT);
		} else {
			send_rec_over_msg(INS_ERR_MUX_WRITE);
		}
	}

	return INS_OK;
}


void stream_sink::send_rec_over_msg(int errcode)
{
	if (b_release_) return;
	
	json_obj obj;
	obj.set_string("name", INTERNAL_CMD_SINK_FINISH);
	obj.set_int("code", errcode);
	obj.set_bool("preview", b_preview_);
	access_msg_center::queue_msg(0, obj.to_string());
}


void stream_sink::send_net_link_state_msg(bool b_connect)
{
	if (b_release_) return;
	if (b_preview_) return;
	
	json_obj param_obj;
	if (b_connect) {
		param_obj.set_string("state", "connected");
	} else {
		param_obj.set_string("state", "connecting");
	}

	access_msg_center::send_msg(ACCESS_CMD_NET_LINK_STATE_, &param_obj);
}


int stream_sink::key_frame_to_jpeg(const std::shared_ptr<ins_frame>& frame)
{
	if (frame == nullptr) 
		return INS_ERR;
	
	if (video_param_ == nullptr) 
		return INS_ERR;

	/*
	 * 1.获取保存的sps, pps数据
	 */
	unsigned char* p = video_param_->config->data();
	unsigned int i = 0;
	for (i = 4; i < video_param_->config->size(); i++) {
		if (((p[i] == 0) && (p[i+1] == 0) && (p[i+2] == 0) && (p[i+3] == 1)) 
			|| ((p[i] == 0) && (p[i+1] == 0) && (p[i+2] == 1))) {
			break;
		}
	}

	RETURN_IF_TRUE(i >= video_param_->config->size(), "sps pps paser errror", INS_ERR);

	ffh264dec_param dec_param;
	dec_param.width = video_param_->width;
	dec_param.height = video_param_->height;
	dec_param.sps = video_param_->config->data();
	dec_param.sps_len = i;
	dec_param.pps = video_param_->config->data()+i;
	dec_param.pps_len = video_param_->config->size() - i;

	/*
	 * 2.解码最后一个关键帧
	 */
	ffh264dec dec;
	auto ret = dec.open(dec_param);
	RETURN_IF_NOT_OK(ret);
 	auto RGBA_frame = dec.decode(frame->page_buf->data(), frame->page_buf->size(), 0, 0);
	if (RGBA_frame == nullptr) {
		RGBA_frame = dec.decode(nullptr, 0, 0, 0);
	}
	RETURN_IF_TRUE(RGBA_frame == nullptr, "h264 key frame dec fail", INS_ERR);

	/*
	 * 3.解码后的数据jpeg编码
	 */
	tjpeg_enc enc;
	ret = enc.open();
	RETURN_IF_NOT_OK(ret);
	
	ins_img_frame jpg_frame;
	ret = enc.encode(*(RGBA_frame.get()), jpg_frame);
	RETURN_IF_NOT_OK(ret);

	std::string url = url_;
	auto pos = url.find(".mp4", 0);
	if (pos == std::string::npos) {
		LOGERR("url:%s not mp4", url.c_str());
		return INS_ERR;
	}
	
	/* 4.存成文件 */
	url.replace(pos, 4, ".jpg");
	std::ofstream file(url.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!file.is_open()) {
		LOGERR("file:%s open fail", url.c_str());
		return INS_ERR_FILE_OPEN;
	}

	file.write((const char*)jpg_frame.buff->data(), jpg_frame.buff->size());

	LOGINFO("last frame to file:%s", url.c_str());

	return INS_OK;
}


