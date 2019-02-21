#include <nv_video_enc.h>
#include "inslog.h"
#include "common.h"
#include "ffutil.h"
#include <unistd.h>

int INS_CAM_NUM = 6;

FILE* fp_i_ = nullptr;
int frames_ = 0;

int read_yuv_frame(NvBuffer* buffer)
{
    if (!fp_i_) return INS_ERR;

    for (int32_t i = 0; i < buffer->n_planes; i++)
    {
        auto &plane = buffer->planes[i];

        auto p = (char*)plane.data;
        auto line_size = plane.fmt.bytesperpixel*plane.fmt.width;

        for (int32_t j = 0; j < plane.fmt.height; j++)
        {
            if (line_size != fread(p, 1, line_size, fp_i_))
            {
                LOGINFO("yuv file read over");
                return INS_ERR;
            }
            p += plane.fmt.stride;
        }
        plane.bytesused = plane.fmt.stride * plane.fmt.height;
    }

    frames_++;

    return INS_OK;
}

int32_t main(int32_t argc, char* argv[])
{
    std::string i_file_name;
    std::string o_file_name;
    std::string fmt;
    int width, height, bitrate;

    int ch, opt_cnt = 0;
    while ((ch = getopt(argc, argv, "w:h:i:o:f:b:")) != -1)
    {
        switch (ch)
        {
            case 'w':
                width = atoi(optarg);
                opt_cnt++;
                break;
            case 'h':
                height = atoi(optarg);
                opt_cnt++;
                break;
            case 'f':
                printf("fmt opt:%s\n", optarg);
                fmt = optarg;
                opt_cnt++;
                break;
            case 'i':
                i_file_name = optarg;
                opt_cnt++;
                break;
            case 'o':
                o_file_name = optarg;
                opt_cnt++;
                break;
            case 'b':
                bitrate = atoi(optarg); //Mbit
                opt_cnt++;
                break;
            default:
                break;
        }

    }

    if (opt_cnt < 6)
    {
        printf("option: w:h:i:o:f:b:\n");
        return -1;
    }

    ins_log::init("./", "log");

    fp_i_ = fopen(i_file_name.c_str(), "rb");
    RETURN_IF_TRUE(fp_i_ == nullptr, "yuv file open fail", INS_ERR);
    
    auto enc = std::make_shared<nv_video_enc>();
    enc->set_resolution(width, height);
    enc->set_bitrate(bitrate*1000*1000);
    Arational framerate(30, 1);
    enc->set_framerate(framerate);
    auto ret = enc->open("enc", "h264");
    RETURN_IF_TRUE(ret != INS_OK, "enc open fail", ret);

    NvBuffer* buff = nullptr;
    while (1)
    {
        ret = enc->dequeue_input_buff(buff, 0);
        if (ret != INS_OK) break;
        ret = read_yuv_frame(buff);
        if (ret != INS_OK) buff->planes[0].bytesused = 0;
        enc->queue_intput_buff(buff, 0);
        if (ret != INS_OK) break;
    }

    enc = nullptr;

    if (fp_i_) fclose(fp_i_);

    LOGINFO("main exit frames:%d", frames_);


    while (1);

    return 0;
}