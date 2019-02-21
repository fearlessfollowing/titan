
// #ifndef _INS_MEDIA_H_
// #define _INS_MEDIA_H_

// #define INS_MODE_PANORAMA 0
// #define INS_MODE_3D 1
// #define INS_SUBMODE_3D_TOP_LEFT 0
// #define INS_SUBMODE_3D_TOP_RIGHT 1

// #define INS_ALGORITHM_NORMAL 0
// #define INS_ALGORITHM_OPTICALFLOW 1

// #define INS_MEDIA_NONE      0
// #define INS_MEDIA_AUDIO     1
// #define INS_MEDIA_VIDEO     2
// #define INS_MEDIA_SUBTITLE  3

// #define INS_THUMBNAIL_WIDTH 1920
// #define INS_THUMBNAIL_HEIGHT 960

// #define INS_H264_MIME "video/avc"
// #define INS_H265_MIME "video/hevc"
// #define INS_AAC_MIME  "audio/mp4a-latm"
// #define INS_JPEG_MIME "jpeg"
// #define INS_RAW_MIME  "raw"

// #define INS_PIC_TYPE_RAW "raw"
// #define INS_PIC_TYPE_BURST "burst"
// #define INS_PIC_TYPE_HDR "hdr"
// #define INS_PIC_TYPE_TIMELAPSE "timelapse"
// #define INS_PIC_TYPE_JPEG "jpeg"

// #define INS_RAW_EXT ".dng"
// #define INS_JPEG_EXT ".jpg"

// #define INS_COLOR_FORMAT_SURFACE   0x7F000789
// #define INS_AAC_OBJ_LC                           2
// #define INS_H264_PROFILE_HIGH             0x08
// #define INS_H264_LEVEL_50                     0x4000
// #define INS_H264_LEVEL_40                     0x800

// #define INS_MC_KEY_PROFILE                     "profile"
// #define INS_MC_KEY_LEVEL                         "level"
// #define INS_MC_KEY_MIME                          "mime"
// #define INS_MC_KEY_WIDTH 		             "width"
// #define INS_MC_KEY_HEIGHT 		             "height"
// #define INS_MC_KEY_COLOR_FORMAT        "color-format"
// #define INS_MC_KEY_IFRMAE_INTERVAL     "i-frame-interval"
// #define INS_MC_KEY_FRAME_RATE              "frame-rate"
// #define INS_MC_KEY_BITRATE                      "bitrate"
// #define INS_MC_KEY_AAC_PROFILE              "aac-profile"
// #define INS_MC_KEY_SAMPLE_RATE             "sample-rate"
// #define INS_MC_KEY_CHANNEL_CNT            "channel-count"
// #define INS_MC_KEY_CSD0                            "csd-0"
// #define INS_MC_KEY_CSD1                     	  "csd-1"

// #define INS_PTS_NO_VALUE ((long long)0x8000000000000000)

// void ff_init();
// void threadpool_init();

// #define MC_CHECK_ERR_RETURN(status, err, msg) \
// if (status != OK) \
// { \
// 	LOGERR("%s %d", msg, status); \
// 	return err; \
// } 

// #define MC_CHECK_NULL_RETURN(status, err, msg) \
// if (status == nullptr) \
// { \
// 	LOGERR("%s", msg); \
// 	return err; \
// }

// #endif