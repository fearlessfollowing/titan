
#include "access_state.h"
#include "access_msg_center_head.h"
#include "access_msg.h"
#include <iostream>
#include <sstream>
#include "common.h"
#include "inslog.h"
#include "json_obj.h"

void access_state::set_time()
{
    gettimeofday(&start_time_, nullptr);
}

long long access_state::get_elapse_usec()
{
	struct timeval end_time;
	gettimeofday(&end_time, nullptr);
	long long elapse = end_time.tv_sec*1000000 - start_time_.tv_sec*1000000 + end_time.tv_usec - start_time_.tv_usec;
	return elapse;
}


void access_state::get_state_param(int state, json_obj& root_obj) const
{
	json_obj ori_obj;
	json_obj aud_obj;
	json_obj sti_obj;
	json_obj pre_obj;
	std::string operation;

	if (state & CAM_STATE_LIVE || state & CAM_STATE_RECORD) {
		ori_obj.set_int("width", option_.origin.width);
		ori_obj.set_int("height", option_.origin.height);
		ori_obj.set_int("framerate", option_.origin.framerate);
		ori_obj.set_int("bitrate", option_.origin.bitrate);
		ori_obj.set_bool("saveOrigin",  option_.origin.storage_mode != INS_STORAGE_MODE_NONE);
		ori_obj.set_string("mime", option_.origin.mime);
		ori_obj.set_int("logMode", option_.origin.logmode);
		ori_obj.set_bool("hdr", option_.origin.hdr);

		if (option_.b_audio) {
			aud_obj.set_string("mime", "aac");
			aud_obj.set_string("sampleFormat", option_.audio.samplefmt);
			aud_obj.set_string("channelLayout", option_.audio.ch_layout);
			aud_obj.set_int("samplerate", option_.audio.samplerate);
			aud_obj.set_int("bitrate", option_.audio.bitrate);
		}

		std::string stiching_url;
		if (state & CAM_STATE_RECORD) {
			operation = "record";
			stiching_url = option_.path;
		} else {
			operation = "live";
			stiching_url = option_.stiching.url;
		}
		
		struct timeval end_time;
		gettimeofday(&end_time, nullptr);
		int time_past = end_time.tv_sec - start_time_.tv_sec;

		if (option_.b_stiching) {
			sti_obj.set_int("width", option_.stiching.width);
			sti_obj.set_int("height", option_.stiching.height);
			sti_obj.set_int("framerate", option_.stiching.framerate);
			sti_obj.set_int("bitrate", option_.stiching.bitrate);
			sti_obj.set_string("mime", option_.stiching.mime);
			sti_obj.set_string("mode", mode_to_str(option_.stiching.mode));

			/*
			 * map_type - Add by skymixos 2019年3月5日
			 * "cube", "flat"
			 */
			sti_obj.set_string("map", (option_.stiching.map_type == INS_MAP_FLAT) ? "flat" : "cube");


			/* 网络直播和hdmi直播可以同时开启 */
			if (option_.stiching.hdmi_display) {
				sti_obj.set_bool("liveOnHdmi", true);
			}
			
			if (stiching_url != "") {
				sti_obj.set_string("url", stiching_url);
			}
		} else {
			sti_obj.set_string("url", option_.path);
		}

		sti_obj.set_bool("stabilization", option_.b_stabilization);
		sti_obj.set_int("timePast", time_past);

		if (option_.duration > 0) {
			sti_obj.set_int("timeLeft", option_.duration - time_past);
		}

		if (state & CAM_STATE_LIVE) {
			sti_obj.set_bool("liveRecording", b_live_rec_);
		}

		if (option_.timelapse.enable) {
			json_obj timelapse_obj;
			timelapse_obj.set_bool("enalbe", true);
			timelapse_obj.set_int("interval", option_.timelapse.interval);
			sti_obj.set_obj("timelapse", &timelapse_obj);
		}
	}

	if (state & CAM_STATE_PREVIEW) {
		if (ori_obj.length() <= 0) {
			ori_obj.set_int("width", preview_opt_.origin.width);
			ori_obj.set_int("height", preview_opt_.origin.height);
			ori_obj.set_int("framerate", preview_opt_.origin.framerate);
			ori_obj.set_int("bitrate", preview_opt_.origin.bitrate);
			ori_obj.set_string("mime", preview_opt_.origin.mime);
			ori_obj.set_int("logMode", preview_opt_.origin.logmode);
			ori_obj.set_bool("hdr", preview_opt_.origin.hdr);
		}

		pre_obj.set_int("width", preview_opt_.stiching.width);
		pre_obj.set_int("height", preview_opt_.stiching.height);
		pre_obj.set_int("framerate", preview_opt_.stiching.framerate);
		pre_obj.set_int("bitrate", preview_opt_.stiching.bitrate);
		pre_obj.set_string("mime", preview_opt_.stiching.mime);
		pre_obj.set_string("mode", mode_to_str(preview_opt_.stiching.mode));
		pre_obj.set_string("url", preview_opt_.stiching.url);
		pre_obj.set_bool("stabilization", preview_opt_.b_stabilization);
	}

	if (ori_obj.length() > 0) {
		root_obj.set_obj("origin", &ori_obj);
	}

	if (aud_obj.length() > 0) {
		root_obj.set_obj("audio", &aud_obj);
	}

	if (sti_obj.length() > 0) {
		root_obj.set_obj(operation.c_str(), &sti_obj);
	}

	if (pre_obj.length() > 0) {
		root_obj.set_obj("preview", &pre_obj);
	}
}

std::string access_state::mode_to_str(int mode) const
{
	if (mode == INS_MODE_PANORAMA) {
		return ACCESS_MSG_OPT_VRMODE_PANO;
	} else if (mode == INS_MODE_3D_TOP_LEFT) {
		return ACCESS_MSG_OPT_VRMODE_3D_TOP_LEFT;
	} else {
		return ACCESS_MSG_OPT_VRMODE_3D_TOP_RIGHT;
	}
}

std::string access_state::state_to_string(int state) const
{
	if (state == CAM_STATE_IDLE) {
		return CAM_STATE_STR_IDLE;
	}

	std::string str;
	if (state & CAM_STATE_PREVIEW) {
		str = str + CAM_STATE_STR_PREVIEW + " ";
	}

	if (state & CAM_STATE_LIVE) {
		str = str + CAM_STATE_STR_LIVE + " ";
	}

	if (state & CAM_STATE_RECORD) {
		str = str + CAM_STATE_STR_RECORD + " ";
	}

	if (state & CAM_STATE_STORAGE_ST) {
		str = str + CAM_STATE_STR_STORAGE_ST + " ";
	}

	if (state & CAM_STATE_GYRO_CALIBRATION) {
		str = str + CAM_STATE_STR_GYRO_CALIBRATION + " ";
	}

	if (state & CAM_STATE_MAGMETER_CAL) {
		str = str + CAM_STATE_STR_MAGMETER_CALIBRATION + " ";
	}

	if (state & CAM_STATE_CALIBRATION) {
		str = str + CAM_STATE_STR_CALIBRATION + " ";
	}

	if (state & CAM_STATE_PIC_SHOOT) {
		str = str + CAM_STATE_STR_PIC_SHOOT + " ";
	}

	if (state & CAM_STATE_PIC_PROCESS) {
		str = str + CAM_STATE_STR_PIC_PROCESS + " ";
	}

	if (state & CAM_STATE_QRCODE_SCAN) {
		str = str + CAM_STATE_STR_QR_SCAN + " ";
	}

	if (state & CAM_STATE_STOP_RECORD) {
		str = str + CAM_STATE_STR_STOP_REC + " ";
	}

	if (state & CAM_STATE_STOP_LIVE) {
		str = str + CAM_STATE_STR_STOP_LIVE + " ";
	}

	if (state & CAM_STATE_AUDIO_CAPTURE) {
		str = str + CAM_STATE_STR_AUDIO_CAP + " ";
	}

	if (state & CAM_STATE_STITCHING_BOX) {
		str = str + CAM_STATE_STR_STITCHING_BOX + " ";
	}

	if (state & CAM_STATE_BLC_CAL) {
		str = str + CAM_STATE_STR_BLC_CAL + " ";
	}

	if (state & CAM_STATE_BPC_CAL) {
		str = str + CAM_STATE_STR_BPC_CAL + " ";
	}

	if (state & CAM_STATE_MODULE_STORAGE) {
		str = str + CAM_STATE_STR_MODULE_STORAGE + " ";
	}

	if (state & CAM_STATE_DELETE_FILE) {
		str = str + CAM_STATE_STR_DELETE_FILE + " ";
	}

	if (state & CAM_STATE_MODULE_POWON) {
		str = str + CAM_STATE_STR_MODULE_POWERON + " ";
	}

	if (state & CAM_STATE_UDISK_MODE) {
		str = str + CAM_STATE_STR_UDISK_MODE + " ";
	}

	if (str.size() <= 0) {
		str = CAM_STATE_STR_IDLE;
	} else {	// 去掉最后的一个空格
		str.erase(str.size()-1, 1);
	}

	return str;
}

