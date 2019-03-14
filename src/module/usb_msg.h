
#ifndef _USB_MSG_H_
#define _USB_MSG_H_

#define USB_MAX_CMD_SIZE      (1024)

//amba camera cmd
#define USB_CMD_ID     (0x55)       /**< CMD group ID for usb control */
#define USB_CMD(x)     (((unsigned int)USB_CMD_ID << 24) | (x))

#define USB_CMD_START_VIDEO_RECORD               USB_CMD(0x00000010)
#define USB_CMD_STOP_VIDEO_RECORD                USB_CMD(0x00000011)
#define USB_CMD_VIDEO_FRAGMENT          		 USB_CMD(0x00000012)
#define USB_CMD_STILL_CAPTURE                    USB_CMD(0x00000020)
// #define USB_CMD_START_TIMELAPSE                  USB_CMD(0x00000022)
// #define USB_CMD_STOP_TIMELAPSE                   USB_CMD(0x00000023)
#define USB_CMD_GET_VIDEO_PARAM                  USB_CMD(0x00000030)
#define USB_CMD_SET_VIDEO_PARAM                  USB_CMD(0x00000031)
// #define USB_CMD_GET_LOG_MODE                     USB_CMD(0x00000032)
// #define USB_CMD_SET_LOG_MODE                     USB_CMD(0x00000033)
#define USB_CMD_GET_PHOTO_PARAM                  USB_CMD(0x00000040)
#define USB_CMD_SET_PHOTO_PARAM                  USB_CMD(0x00000041)
#define USB_CMD_GET_IMAGE_PROPERTY               USB_CMD(0x00000050)
#define USB_CMD_SET_IMAGE_PROPERTY               USB_CMD(0x00000051)
//#define USB_CMD_SET_3A_MODE                      USB_CMD(0x00000052)
//#define USB_CMD_GET_CAMERA_STATE                 USB_CMD(0x00000060)
//#define USB_CMD_RESET_CAMERA_STATE               USB_CMD(0x00000061)
#define USB_CMD_GET_SYSTEM_VERSION               USB_CMD(0x00000070)
#define USB_CMD_UPDATE_SYSTEM_START              USB_CMD(0x00000071)
#define USB_CMD_UPDATE_SYSTEM_COMPLETE           USB_CMD(0x00000072)
#define USB_CMD_REBOOT                           USB_CMD(0x00000073)
#define USB_CMD_SET_SYSTEM_TIME                  USB_CMD(0x00000080)
#define USB_CMD_GET_SYSTEM_TIME                  USB_CMD(0x00000081)
#define USB_CMD_RECEIVE_ERROR_CMD                USB_CMD(0x00000090)
#define USB_CMD_ENCRYPT_FAIL                     USB_CMD(0x00000091)
#define USB_CMD_REQ_RETRANSMIT                   USB_CMD(0x00000092)
#define USB_CMD_SET_CAMERA_PARAM                 USB_CMD(0x000000B0)
#define USB_CMD_GET_CAMERA_PARAM                 USB_CMD(0x000000B1)
#define USB_CMD_FORMAT_FLASH                     USB_CMD(0x000000F0)
#define USB_CMD_STORATE_SPEED_TEST_REQ           USB_CMD(0x000000F1)
#define USB_CMD_STORATE_SPEED_TEST_IND           USB_CMD(0x000000F2)
#define USB_CMD_STORAGE_STATE_IND                USB_CMD(0x000000F3)
#define USB_CMD_DELETE_FILE                      USB_CMD(0x000000F4)

#define USB_CMD_SET_UUID                         USB_CMD(0x000000A1) 
#define USB_CMD_GET_UUID                         USB_CMD(0x000000A2) 
#define USB_CMD_CALIBRATION_AWB_REQ              USB_CMD(0x000000A3)
#define USB_CMD_CALIBRATION_AWB_IND              USB_CMD(0x000000A4)
#define USB_CMD_CALIBRATION_BP_REQ               USB_CMD(0x000000A5)
#define USB_CMD_CALIBRATION_BP_IND               USB_CMD(0x000000A6)
#define USB_CMD_CALIBRATION_BPC_REQ              USB_CMD(0x000000A7)
#define USB_CMD_CALIBRATION_BPC_IND              USB_CMD(0x000000A8)
#define USB_CMD_CALIBRATION_CS_REQ               USB_CMD(0x000000A9)
#define USB_CMD_CALIBRATION_CS_IND               USB_CMD(0x000000AA)
#define USB_CMD_CALIBRATION_BLC_REQ              USB_CMD(0x000000AB)
#define USB_CMD_CALIBRATION_BLC_IND              USB_CMD(0x000000AC)
#define USB_CMD_CALIBRATION_BLC_RESET            USB_CMD(0x000000AD)
#define USB_CMD_SEND_DATA_RESULT_IND             USB_CMD(0x000000C0)

#define USB_CMD_TEST_SPI                         USB_CMD(0x00000100)
#define USB_CMD_TEST_CONNECTION                  USB_CMD(0x00000101)
#define USB_CMD_GET_LOG_FILE                     USB_CMD(0x00000102)
#define USB_CMD_CHANGE_USB_MODE                  USB_CMD(0x00000103)

#define USB_CMD_GYRO_CALIBRATION_REQ             USB_CMD(0x00000200)
#define USB_CMD_GYRO_CALIBRATION_IND             USB_CMD(0x00000201)
#define USB_CMD_MAGMETER_CALIBRATION_REQ         USB_CMD(0x00000202)
#define USB_CMD_MAGMETER_CALIBRATION_IND         USB_CMD(0x00000203)
#define USB_CMD_MAGMETER_CALIBRATION_RES         USB_CMD(0x00000204)

#define USB_CMD_VIG_MIN_VALUE_CHANGE             USB_CMD(0x00000301)
#define USB_CMD_VIG_MIN_VALUE_GET                USB_CMD(0x00000302)
#define USB_CMD_VIG_MIN_VALUE_SET                USB_CMD(0x00000303)

//JSON KEY
#define USB_MSG_KEY_CMD                           ("\"cmd\"")
#define USB_MSG_KEY_DATA                          ("\"data\"")

#define USB_MSG_KEY_WIDTH                         ("\"width\"")
#define USB_MSG_KEY_HEIGHT                        ("\"height\"")
#define USB_MSG_KEY_FRAMERATE                     ("\"framerate\"")
#define USB_MSG_KEY_BITRATE                       ("\"bitrate\"")
#define USB_MSG_KEY_LOG_MODE                      ("\"log_mode\"")
#define USB_MSG_KEY_FLICKER                       ("\"flicker\"")
#define USB_MSG_KEY_MIME                          ("\"mime\"")     //h264/h265
#define USB_MSG_KEY_BITDEPTH                      ("\"bitdepth\"") //8/10

#define USB_MSG_KEY_HDR                         ("\"hdr\"")
#define USB_MSG_KEY_RAW                         ("\"raw\"")

#define USB_MSG_KEY_SEC_STREAM                    ("\"sec_stream\"")

#define USB_MSG_KEY_FILE_STREAM                   ("\"file_stream\"")
#define USB_MSG_KEY_FILE_URL                      ("\"file_url\"")
#define USB_MSG_KEY_RAW_URL                       ("\"raw_url\"")
#define USB_MSG_KEY_USB_STREAM                    ("\"usb_stream\"")

#define USB_MSG_KEY_IQ_TYPE                     ("\"iq_type\"")
#define USB_MSG_KEY_IQ_INDEX                    ("\"iq_index\"")

#define USB_MSG_KEY_EV_BIAS                      ("\"ev_bias\"")                   //（-255:255）
#define USB_MSG_KEY_AE_METER                     ("\"ae_meter\"")                  //（0:2）
#define USB_MSG_KEY_WB                           ("\"wb\"")                        //（0:14）
#define USB_MSG_KEY_BRIGHTNESS              	 ("\"brightness\"")                //（-255:255）
#define USB_MSG_KEY_CONTRAST                     ("\"contrast\"")                  //（0:255）
#define USB_MSG_KEY_SATURATION                   ("\"saturation\"")                //（0:255）
#define USB_MSG_KEY_HUE                          ("\"hue\"")                       //（-15:15）
#define USB_MSG_KEY_SHARPNESS                    ("\"sharpness\"")                 //（0:6）
#define USB_MSG_KEY_DIGITAL_EFFECT               ("\"dig_effect\"")                //（0:8）
#define USB_MSG_KEY_FLICKER                      ("\"flicker\"")                   //（0:2）
#define USB_MSG_KEY_ISO                          ("\"iso_value\"")                 //（0:9）
#define USB_MSG_KEY_SHUTTER                      ("\"shutter_value\"")             //（0:43）

//amba camera state
#define AMBA_STATE_IDLE  "idle"                        //表示空闲
#define AMBA_STATE_RECORDING "recording"                    //正在录像
#define AMBA_STATE_STILL_CAPTURING "stillcapture"              //正在拍照
#define AMBA_STATE_TIMELAPSE "timelapse"
#define AMBA_STATE_ERROR "error"                        //系统出错 

#pragma pack(1) // 自定义1字节对齐
struct amba_frame_info { 
	int syncword;
	unsigned int type;
	unsigned int size;  
	unsigned int sequence;  
	long long timestamp;
	int reserve1; // 0:正常 1：正常結束 -2：卡滿結束
	int reserve2;
};

struct amba_video_extra {
	uint16_t type = 0;
	uint16_t count = 0;
};

#pragma pack() //取消自定义对齐

//__attribute__((packed))

#define AMBA_FLAG_END 1
#define AMBA_ERR_NO_STORAGE_SPACE -2

//frame type
#define AMBA_FRAME_PIC     1
#define AMBA_FRAME_IDR     2
#define AMBA_FRAME_I       3
#define AMBA_FRAME_B       4
#define AMBA_FRAME_P       5
#define AMBA_FRAME_UP_DATA 6
#define AMBA_FRAME_UP_MD5  7
#define AMBA_FRAME_CB_AWB  8
#define AMBA_FRAME_CB_BAD  9
#define AMBA_FRAME_CB_BRIGHT 10
#define AMBA_FRAME_CB_BLACK  11
#define AMBA_FRAME_UUID      12
#define AMBA_FRAME_PIC_INFO  13
#define AMBA_FRAME_PIC_RAW   14
#define AMBA_FRAME_CB_BLC    15
#define AMBA_FRAME_CB_VIG_MAXVALUE  16
#define AMBA_FRAME_LOG  17
#define AMBA_FRAME_GAMMA_CURVE 18
#define AMBA_FRAME_FILE_DIRS 19

//#define AMBA_FRAME_GYRO 20
#define AMBA_FRAME_MAGMETER 21

#define AMBA_FRAME_AUDIO_OPT  100
#define AMBA_FRAME_AUDIO_DATA 101
#define AMBA_FRAME_GPS_DATA   110
#define AMBA_FRAME_PRJ_INFO   120

#define AMBA_EXTRA_EXPOSURE 1
#define AMBA_EXTRA_GYRO     2
#define AMBA_EXTRA_EXIF     3
#define AMBA_EXTRA_GYRO_EXT 4
#define AMBA_EXTRA_TEMP 5

// #define USB_IF_NOT_OK_RETURN(ret) \
// if (ret == LIBUSB_ERROR_NULL_PACKET) \
// { \
// 	return INS_ERR_RETRY; \
// } \
// else if (ret != LIBUSB_SUCCESS) \
// { \
// 	exception_ = INS_ERR_CAMERA_READ_DATA; \
// 	return exception_; \
// }

#endif