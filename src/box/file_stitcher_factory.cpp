#include "file_stitcher_factory.h"
#include "file_img_stitcher.h"
#include "file_burst_stitcher.h"
#include "file_video_stitcher.h"
#include "file_hdr_stitcher.h"
#include "file_raw_stitcher.h"
#include "inslog.h"
#include "common.h"

std::shared_ptr<file_stitcher> file_stitcher_factory::create_stitcher(std::string type)
{
    std::shared_ptr<file_stitcher> stitcher;

    if (type == INS_VIDEO_TYPE)
    {
        stitcher = std::make_shared<file_video_stitcher>();
    }
    else if (type == INS_PIC_TYPE_PHOTO)
    {
        stitcher = std::make_shared<file_img_stitcher>();
    }
    else if (type == INS_PIC_TYPE_BURST || type == INS_PIC_TYPE_TIMELAPSE)
    {
        stitcher = std::make_shared<file_burst_stitcher>();
    }
    else if (type == INS_PIC_TYPE_HDR)
    {
        stitcher = std::make_shared<file_hdr_stitcher>();
    }
    // else if (type == INS_PIC_TYPE_RAW)
    // {
    //     stitcher = std::make_shared<file_raw_stitcher>();
    // }
    else
    {
        LOGERR("unsupport type:%s", type.c_str());
    }

    return stitcher;
}