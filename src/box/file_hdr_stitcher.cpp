
#include "file_hdr_stitcher.h"
#include "common.h"
#include "inslog.h"
#include "ins_blender.h"
#include "img_enc_wrap.h"
#include "offset_wrap.h"
#include "exif_parser.h"
#include "hdr_wrap.h"
#include "metadata.h"

file_hdr_stitcher::~file_hdr_stitcher()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int file_hdr_stitcher::open(const task_option& option)
{
    running_th_cnt_ = 1;
    th_ = std::thread(&file_hdr_stitcher::task, this, option);
    return INS_OK;
}

void file_hdr_stitcher::task(task_option option)
{
    do 
    {
        std::string offset;
        if (option.blend.offset != "") 
        {
            offset = option.blend.offset;
        }
        else
        {
            // offset_wrap ow;
            // result_ = ow.calc(option.input.file[0], option.blend.len_ver);
            // BREAK_IF_NOT_OK(result_);
            // offset = ow.get_4_3_offset();
        }

        std::function<void(double)> func = [this](double progress)
        {
            progress_ += progress*75.0; //hdr占75%的时间
        };

        hdr_wrap hdr;
        hdr.set_progress_cb(func);
        std::vector<ins_img_frame> hdr_img;
        result_ = hdr.process(option.input.file, hdr_img);
        BREAK_IF_NOT_OK(result_);

        //中间一组的第0个取exif信息
        jpeg_metadata metadata;
        exif_parser parser;
        parser.parse(option.input.file[0][0], metadata);

        ins_blender blender(INS_BLEND_TYPE_OPTFLOW, option.blend.mode, ins::BLENDERTYPE::BGR_CUDA);
        blender.set_map_type(INS_MAP_FLAT);
        blender.set_speed(ins::SPEED::SLOW);
        blender.setup(offset);

        ins_img_frame out_img;
        out_img.w = option.output.width;
        out_img.h = option.output.height;
        result_ = blender.blend(hdr_img, out_img);

        img_enc_wrap enc(option.blend.mode, INS_MAP_FLAT, &metadata);
        std::vector<std::string> url;
        std::string out_file = option.output.file + "/merge.jpg";
        url.push_back(out_file);
        result_ = enc.encode(out_img, url);
    } 
    while(0);

    progress_ = 100;
    running_th_cnt_--;
}