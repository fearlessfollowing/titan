#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#pragma pack(1) //自定义1字节对齐
struct gyro_packet
{
    int64_t timestamp = 0;
    double accel[3];
    double gyro[3];
};

struct exposure_packet
{
    int64_t timestamp = 0;
    double exposure = 0;
};
#pragma pack() //自定义1字节对齐


void parse_gyro(FILE* in_fp, FILE* out_fp)
{
    uint32_t packet_size = sizeof(gyro_packet);
    gyro_packet pkt;
    while (1)
    {
        auto ret = fread(&pkt, 1, packet_size, in_fp);
        if (ret < packet_size)
        {
            std::cout << "file read over" << std::endl;
            break;
        }

        fprintf(out_fp, "ts:%ld gyro:%lf %lf %lf accel:%lf %lf %lf\n", 
            pkt.timestamp, 
            pkt.gyro[0], 
            pkt.gyro[1], 
            pkt.gyro[2],
            pkt.accel[0], 
            pkt.accel[1], 
            pkt.accel[2]);
    }
}

void parse_exposure(FILE* in_fp, FILE* out_fp)
{
    uint32_t packet_size = sizeof(exposure_packet);
    exposure_packet pkt;
    while (1)
    {
        auto ret = fread(&pkt, 1, packet_size, in_fp);
        if (ret < packet_size)
        {
            std::cout << "file read over" << std::endl;
            break;
        }

        fprintf(out_fp, "ts:%ld exposure:%f\n", 
            pkt.timestamp, 
            pkt.exposure);
    }
}

int32_t main(int32_t argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "please input type(gyro/exp) && file" << std::endl;
        return -1;
    }

    std::cout << "gyro:" << sizeof(gyro_packet) << " exposure:" << sizeof(exposure_packet) << std::endl;

    std::string in_filename = argv[2];

    FILE* in_fp = fopen(in_filename.c_str(), "rb");
    if (!in_fp) 
    {
        std::cout << "file:" << in_filename << " open fail" << std::endl;
        return -1;
    }

    std::string out_filename = in_filename + ".txt";

    FILE* out_fp = fopen(out_filename.c_str(), "w");
    if (!out_fp) 
    {
        fclose(in_fp);
        std::cout << "file:" << out_filename << " open fail" << std::endl;
        return -1;
    }

    if (std::string(argv[1]) == "gyro")
    {
        parse_gyro(in_fp, out_fp);
    }
    else
    {
        parse_exposure(in_fp, out_fp);
    }

    fclose(in_fp);
    fclose(out_fp);

    std::cout << "paser over" << std::endl;

    return 0;
}
