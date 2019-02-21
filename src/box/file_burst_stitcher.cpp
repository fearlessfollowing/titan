
#include "file_burst_stitcher.h"
#include "common.h"
#include "inslog.h"
#include "ins_blender.h"
#include "offset_wrap.h"
#include "exif_parser.h"
#include "ins_util.h"
#include "img_enc_wrap.h"
#include "tjpegdec.h"

file_burst_stitcher::~file_burst_stitcher()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);
}

int file_burst_stitcher::open(const task_option& option)
{
    running_th_cnt_ = 1;
    th_ = std::thread(&file_burst_stitcher::task, this, option);
    return INS_OK;
}

void file_burst_stitcher::task(task_option option)
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
            std::vector<std::string> v_file;
            for (unsigned j = 0; j < option.input.file[0].size(); j++)
            {
                char filename[256];
                sprintf(filename, option.input.file[0][j].c_str(), 1);
                v_file.push_back(filename);
            }
            // offset_wrap ow;
            // result_ = ow.calc(v_file, option.blend.len_ver);
            // BREAK_IF_NOT_OK(result_);
            // offset = ow.get_4_3_offset();
        }

        for (int i = 0; i < option.input.count; i++)
        {
            if (quit_) break;

            auto pos = option.output.file.rfind("/");
            std::string path = option.output.file.substr(0, pos);
            if (ins_util::disk_available_size(path.c_str()) <= INS_MIN_SPACE_MB - 100) 
            {
                LOGERR("no disk space left");
                result_ = INS_ERR_NO_STORAGE_SPACE;
                break;
            }

            std::vector<std::string> v_file;
            for (unsigned j = 0; j < option.input.file[0].size(); j++)
            {
                char filename[256];
                sprintf(filename, option.input.file[0][j].c_str(), i+1);
                if (access(filename, 0)) break;
                v_file.push_back(filename);
            }

            if (v_file.size() < INS_CAM_NUM)
            {
                LOGERR("sequece:%d file not fond", i);
                continue;
            }

            jpeg_metadata metadata;
            exif_parser parser;
            parser.parse(v_file[0], metadata);

            std::vector<std::shared_ptr<page_buffer>> enc_img;
            result_ = ins_util::read_file(v_file, enc_img);
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
            char filename[256];
            sprintf(filename, option.output.file.c_str(), i+1);
            std::vector<std::string> url;
            url.push_back(filename);
            result_ = enc.encode(out_img, url);
            BREAK_IF_NOT_OK(result_);

            progress_ = (i+2)*100.0/(option.input.count+1);
        }
    } 
    while(0);

    progress_ = 100;
    running_th_cnt_--;
}
