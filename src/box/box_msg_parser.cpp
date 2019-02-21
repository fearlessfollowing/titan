#include "box_msg_parser.h"
#include "json_obj.h"
#include "common.h"
#include "inslog.h"
#include "prj_file_parser.h"
#include "insdemux.h"
#include <sstream>
#include <unistd.h>

int box_msg_parser::parse(const std::string& param, std::shared_ptr<task_entry>& entry)
{
    json_obj root(param.c_str());
    root.get_string("uuid", entry->id);
    if (entry->id == "")
    {
        LOGERR("no key uuid in task msg");
        return INS_ERR_INVALID_MSG_FMT;
    }

    //只有添加任務的時候纔有這個字段
    root.get_string("state", entry->state);
    if (entry->state == "") entry->state = "pending";
    if (entry->state != "pending" && entry->state != "idle")
    {
        LOGERR("invalid state:%s", entry->state.c_str());
        return INS_ERR_INVALID_MSG_PARAM;
    }

    auto input_obj = root.get_obj("input");
    if (input_obj == nullptr)
    {
        LOGERR("no obj input in task msg");
        return INS_ERR_INVALID_MSG_FMT;
    }
    int ret = parse_input_option(input_obj, entry->option.input);
    RETURN_IF_NOT_OK(ret);

    std::string path;
    input_obj->get_string("path", path);

    auto output_obj = root.get_obj("output");
    if (output_obj == nullptr)
    {
        LOGERR("no obj output in task msg");
        return INS_ERR_INVALID_MSG_FMT;
    }
    ret = parse_output_option(output_obj, entry->option.output, entry->option.input);
    RETURN_IF_NOT_OK(ret);

    auto blend_obj = root.get_obj("blend");
    if (blend_obj == nullptr)
    {
        LOGERR("no obj blend in task msg");
        return INS_ERR_INVALID_MSG_FMT;
    }
    ret = parse_blend_option(blend_obj, entry->option.blend, entry->option.input.type);
    RETURN_IF_NOT_OK(ret);

    if (entry->option.input.type == INS_VIDEO_TYPE)
    {
        auto& input = entry->option.input;
        auto& blend = entry->option.blend;
        for (unsigned i = 0; i < input.file.size(); i++)
        {
            ins_demux demux;
            if (demux.open(input.file[i][0].c_str())) return INS_ERR_ORIGIN_VIDEO_DAMAGE;
            ins_demux_param param;
            demux.get_param(param);
            input.total_frames += param.v_frames;
            input.duration += param.v_duration;
            if (i == 0) input.framerate = param.fps;
            if (i == 0) input.b_camm = param.has_camm;
            if (i == 0) input.b_audio = param.has_audio;

            if (blend.cap_index == -1 && input.duration >= blend.cap_time) 
            {
                blend.cap_time = blend.cap_time + param.v_duration - input.duration; 
                blend.cap_index = i;
            }
        }

        if (blend.cap_index == -1 && blend.offset == "")
        {
            LOGERR("no offset and capture time:%lf beyond duration:%lf", blend.cap_time, input.duration);
            return INS_ERR_INVALID_MSG_PARAM;
        }

        entry->option.output.file_size = entry->option.output.bitrate*input.duration/8;//KB
    }
    //photo hdr
    else if (entry->option.input.type == INS_PIC_TYPE_HDR || entry->option.input.type == INS_PIC_TYPE_PHOTO)
    {
        entry->option.output.file_size = entry->option.output.width*entry->option.output.height/3/1000; //kB
    }
    //timelapse
    else
    {
        entry->option.output.file_size = entry->option.input.count*entry->option.output.width*entry->option.output.height/3/1000*11/10; //kB 10%浮动
    }

    //3D 不防抖
    if (entry->option.blend.mode == INS_MODE_PANORAMA)
    {
        auto gyro_obj = root.get_obj("gyro");
        if (gyro_obj != nullptr)
        {
            ret = parse_gyro_option(path, gyro_obj, entry->option.gyro);
            RETURN_IF_NOT_OK(ret);
        }
    }

    //print_task_entry(entry);

    return INS_OK;
}

int box_msg_parser::parse_input_option(const std::shared_ptr<json_obj>& obj, task_input_option& opt)
{
    std::string path;
    obj->get_string("path", path);

    return prj_file_parser::get_input_param(path, opt);
}

int box_msg_parser::parse_output_option(const std::shared_ptr<json_obj>& obj, task_output_option& opt, const task_input_option& input)
{
    obj->get_string("file", opt.file);  

    //判断是否可以创建文件
    int ret = create_dir(opt.file.c_str());
    RETURN_IF_NOT_OK(ret);

    if (input.type == INS_VIDEO_TYPE)
    {
        auto video_obj = obj->get_obj("video");
        if (video_obj == nullptr)
        {
            LOGERR("no obj video in output");
            return INS_ERR_INVALID_MSG_FMT; 
        }

        video_obj->get_int("width", opt.width);
        video_obj->get_int("height", opt.height);
        video_obj->get_int("bitrate", opt.bitrate);

        // std::string speed;
        // video_obj->get_string("preset", speed);
        // if (speed == "slow")
        // {
        //     opt.speed = INS_SPEED_SLOW;
        // }
        // else if (speed == "medium")
        // {
        //     opt.speed = INS_SPEED_MEDIUM;
        // }
        // else
        {
            opt.speed = INS_SPEED_FAST;
        }
        //video_obj->get_double("fps", opt.framerate); //帧率保存和原始流一致，不传帧率信息

        auto audio_obj = obj->get_obj("audio");
        if (audio_obj)
        {
            std::string type;
            audio_obj->get_string("type", type);
            if (type == "none")
            {
                opt.audio_type = INS_AUDIO_TYPE_NONE;
            }
            else if (type == "normal")
            {
                opt.audio_type = INS_AUDIO_TYPE_NORMAL;
            }
            else if (type == "pano")
            {
                if (input.b_spatial_audio)
                {
                    opt.audio_type = INS_AUDIO_TYPE_SPATIAL;
                }
                else
                {
                    LOGINFO("origin not sptial audio, change to normal");
                    opt.audio_type = INS_AUDIO_TYPE_NORMAL;
                }
            }
            else
            {
                LOGERR("unknow audio type:%s", type.c_str());
                return INS_ERR_INVALID_MSG_PARAM;
            }
        }
    }
    else
    {
        auto img_obj = obj->get_obj("image");
        if (img_obj == nullptr)
        {
            LOGERR("no obj image in output");
            return INS_ERR_INVALID_MSG_FMT; 
        }

        img_obj->get_int("width", opt.width);
        img_obj->get_int("height", opt.height);
    }

    return INS_OK;
}

int box_msg_parser::parse_blend_option(const std::shared_ptr<json_obj>& obj, task_blend_option& opt, std::string type)
{
    std::string mode;
    obj->get_string("mode", mode);
    if (mode == "pano")
    {
        opt.mode = INS_MODE_PANORAMA;
    }
    else if (mode == "3d" || mode == "3d_top_left")
    {
        opt.mode = INS_MODE_3D_TOP_LEFT;
    }
    else if (mode == "3d_top_right")
    {
        opt.mode = INS_MODE_3D_TOP_RIGHT;
    }
    else
    {
        return INS_ERR_INVALID_MSG_PARAM;
    }

    obj->get_string("offset", opt.offset);

    if (opt.offset == "" && type == INS_VIDEO_TYPE)
    {
        auto cal_obj = obj->get_obj("calibration");
        if (cal_obj == nullptr)
        {
            LOGERR("no offset nor calibration");
            return INS_ERR_INVALID_MSG_PARAM;
        }
        else
        {
            cal_obj->get_double("captureTime", opt.cap_time);
        }
    }

    return INS_OK;
}

int box_msg_parser::parse_gyro_option(const std::string& path, const std::shared_ptr<json_obj>& obj, task_gyro_option& opt)
{
    obj->get_boolean("enable", opt.enable);
    
    if (!opt.enable) 
    {
        return INS_OK;
    }
    else
    {
        return prj_file_parser::get_gyro_param(path, opt);
    }
}

int box_msg_parser::create_dir(std::string filename)
{
    auto pos = filename.rfind("/");
    if (pos == std::string::npos)
    {
        LOGERR("no char / in path:%s", filename.c_str());
        return INS_ERR_INVALID_MSG_PARAM;
    }

    filename.erase(pos);

    if (!access(filename.c_str(), 0)) //路径存在
    {
        if (!access(filename.c_str(), 6)) //且有读写权限
        {
            return INS_OK;
        }
        else //没有读写权限
        {
            LOGERR("path:%s no rw permission", filename.c_str());
            return INS_ERR_FILE_OPEN;
        }
    }

    if (filename.find("/mnt/media_rw/") == 0) //如果是外置存储，判断是否挂载
    {
        auto file_tmp = filename;
        pos = file_tmp.find("/", strlen("/mnt/media_rw/"));
        if (pos != std::string::npos) file_tmp.erase(pos);

        if (access(file_tmp.c_str(), 0))
        {
            LOGERR("storage device:%s not exist", file_tmp.c_str());
            return INS_ERR_INVALID_MSG_PARAM;
        }
    }

    //创建路径
    std::stringstream ss;
    ss << "mkdir -p " << filename;
    system(ss.str().c_str());

    if (access(filename.c_str(), 6)) //路径没有读写权限或者不存在
    {
        LOGERR("can't create dir:%s", filename.c_str());
        return INS_ERR_FILE_OPEN;
    }
    else
    {
        return INS_OK;
    }
}

void box_msg_parser::print_task_entry(const std::shared_ptr<task_entry>& entry)
{
    LOGINFO("task entry id:%s", entry->id.c_str());
    LOGINFO("input type:%s w:%d h:%d fps:%lf duration:%lf frames:%lld count:%d audio dev:%s spatial:%d", 
        entry->option.input.type.c_str(), 
        entry->option.input.width, 
        entry->option.input.height, 
        entry->option.input.framerate, 
        entry->option.input.duration,
        entry->option.input.total_frames,
        entry->option.input.count,
        entry->option.input.audio_dev.c_str(), 
        entry->option.input.b_spatial_audio);
    for (unsigned i = 0; i < entry->option.input.file.size(); i++)
    {
        for (unsigned j = 0; j < entry->option.input.file[i].size(); j++)
        {
            LOGINFO("origin file:%s", entry->option.input.file[i][j].c_str());
        }
    }
    LOGINFO("blend mode:%d offset:%s cap index:%d time:%lf", 
        entry->option.blend.mode, 
        entry->option.blend.offset.c_str(),
        entry->option.blend.cap_index,
        entry->option.blend.cap_time);
    LOGINFO("gyro enable:%d version:%d file:%s time_offset:%lf gravity offset:%d %lf %lf %lf", 
        entry->option.gyro.enable,
        entry->option.gyro.version,
        entry->option.gyro.file.c_str(),
        entry->option.gyro.time_offset, 
        entry->option.gyro.b_gravity_offset,
        entry->option.gyro.gravity_x_offset, 
        entry->option.gyro.gravity_y_offset, 
        entry->option.gyro.gravity_z_offset);
    LOGINFO("output file:%s w:%d h:%d bitrate:%d file size:%lldKB audio type:%d speed:%d", 
        entry->option.output.file.c_str(), 
        entry->option.output.width, 
        entry->option.output.height, 
        entry->option.output.bitrate, 
        entry->option.output.file_size,
        entry->option.output.audio_type, 
        entry->option.output.speed);
}
