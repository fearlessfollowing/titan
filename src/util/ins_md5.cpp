
// #include <string>
// #include "ins_md5.h"
// #include "inslog.h"
// #include "inserr.h"

// extern "C"
// {
// #include "libavutil/md5.h"
// #include "libavutil/avutil.h"
// }

// int md5_file(std::string file_name, std::string& md5_value)
// {
//     struct AVMD5* ctx = av_md5_alloc();
//     if (ctx == nullptr)
//     {
//         LOGERR("alloc md5 context fail");
//         return INS_ERR;
//     }

//     av_md5_init(ctx);

//     FILE* fp = fopen(file_name.c_str(), "rb");
//     if (fp == nullptr)
//     {
//         LOGERR("file:%s open fail", file_name.c_str());
//         return INS_ERR;
//     }

//     unsigned int buff_len = 16*1024;
//     unsigned char* buff = new unsigned char[buff_len]();
//     while (1)
//     {
//         int len = fread(buff, 1, buff_len, fp);
//         if (len <= 0) break;
//         av_md5_update(ctx, buff, len);
//     }

//     unsigned char* result = new unsigned char[16](); 
//     av_md5_final(ctx, result);

//     char* result_string = new char[16*2+1](); 
//     unsigned int offset = 0;
//     for (int i = 0; i < 16; i++)
//     {
//         sprintf(result_string+offset, "%02x", result[i]);
//         offset += 2;
//     }

//     LOGINFO("file:%s md5 value:%s", file_name.c_str(), result_string);

//     md5_value = std::string(result_string);

//     delete[] buff;
//     delete[] result;
//     delete[] result_string;
//     fclose(fp);
//     av_free(ctx);

//     return INS_OK;
// }