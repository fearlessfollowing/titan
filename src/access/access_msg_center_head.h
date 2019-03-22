
#ifndef _ACCESS_MSG_CENTER_HEAD_H_
#define _ACCESS_MSG_CENTER_HEAD_H_

#define CAM_STATE_IDLE 				0
#define CAM_STATE_PREVIEW 			(1 << 0)
#define CAM_STATE_RECORD 			(1 << 1)
#define CAM_STATE_LIVE 				(1 << 2)
//#define CAM_STATE_TAKE_PICTURE 	(1 << 6)
#define CAM_STATE_QRCODE_SCAN 		(1 << 7)
#define CAM_STATE_STORAGE_ST 		(1 << 9)
#define CAM_STATE_GYRO_CALIBRATION 	(1 << 10)
#define CAM_STATE_CALIBRATION 		(1 << 11)
#define CAM_STATE_AUDIO_CAPTURE 	(1 << 12)
#define CAM_STATE_STOP_RECORD 		(1 << 13)
#define CAM_STATE_STOP_LIVE 		(1 << 14)
#define CAM_STATE_STITCHING_BOX 	(1 << 15)
#define CAM_STATE_BLC_CAL 			(1 << 16)
#define CAM_STATE_PIC_SHOOT 		(1 << 17)
#define CAM_STATE_PIC_PROCESS 		(1 << 18)
#define CAM_STATE_MODULE_STORAGE 	(1 << 19)
#define CAM_STATE_BPC_CAL 			(1 << 20)
#define CAM_STATE_MAGMETER_CAL 		(1 << 21)
#define CAM_STATE_DELETE_FILE 		(1 << 22)
#define CAM_STATE_MODULE_POWON 		(1 << 23)
#define CAM_STATE_UDISK_MODE 		(1 << 24)

#define CAM_STATE_STR_IDLE 					"idle"
#define CAM_STATE_STR_PREVIEW 				"preview"
#define CAM_STATE_STR_LIVE 					"live"
#define CAM_STATE_STR_RECORD 				"record"
#define CAM_STATE_STR_STORAGE_ST 			"storage_speed_test"
#define CAM_STATE_STR_GYRO_CALIBRATION 		"gyro_calibration"
#define CAM_STATE_STR_MAGMETER_CALIBRATION 	"magmeter_calibration"
#define CAM_STATE_STR_CALIBRATION 			"calibration"
//#define CAM_STATE_STR_TAKE_PICTURE 		"takePicture"
#define CAM_STATE_STR_PIC_SHOOT 			"pic_shoot"
#define CAM_STATE_STR_PIC_PROCESS 			"pic_process"
#define CAM_STATE_STR_STITCHING_BOX 		"stitching_box"
#define CAM_STATE_STR_BLC_CAL 				"blc_calibration"
#define CAM_STATE_STR_BPC_CAL 				"bpc_calibration"
#define CAM_STATE_STR_QR_SCAN 				"qrscan"
#define CAM_STATE_STR_STOP_REC 				"stop_rec"
#define CAM_STATE_STR_STOP_LIVE 			"stop_live"
#define CAM_STATE_STR_AUDIO_CAP 			"audio_capture"
#define CAM_STATE_STR_MODULE_STORAGE 		"module_storage"
#define CAM_STATE_STR_DELETE_FILE 			"deleteFile"
#define CAM_STATE_STR_MODULE_POWERON 		"module_power_on"
#define CAM_STATE_STR_UDISK_MODE 			"udisk_mode"

#define INVALID_UUID "ffffffffffffffffffffffffffffffff"

#define DECLARE_AND_DO_WHILE_0_BEGIN \
int ret = INS_OK; \
json_obj res_obj; \
do \
{

#define DO_WHILE_0_END } while(0);


#define RETURN_NOT_IN_STATE(state) \
if (!(state & state_)) \
{ \
	LOGERR("cam not in %s state", state_mgr_.state_to_string(state).c_str()); \
	return INS_OK; \
}

#define BREAK_NOT_IN_STATE(state) \
if (!(state & state_)) \
{ \
	LOGERR("cam not in %s state", state_mgr_.state_to_string(state).c_str()); \
	ret =  INS_ERR_NOT_ALLOW_OP_IN_STATE; \
	break; \
}

#define BREAK_NOT_IN_IDLE() \
if (state_ != CAM_STATE_IDLE) \
{ \
	LOGERR("cam not in idle state"); \
	ret = INS_ERR_NOT_ALLOW_OP_IN_STATE; \
	break; \
}

#define BREAK_EXCPET_STATE(state) \
if (state_ & (~state)) \
{ \
	ret = INS_ERR_NOT_ALLOW_OP_IN_STATE; \
	break; \
}

#define BREAK_EXCPET_STATE_2(state_1, state_2) \
if (state_ & (~state_1) & (~state_2))  \
{ \
	ret = INS_ERR_NOT_ALLOW_OP_IN_STATE; \
	break; \
} 

#define OPEN_CAMERA_IF_ERR_RETURN(index) \
{ \
	int err = open_camera(index); \
	if (INS_OK != err) \
	{ \
		return err; \
	} \
}

#define OPEN_CAMERA_IF_ERR_BREAK(index) \
{ \
	ret = open_camera(index); \
	if (INS_OK != ret) \
	{ \
		break; \
	} \
}

#define BREAK_IF_CAM_NOT_OPEN(err) \
if (camera_ == nullptr) \
{ \
	LOGERR("camera not open"); \
	ret = err; \
	break; \
} 

#define RSP_ERR_NOT_IN_BOX_STATE \
if (state_ != CAM_STATE_STITCHING_BOX || task_mgr_ == nullptr) \
{ \
	sender_->send_rsp_msg(sequence, INS_ERR_NOT_ALLOW_OP_IN_STATE, cmd); \
	return; \
} 

#endif