#include "prj_file_parser.h"
#include "tinyxml2.h"
#include "inslog.h"
#include "common.h"
#include "ins_util.h"

using namespace tinyxml2;

std::string prj_file_parser::get_offset(std::string path, int width, int height)
{
    std::string url = path + "/" + INS_PROJECT_FILE;
    
    XMLDocument xml_doc;
    if (XML_NO_ERROR != xml_doc.LoadFile(url.c_str()))
    {
        LOGERR("xml file:%s load fail", url.c_str());
        return ""; 
    }

    XMLElement* element = xml_doc.RootElement();
    if (!element)
    {
        LOGERR("xml: cann't find root element");
        return "";
    }
    
    element = element->FirstChildElement("offset");  
    if (!element)
    {
        LOGERR("xml: cann't find element:offset");
        return "";
    }

    std::string type = "pano_4_3";
    if (width*9 == height*16) type = "pano_16_9";

    element = element->FirstChildElement(type.c_str());  
    if (!element)
    {
        LOGERR("xml: cann't find element:%s", type.c_str());
        return "";
    }

    return element->GetText();
}

static int parse_video_input(const std::string& path, const XMLElement* origin_e, task_input_option& opt)
{
    auto e = origin_e->FirstChildElement("filegroup");
    while (e)
    {
        auto file_e = e->FirstChildElement("file");
        std::vector<std::string> group_file;
        while (file_e)
        {
            std::string file_name = path + "/" + file_e->GetText();
            group_file.push_back(file_name);
            file_e = file_e->NextSiblingElement();
        }
        if (group_file.size() != INS_CAM_NUM)
        {
            LOGERR("origin file cnt:%d != %d", group_file.size(), INS_CAM_NUM);
            return INS_ERR_PRJ_FILE_DAMAGE;
        }
        opt.file.push_back(group_file);
        e = e->NextSiblingElement();
    }

    if (opt.file.empty())
    {
        LOGERR("no input file");
        return INS_ERR_PRJ_FILE_DAMAGE;
    }
    else
    {
        return INS_OK;
    }
}

static int parse_photo_input(const std::string& path, const XMLElement* origin_e, task_input_option& opt)
{
    auto file_e = origin_e->FirstChildElement("file");
    std::vector<std::string> group_file;
    while (file_e)
    {
        std::string file_name = path + "/" + file_e->GetText();
        group_file.push_back(file_name);
        file_e = file_e->NextSiblingElement();
    }
    if (group_file.size() != INS_CAM_NUM)
    {
        LOGERR("origin file cnt:%d != %d", group_file.size(), INS_CAM_NUM);
        return INS_ERR_PRJ_FILE_DAMAGE;
    }
    opt.file.push_back(group_file);

    return INS_OK;
}

int prj_file_parser::get_input_param(const std::string& path, task_input_option& opt)
{
    int ret = INS_OK;
    std::string url = path + "/" + INS_PROJECT_FILE;

    XMLDocument xml_doc;
    ret = xml_doc.LoadFile(url.c_str());
    if (XML_ERROR_FILE_NOT_FOUND == ret)
    {
        LOGERR("prj file:%s not fond", url.c_str());
        return INS_ERR_PRJ_FILE_NOT_FOND;
    }
    else if (XML_NO_ERROR != ret)
    {
        LOGERR("prj file:%s load fail:%d", url.c_str(), ret);
        return INS_ERR_PRJ_FILE_DAMAGE;
    }

    XMLElement* e = xml_doc.RootElement();
    if (!e)
    {
        LOGERR("xml: cann't find root element");
        return INS_ERR_PRJ_FILE_DAMAGE;
    }

    XMLElement* origin_e = e->FirstChildElement("origin");  
    if (!origin_e)
    {
        LOGERR("xml: cann't find element:origin");
        return INS_ERR_PRJ_FILE_DAMAGE;
    }

    e = origin_e->FirstChildElement("metadata");  
    if (!e)
    {
        LOGERR("xml: cann't find element:metadata");
        return INS_ERR_PRJ_FILE_DAMAGE;
    }

    opt.width =  e->IntAttribute("width");
    opt.height =  e->IntAttribute("height");
    opt.type = e->Attribute("type");
    opt.count = e->IntAttribute("count");
    opt.b_spatial_audio = e->BoolAttribute("spatial_audio");
    auto audio_dev = e->Attribute("audio_device");
    if (audio_dev != nullptr) opt.audio_dev = audio_dev;

    if (opt.type == INS_VIDEO_TYPE)
    {
        opt.framerate =  e->IntAttribute("framerate");
        ret = parse_video_input(path, origin_e, opt);
    }
    else if (opt.type == INS_PIC_TYPE_PHOTO ||
            opt.type == INS_PIC_TYPE_TIMELAPSE || 
            opt.type == INS_PIC_TYPE_BURST ||
            opt.type == INS_PIC_TYPE_HDR)
    {
        if (opt.count < 0)
        {
            LOGERR("invalid img count:%d", opt.count);
            return INS_ERR_PRJ_FILE_DAMAGE;
        }
        if (opt.type == INS_PIC_TYPE_TIMELAPSE)
        {
            opt.count = ins_util::file_cnt_under_dir(path)/INS_CAM_NUM; //因为timelapse的count,在工程文件里没有写
        }
        ret = parse_photo_input(path, origin_e, opt);
    }
    else
    {
        LOGERR("invalid type:%s", opt.type.c_str());
        return INS_ERR_PRJ_FILE_DAMAGE;
    }

    return ret;
}

int prj_file_parser::get_gyro_param(const std::string& path, task_gyro_option& opt)
{
    int ret = INS_OK;
    std::string url = path + "/" + INS_PROJECT_FILE;

    XMLDocument xml_doc;
    ret = xml_doc.LoadFile(url.c_str());
    if (XML_ERROR_FILE_NOT_FOUND == ret)
    {
        LOGERR("prj file:%s not fond", url.c_str());
        return INS_ERR_PRJ_FILE_NOT_FOND;
    }
    else if (XML_NO_ERROR != ret)
    {
        LOGERR("prj file:%s load fail:%d", url.c_str(), ret);
        return INS_ERR_PRJ_FILE_DAMAGE;
    }

    XMLElement* e = xml_doc.RootElement();
    if (!e)
    {
        LOGERR("xml: cann't find root element");
        return INS_ERR_PRJ_FILE_DAMAGE;
    }

    XMLElement* gyro_e = e->FirstChildElement("gyro");  
    if (!gyro_e)
    {
        opt.version = 2; //old version gyro
        LOGERR("xml: cann't find element:gyro");
        return INS_OK;
    }

    opt.version = gyro_e->IntAttribute("version");
    e = gyro_e->FirstChildElement("file");
    if (e) opt.file = path + "/" + e->GetText();

    e = gyro_e->FirstChildElement("start_ts"); //video
    if (e) e->QueryDoubleText(&opt.time_offset);

    e = gyro_e->FirstChildElement("timestamp"); //raw 
    if (e) e->QueryDoubleText(&opt.time_offset);

    auto cal_e = gyro_e->FirstChildElement("calibration");
    if (cal_e)
    {
        opt.b_gravity_offset = true;
        e = cal_e->FirstChildElement("gravity_x");
        if (e) e->QueryDoubleText(&opt.gravity_x_offset);
        e = cal_e->FirstChildElement("gravity_y");
        if (e) e->QueryDoubleText(&opt.gravity_y_offset);
        e = cal_e->FirstChildElement("gravity_z");
        if (e) e->QueryDoubleText(&opt.gravity_z_offset);
    }

    return INS_OK;
}