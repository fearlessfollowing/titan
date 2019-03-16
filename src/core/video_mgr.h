#ifndef _VIDEO_MGR_H_
#define _VIDEO_MGR_H_

#include <stdint.h>
#include <memory>
#include "access_msg_option.h"
#include "cam_data.h"

class cam_manager;
class video_composer;   
class all_cam_video_buff;
class audio_mgr;
class stream_sink;
class usb_sink;
class cam_video_param;
class stabilization;

class video_mgr   {
public:
    			~video_mgr();   
    int32_t 	start(cam_manager* camera, const ins_video_option& option);
    int32_t 	start_preview(ins_video_option option);
    void 		stop_preview();
    void 		stop_live_file();	//停止直播时候的文件存储
    int32_t 	get_snd_type();
    void 		set_first_frame_ts(int32_t rec_seq, int64_t ts);
    void 		notify_video_fragment(int32_t sequence);
    void 		switch_stablz(bool enable);
    bool 		get_stablz_status() { return b_stablz_; };

private:
    int32_t 	open_camera_rec(const ins_video_option& option, bool storage_aux);
    void 		open_audio(const ins_video_option& option);
    void 		open_usb_sink(std::string prj_path);
    void 		open_local_sink(std::string path);
    void 		open_origin_stream(const ins_video_option& option);
    int32_t 	open_composer(const ins_video_option& option, bool preview);
    int32_t 	open_preview(const ins_video_option& option);
    void 		setup_stablz();
    void 		audio_vs_sink(std::shared_ptr<stream_sink>& sink, uint32_t index, bool is_origin);
    void 		send_snd_state_msg(int32_t type, std::string name, bool b_spatial);
    void 		print_option(const ins_video_option& option) const;
	
    cam_manager* 						camera_ = nullptr;
    std::shared_ptr<audio_mgr> 			audio_;
    std::shared_ptr<video_composer> 	composer_;
    std::shared_ptr<all_cam_video_buff> video_buff_;
    std::shared_ptr<cam_video_param> 	aux_param_;
    cam_video_param 					video_param_;
    bool 								b_frag_ = false;        /* 视频是否分段 */
    int32_t 							audio_type_ = 0;        /* 音频设备的类型 */
    std::string 						prj_path_;              /* 工程文件的路径 */
    std::shared_ptr<usb_sink> 			usb_sink_;
    std::shared_ptr<stream_sink> 		local_sink_;
    std::shared_ptr<stabilization> 		stablz_;
    bool 								b_stablz_ = false;
    static int32_t 						rec_seq_;
};

#endif