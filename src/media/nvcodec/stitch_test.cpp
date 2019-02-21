
#include "video_stitcher.h"
#include "inslog.h"
#include "common.h"
#include "ffutil.h"
#include <unistd.h>

int32_t main(int32_t argc, char* argv[])
{
    std::string i_file_name;
    std::string o_file_name;
    int width, height, bitrate;

    int ch, opt_cnt = 0;
    while ((ch = getopt(argc, argv, "w:h:i:o:b:")) != -1)
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

    if (opt_cnt < 5)
    {
        printf("option: w:h:i:o:b:\n");
        return -1;
    }

    ins_log::init("./", "log");
    ffutil::init();

    std::vector<std::string> v_in_file;
    for (int32_t i = 0; i < 6; i++)
    {
        std::stringstream ss;
        ss << i_file_name << "/origin_" << i << ".mp4";
        v_in_file.push_back(ss.str());
    }

    auto st = std::make_shared<video_stitcher>();
    auto ret = st->open(v_in_file, o_file_name, width, height, bitrate);
    RETURN_IF_TRUE(ret != INS_OK, "video stitcher open fail", ret);

    st = nullptr;

    LOGINFO("main exit");

    return 0;
}