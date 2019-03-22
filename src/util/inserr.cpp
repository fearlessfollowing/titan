
#include "inserr.h"

std::string inserr_to_str(int err)
{
    switch (err) {
        case INS_OK:
            return "success";
        case INS_ERR_INVALID_MSG_FMT:
            return "invalid message format";
        case INS_ERR_INVALID_MSG_PARAM:
            return "invalid message param";
        case INS_ERR_CONFIG_FILE:
            return "config file error";
        case INS_ERR_NOT_ALLOW_OP_IN_STATE:
            return "operation not allowed in this state";
        case INS_ERR_FILE_OPEN:
            return "file open fail";
        case INS_ERR_CAMERA_OPEN:
            return "module open fail";
        case INS_ERR_CAMERA_NOT_OPEN:
            return "module not open";
        case INS_ERR_CAMERA_READ_CMD:
            return "recv command/rspone from module fail";
        case INS_ERR_CAMERA_WRITE_CMD:
            return "send command to module fail";
        case INS_ERR_CAMERA_WRITE_DATA:
            return "send data to module fail";
        case INS_ERR_CAMERA_READ_DATA:
            return "read data from module fail";
        case INS_ERR_CAMERA_OFFLINE:
            return "module offline";
        case INS_ERR_CAMERA_FRAME_NOT_SYNC:
            return "module frame not sync";
        case INS_ERR_FILE_IO:
            return "file io error";
        case INS_ERR_NO_STORAGE_SPACE:
            return "no storage space left";
        case INS_ERR_NO_STORAGE_DEVICE:
            return "no storage device";
        case INS_ERR_MICPHONE_EXCEPTION:
            return "micphone exception";
        case INS_ERR_UNSPEED_STORAGE:
            return "storage speed insufficient";
        case INS_ERR_MUX_WRITE:
            return "mux write fail";
        case INS_ERR_MUX_OPEN:
            return "mux open fail";
        case INS_ERR_NET_CONNECT:
            return "network connect fail";
        case INS_ERR_NET_DISCONECT:
            return "network disconnect";
        case INS_ERR_NET_RECONECT:
            return "network reconnect fail";
        case INS_ERR_TEMPERATURE_HIGH:
            return "temperature high";
        case INS_ERR_BUSY:
            return "camera service is busying";
        case INS_ERR_USB_SND_UNSUPPORT_A_GAIN:
            return "usb audio device unsupport adjust gain";
        case INS_ERR_PRJ_FILE_NOT_FOND:
            return "project file not found";
        case INS_ERR_PRJ_FILE_DAMAGE:
            return "project file damaged";
        case INS_ERR_TASK_NOT_FOND:
            return "task not found";
        case INS_ERR_TASK_ALREADY_EXIST:
            return "task already exist";
        case INS_ERR_TASK_IS_RUNNING:
            return "task is running";
        case INS_ERR_TASK_DATABASE_DAMAGE:
            return "task database damaged";
        case INS_ERR_ORIGIN_VIDEO_DAMAGE:
            return "origin video damaged";
        case INS_ERR_GYRO_DATA_DAMAGE:
            return "gyro data damage";
        case INS_ERR_CODEC_DIE:
            return "codec die";
        case INS_ERR_NO_MEM:
            return "no memory";
        case INS_ERR_DECODE:
            return "decode fail";
        case INS_ERR_ENCODE:
            return "encode fail";
        case INS_ERR_NOT_IMPLEMENT:
            return "not implement";
        case INS_CATCH_EXCEPTION:
            return "catch exception";
        case INS_ERR_NOT_EXIST:
            return "not exist";
        case INS_ERR_COMPASS_STILL:
            return "compass device is still";
        //module error code
        case INS_ERR_M_INVALID_MSG:
            return "module invalid msg";
        case INS_ERR_M_ENCRYPT_FAIL:
            return "module entry fail";
        case INS_ERR_M_SYNC_FAIL:
            return "module sync fail";
        case INS_ERR_M_RTRANSMIT_FAIL:
            return "module retransmit fail";
        case INS_ERR_M_NO_SDCARD:
            return "module no sdcard";
        case INS_ERR_M_NO_SPACE:
            return "module no storage space";
        case INS_ERR_M_STORAGE_IO:
            return "module storage io error";
        case INS_ERR_M_UNSPEED_STORAGE:
            return "module unspeed storage";
        case INS_ERR_M_EXCEED_FILE_SIZE:
            return "module exceed file size limit";
        default:
            return "unknow error";
    }
}