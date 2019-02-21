#include "file_raw_stitcher.h"
#include "common.h"
#include "inslog.h"
#include "offset_wrap.h"
#include "tiff_mux.h"
#include "tiff_demux.h"
#include "ins_frame.h"
#include "ins_blender.h"

file_raw_stitcher::~file_raw_stitcher()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int file_raw_stitcher::open(const task_option& option)
{
    running_th_cnt_ = 1;
    th_ = std::thread(&file_raw_stitcher::task, this, option);
    return INS_OK;
}

void file_raw_stitcher::task(task_option option)
{
    do 
    {
        tiff_tags tags;
        std::vector<ins_img_frame> v_in_frame;
        for (int i = 0; i < INS_CAM_NUM; i++)
        {
            tiff_demux demux;
            ins_img_frame frame;
            tiff_tags t_tag;
            result_ = demux.parse(option.input.file[0][i], frame, t_tag);
            BREAK_IF_NOT_OK(result_);
            v_in_frame.push_back(frame);
            if (i == 0) tags = t_tag;
        }
        BREAK_IF_NOT_OK(result_);

        std::string offset;
        if (option.blend.offset != "") 
        {
            offset = option.blend.offset;
        }
        // else
        // {
        //     offset_wrap ow;
        //     result_ = ow.calc(v_in_frame, option.blend.len_ver, true);
        //     BREAK_IF_NOT_OK(result_);
        //     offset = ow.get_4_3_offset();
        // }

        ins_blender blender(INS_BLEND_TYPE_OPTFLOW, option.blend.mode, ins::BLENDERTYPE::RAW_CUDA);
        blender.set_map_type(INS_MAP_FLAT);
        blender.set_speed(ins::SPEED::SLOW);
        blender.setup(offset);

        ins_img_frame out_frame;
        out_frame.w = option.output.width;
        out_frame.h = option.output.height;
        result_ = blender.blend(v_in_frame, out_frame);
        BREAK_IF_NOT_OK(result_);

        tags.width = out_frame.w;
        tags.height = out_frame.h;
        tiff_mux mux;
        result_ = mux.mux(option.output.file, out_frame, tags);
        BREAK_IF_NOT_OK(result_);
    } 
    while(0);   

    progress_ = 100;
    running_th_cnt_--;
}