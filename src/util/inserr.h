
#ifndef _INS_ERROR_H_
#define _INS_ERROR_H_

#include <string>

#define INS_UNSED 1
#define INS_OK    0  
#define INS_ERR   -1
#define INS_ERR_TIME_OUT -2
#define INS_ERR_OVER   -3
#define INS_ERR_RETRY   -4

//乱
#define INS_ERR_INVALID_MSG_FMT 		-410		/* 错误的消息格式 */
#define INS_ERR_INVALID_MSG_PARAM 		-411		/* 错误的消息参数 */
#define INS_ERR_CONFIG_FILE 			-412		/* 配置文件错误 */
#define INS_ERR_NOT_ALLOW_OP_IN_STATE 	-413 		/* 操作不允许 */
#define INS_ERR_MICPHONE_EXCEPTION 		-414		/* 麦克风异常 */
#define INS_ERR_NO_GYRO_DATA 			-416		/* 无陀螺仪数据 */
#define INS_ERR_TEMPERATURE_HIGH 		-417		/* 温度过高 */
#define INS_ERR_BUSY 					-418		/* 系统忙 */
#define INS_ERR_USB_SND_UNSUPPORT_A_GAIN -419

#define INS_ERR_NOT_IMPLEMENT -420
#define INS_CATCH_EXCEPTION -421
#define INS_ERR_NOT_EXIST -422
#define INS_ERR_COMPASS_STILL -423

//存储相关
#define INS_ERR_FILE_OPEN -430
#define INS_ERR_FILE_IO -431
#define INS_ERR_NO_STORAGE_SPACE -432
#define INS_ERR_NO_STORAGE_DEVICE -433
#define INS_ERR_UNSPEED_STORAGE -434

//网络相关
#define INS_ERR_MUX_WRITE 		-435
#define INS_ERR_MUX_OPEN 		-436
#define INS_ERR_NET_CONNECT 	-437
#define INS_ERR_NET_RECONECT 	-438
#define INS_ERR_NET_DISCONECT 	-439

//媒体相关
#define INS_ERR_CODEC_DIE -450
#define INS_ERR_DECODE -451
#define INS_ERR_ENCODE -452
#define INS_ERR_GPU -453

//模组相关
#define INS_ERR_CAMERA_OPEN 			-460
#define INS_ERR_CAMERA_NOT_OPEN 		-461
#define INS_ERR_CAMERA_READ_CMD 		-462
#define INS_ERR_CAMERA_WRITE_CMD 		-463
#define INS_ERR_CAMERA_READ_DATA 		-464
#define INS_ERR_CAMERA_WRITE_DATA 		-465
#define INS_ERR_CAMERA_OFFLINE 			-466
#define INS_ERR_CAMERA_FRAME_NOT_SYNC 	-467


//box数据相关
#define INS_ERR_PRJ_FILE_NOT_FOND 		-470
#define INS_ERR_PRJ_FILE_DAMAGE 		-471
#define INS_ERR_TASK_DATABASE_DAMAGE 	-472
#define INS_ERR_ORIGIN_VIDEO_DAMAGE 	-473
#define INS_ERR_GYRO_DATA_DAMAGE 		-474

//box任务相关
#define INS_ERR_TASK_NOT_FOND -480
#define INS_ERR_TASK_ALREADY_EXIST -481
#define INS_ERR_TASK_IS_RUNNING -482

//系统相关
#define INS_ERR_NO_MEM -490

//模组错误码
#define INS_M_OK 1
#define INS_ERR_M_INVALID_MSG -301
#define INS_ERR_M_ENCRYPT_FAIL -302
#define INS_ERR_M_SYNC_FAIL -303
#define INS_ERR_M_RTRANSMIT_FAIL -304

#define INS_ERR_M_NO_SDCARD -310
#define INS_ERR_M_NO_SPACE -311
#define INS_ERR_M_STORAGE_IO -312
#define INS_ERR_M_UNSPEED_STORAGE -313
#define INS_ERR_M_EXCEED_FILE_SIZE -314

std::string inserr_to_str(int err);

#endif