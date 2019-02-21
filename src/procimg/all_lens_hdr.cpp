
#include "all_lens_hdr.h"
#include "hdr.h"
#include "inslog.h"
#include "common.h"
#include "ins_clock.h"
#include <future>

using namespace cv;

std::vector<ins_img_frame> all_lens_hdr::process(const std::vector<std::vector<std::string>>& vv_file)
{
    assert(vv_file.size() == INS_CAM_NUM);

    ins_clock t;
    std::future<ins_img_frame> f[INS_CAM_NUM];
    for (uint32_t i = 0; i < vv_file.size(); i++)
    {
        f[i] = std::async(std::launch::async, &all_lens_hdr::single_group_process, this, vv_file[i]);
    }

    std::vector<ins_img_frame> v_hdr_frame;
    for (uint32_t i = 0; i < INS_CAM_NUM; i++)
    {
        auto frame = f[i].get();
        if (frame.buff) v_hdr_frame.push_back(frame);
    }

    LOGINFO("HDR time:%lf", t.elapse());

    return v_hdr_frame;
}

ins_img_frame all_lens_hdr::single_group_process(const std::vector<std::string>& v_file)
{
    ins_img_frame hdr_frame;
    std::vector<Mat> v_dec_img;
    for (uint32_t i = 0; i < v_file.size(); i++)
    {
        //LOGINFO("imread:%s", v_file[i].c_str());
        auto img = imread(v_file[i]);
        if (img.empty())
        {
            LOGERR("imread:%s fail", v_file[i].c_str()); 
            return hdr_frame;
        }
        v_dec_img.push_back(img);
    }

    Mat hdr_img;
    hdr::HDR hdr_process;
    if (v_dec_img.size() == 3) //3张调用这个接口效果要好些,有优化
    {
        hdr_process.runHDR(v_dec_img[0], v_dec_img[1], v_dec_img[2], hdr_img);
    }
    else
    {
        hdr_process.runHDR(v_dec_img, hdr_img);
    }

    hdr_frame.w = hdr_img.cols;
    hdr_frame.h = hdr_img.rows;
    hdr_frame.buff = std::make_shared<insbuff>(hdr_img.cols*hdr_img.rows*3);
    memcpy(hdr_frame.buff->data(), hdr_img.data, hdr_frame.buff->size());

    return hdr_frame;
}