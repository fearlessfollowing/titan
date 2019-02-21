
#include "ins_len_offset.h"
#include "common.h"
#include "inslog.h"
#include "Lens/Lens.h"
#include "Stitcher/StitchManager.h"
#include "Common/raw/colorspace_raw.hpp"
#include <opencv2/opencv.hpp>
#include "ins_clock.h"

using namespace ins;

int32_t ins_len_offset::calc(const std::vector<ins_img_frame>& v_data, uint32_t version, bool b_raw)
{
    try
    {
        if (v_data[0].w != 5280 || v_data[0].h != 3956)
        {
            LOGERR("invalid w:%d h:%d", v_data[0].w, v_data[0].h);
            return INS_ERR;
        }

        Lens::LENSTYPE len_type = Lens::HJ5117A_TITAN; 
        Lens::LENSVER len_ver = Lens::TitanVer; 

        std::vector<cv::Mat> v_img;
        std::vector<float> v_cx, v_cy, v_radius;
        std::vector<int> v_width, v_height, v_len_type;

        ins_clock clock;

        for (uint32_t i = 0; i < v_data.size(); i++)
        {
            if (b_raw)
            {
                cv::Mat raw_img(v_data[i].h, v_data[i].w, CV_16UC1, v_data[i].buff->data());
                cv::Mat rgb_img;
                ins::raw::raw2rgb(raw_img, rgb_img);
                v_img.push_back(rgb_img);
            }
            else
            {
                cv::Mat img(v_data[i].h, v_data[i].w, CV_8UC3, v_data[i].buff->data());
                v_img.push_back(img);
            }
            
            v_cx.push_back(v_data[0].w/2);
            v_cy.push_back(v_data[0].h/2);
            v_radius.push_back(v_data[0].w/2);
            v_width.push_back(v_data[0].w);
            v_height.push_back(v_data[0].h);
            v_len_type.push_back(len_type);
        }

        //pano offset
        auto manager = std::make_shared<StitchManager>();
        manager->initParams(v_cx, v_cy, v_radius, v_width, v_height, v_len_type, len_ver, PANOTYPE::PANO);
        manager->addImage(v_img);

        std::string offset;
        manager->getOffset(offset);

        // if (v_data[0].w*3 == v_data[0].h*4) //4:3
        // {
            offset_pic_ = offset;
            // StitchManager::transformOffset(offset, offset_pano_16_9_);
            // StitchManager::transformOffset(offset, offset_pano_2_1_, 2);
        // }
        // else
        // {
        //     LOGERR("------------PIC NOT 4:3");
        // }
        // else if (v_data[0].w*9 == v_data[0].h*16) //16:9
        // {
        //     offset_pano_16_9_ = offset;
        //     StitchManager::transformOffset(offset, offset_pano_4_3_, 1);
        //     StitchManager::transformOffset(offset_pano_4_3_, offset_pano_2_1_, 2);
        // }
        // else //2:1
        // {
        //     StitchManager::transformOffset(offset, offset_pano_4_3_);
        //     StitchManager::transformOffset(offset_pano_4_3_, offset_pano_16_9_);
        // }

        LOGINFO("offset calculate time:%lf", clock.elapse());
    }
    catch (...)
    {
        LOGERR("catach exception");
        return INS_CATCH_EXCEPTION;
    }

    return INS_OK;
}