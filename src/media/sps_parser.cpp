#include "sps_parser.h"
#include <iostream>

int32_t sps_parser::parse(const uint8_t* sps, uint32_t size)
{
    uint32_t bitpos = 0;
    int32_t i;
    
    bitpos += 3;
    auto nal_type = U(5, sps, bitpos);
    if (nal_type != 7) return -1;
    std::cout << "naltype:" << nal_type << std::endl;
    
    auto profile_idc = U(8,sps,bitpos);
    auto constraint_set0_flag = U(1,sps,bitpos);
    auto constraint_set1_flag = U(1,sps,bitpos);
    auto constraint_set2_flag = U(1,sps,bitpos);
    auto constraint_set3_flag = U(1,sps,bitpos);
    auto reserved_zero_4bits = U(4,sps,bitpos);
    auto level_idc = U(8,sps,bitpos);
    
    auto seq_parameter_set_id = Ue(sps,size,bitpos);
    
    auto log2_max_frame_num_minus4 = Ue(sps,size,bitpos);
    //m_max_frame_num = pow(2, m_SPS_Param.log2_max_frame_num_minus4+4);
    
    auto pic_order_cnt_type = Ue(sps,size,bitpos);
    
    if(pic_order_cnt_type == 0)
    {
        auto log2_max_pic_order_cnt_lsb_minus4 = Ue(sps,size,bitpos);
        //m_max_pic_order_cnt_lsb = pow(2, m_SPS_Param.log2_max_pic_order_cnt_lsb_minus4+4);
    }
    else 
    {
        std::cout << "!!!!!pic_order_cnt_type:" << pic_order_cnt_type << std::endl;
    }
    
    auto num_ref_frames = Ue(sps,size,bitpos);
    auto gaps_in_frame_num_value_allowed_flag = U(1,sps,bitpos);
    auto pic_width_in_mbs_minus1 = Ue(sps,size,bitpos);
    auto pic_height_in_map_units_minus1 = Ue(sps,size,bitpos);
    auto frame_mbs_only_flag = U(1,sps,bitpos);
    
    if(!frame_mbs_only_flag)
    {
        auto mb_adaptive_frame_field_flag = U(1,sps,bitpos);
    }
    
    auto direct_8x8_inference_flag = U(1,sps,bitpos);
    auto frame_cropping_flag = U(1,sps,bitpos);
    
    if(frame_cropping_flag)
    {
        auto frame_crop_left_offset = Ue(sps,size,bitpos);
        auto frame_crop_right_offset = Ue(sps,size,bitpos);
        auto frame_crop_top_offset = Ue(sps,size,bitpos);
        auto frame_crop_bottom_offset = Ue(sps,size,bitpos);
    }

    printf("vui pos:%d value:0x%x\n", bitpos, sps[bitpos/8]);
    auto vui_parameters_present_flag = U(1,sps,bitpos);
    if (vui_parameters_present_flag)
    {
        auto aspect_ratio_info_present_flag = U(1,sps,bitpos);
        if (aspect_ratio_info_present_flag) 
        {
            auto aspect_ratio_idc = U(8,sps,bitpos);
            if (aspect_ratio_idc == 255) 
            {
                auto num = U(16,sps,bitpos);
                auto den = U(16,sps,bitpos);
            } 
        }

        auto overscan_info_present_flag = U(1,sps,bitpos); 
        if (overscan_info_present_flag)
        {
            auto overscan_appropriate_flag = U(1,sps,bitpos); 
        }

        auto video_signal_type_present_flag = U(1,sps,bitpos);
        if (video_signal_type_present_flag) 
        {
            auto video_format = U(3,sps,bitpos); 
            auto full_range = U(1,sps,bitpos); 
            //printf("fullrange:%d pos:%d value:0x%x\n", full_range, bitpos-1, sps[(bitpos-1)/8]);
            //return pos;
        }
        else
        {
            std::cout << "!!!!!no video_signal_type_present_flag" << std::endl;
        }
    }
    else
    {
        std::cout << "!!!!!no vui" << std::endl;
    }

    return -1;
}

int32_t sps_parser::U(uint32_t bits, const uint8_t *buff, uint32_t &bitpos)
{
    int32_t res = 0;
    for (uint32_t i = 0; i < bits; i++)
    {
        res <<= 1;
        
        if (buff[bitpos/8] & (0x80 >> (bitpos % 8)))
        {
            res += 1;
        }
        
        bitpos++;
    }
    
    return res;
}

int32_t sps_parser::Ue(const uint8_t* buff, uint32_t len, uint32_t &bitpos)
{
    uint32_t zeronum = 0;
    int32_t res = 0;
    
    while (bitpos < len * 8)
    {
        if (buff[bitpos/8] & (0x80 >> (bitpos%8)))
        {
            break;
        }
        
        zeronum++;
        
        bitpos++;
    }
    
    bitpos++;
    
    for (uint32_t  i = 0; i < zeronum; i++)
    {
        res <<= 1;
        
        if (buff[bitpos / 8] & (0x80 >> (bitpos % 8)))
        {
            res += 1;
        }
        
        bitpos++;
    }
    
    return (1 << zeronum) - 1 + res;
}