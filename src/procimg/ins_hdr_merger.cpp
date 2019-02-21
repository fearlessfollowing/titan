#include "ins_hdr_merger.h"
#include "ins_clock.h"
#include "common.h"
#include "inslog.h"
#include <opencv2/photo.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;

int32_t ins_hdr_merger(const std::vector<std::string>& in_file, ins_img_frame& out_img)
{
    try
    {
        ins_clock clock;
        std::vector<Mat> v_img;
        for (uint32_t i = 0; i < in_file.size(); i++)
        {
            Mat img = imread(in_file[i]);
            v_img.push_back(img);
        }

        Mat fusion;
        Ptr<MergeMertens> merge_mertens = createMergeMertens();
        merge_mertens->process(v_img, fusion);
        fusion.convertTo(fusion, CV_8UC3, 255);

        out_img.w = fusion.cols;
        out_img.h = fusion.rows;
        out_img.buff = std::make_shared<insbuff>(fusion.cols*fusion.rows*3);
        memcpy(out_img.buff->data(), fusion.data, out_img.buff->size());

        LOGINFO("hdr imaging time:%lf", clock.elapse());
    }
    catch (...)
    {
        LOGERR("ins_hdr_merger catch error");
        return INS_CATCH_EXCEPTION;
    }

    return INS_OK;
}