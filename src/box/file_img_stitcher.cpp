
#include "file_img_stitcher.h"
#include "common.h"
#include "inslog.h"
#include "ins_blender.h"
#include "offset_wrap.h"
#include "exif_parser.h"
#include "tjpegdec.h"
#include "ins_util.h"
#include "img_enc_wrap.h"

file_img_stitcher::~file_img_stitcher()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int file_img_stitcher::open(const task_option& option)
{
    running_th_cnt_ = 1;
    th_ = std::thread(&file_img_stitcher::task, this, option);
    return INS_OK;
}

void file_img_stitcher::task(task_option option)
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

        jpeg_metadata metadata;
        exif_parser parser;
        parser.parse(option.input.file[0][0], metadata);

        std::vector<std::shared_ptr<page_buffer>> enc_img;
        result_ = ins_util::read_file(option.input.file[0], enc_img);
        BREAK_IF_NOT_OK(result_);

        tjpeg_dec dec;
        result_ = dec.open(TJPF_BGR);
        BREAK_IF_NOT_OK(result_);

        std::vector<ins_img_frame> dec_img;
        for (unsigned i = 0; i < INS_CAM_NUM; i++)
        {
            ins_img_frame frame;
            result_ = dec.decode(enc_img[i]->data(), enc_img[i]->size(), frame);
            BREAK_IF_NOT_OK(result_);
            dec_img.push_back(frame);
        }

        ins_blender blender(INS_BLEND_TYPE_OPTFLOW, option.blend.mode, ins::BLENDERTYPE::BGR_CUDA);
        blender.set_map_type(INS_MAP_FLAT);
        blender.set_speed(ins::SPEED::SLOW);
        blender.setup(offset);

        ins_img_frame out_img;
        out_img.w = option.output.width;
        out_img.h = option.output.height;
        result_ = blender.blend(dec_img, out_img);
        BREAK_IF_NOT_OK(result_);

        img_enc_wrap enc(option.blend.mode, INS_MAP_FLAT, &metadata);
        std::vector<std::string> url;
        url.push_back(option.output.file);
        result_ = enc.encode(out_img, url);
    } 
    while(0);   

    progress_ = 100;
    running_th_cnt_--;
}
