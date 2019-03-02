
#include "jpeg_file_dec.h"
#include "insbuff.h"
#include "common.h"
#include "inslog.h"
#include <fstream>

ins_img_frame jpeg_file_dec::decode(std::string file_name)
{
    tjpeg_dec dec;
	if (INS_OK != dec.open()) return nullptr;

    std::fstream s(file_name, s.binary | s.in);
    if (!s.is_open()) {
        LOGERR("file:%s open fail", file_name.c_str());
        return nullptr;
    }

    s.seekg(0, std::ios::end);
    int size = (int)s.tellg();
    LOGINFO("----file size:%d", size);
    s.seekg(0, std::ios::beg);

    auto buff = std::make_shared<insbuff>(size);
    if (!s.read((char*)buff->data(), buff->size())) {
        LOGERR("file:%s read fail", file_name.c_str());
        return nullptr;
    }
    
    auto frame = dec.decode(buff->data(), buff->size());

    return frame;
}