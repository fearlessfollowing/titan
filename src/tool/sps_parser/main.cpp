
#include <iostream>
#include <stdio.h>
#include "sps_parser.h"

int32_t main(int32_t argc, char* argv[])
{
    FILE* fp = fopen(argv[1], "rb+");
    if (fp == nullptr)
    {
        std::cout << "file open fail" << std::endl;
        return -1;
    }

    uint8_t buff[128];
    fread(buff, 1, sizeof(buff), fp);
    //fclose(fp);

    sps_parser sps;
    auto pos = sps.parse(buff+4, sizeof(buff)-4);
    if (pos != -1)
    {
        printf("pos:%d value:0x%x %d\n", pos, buff[4+pos/8], buff[4+pos/8]);
        buff[4+pos/8] &= ~(0x80 >> (pos % 8));
        printf("after value:0x%x %d\n", buff[4+pos/8], buff[4+pos/8]);
        fseek(fp, 0, SEEK_SET);
        fwrite(buff, 1, 128, fp);
    }

    fclose(fp);


    return 0;
}