
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include "insbuff.h"
#include <memory>
#include <sys/time.h>

int storage_test(std::string path, int file_cnt, int block_size, long long total_size)
{
    std::cout << "path:" << path << std::endl;
    std::cout << "file cnt:" << file_cnt << std::endl;
    std::cout << "block size:" << block_size << std::endl;
    std::cout << "total size:" << total_size << std::endl;

    std::vector<int> fd_;
    for (int i = 0; i < file_cnt; i++) {
        std::stringstream ss;
        ss << path << "/" << i << ".dat";
        int fd = ::open(ss.str().c_str(), O_CREAT|O_WRONLY, 0666);
        if (fd < 0) {
            std::cout << "file:" << ss.str() << " open fail" << std::endl;
            return -1;
        }

        fd_.push_back(fd);
    }

    struct timeval tm_start;
    gettimeofday(&tm_start, nullptr);

    int loop_cnt = total_size/block_size/file_cnt;
    int i = 0;
    while (++i <= loop_cnt) {
        for (int i = 0; i < file_cnt; i++) {
            auto buff = std::make_shared<page_buffer>(block_size);
            ::write(fd_[i], buff->data(), buff->size());
        }
    }

    for (int i = 0; i < file_cnt; i++) {
        ::close(fd_[i]);
    }

    struct timeval tm_end;
    gettimeofday(&tm_end, nullptr);

    double duration = tm_end.tv_sec - tm_start.tv_sec + (double)(tm_end.tv_usec - tm_start.tv_usec)/1000000.0;

    std::cout << "time:" << duration << "s" << std::endl;

    double bitrate = (double)loop_cnt*block_size*file_cnt/1024.0/duration;

    std::cout << "bitrate:" << bitrate << "KB" << std::endl;

    for (int i = 0; i < file_cnt; i++) {
        std::stringstream ss;
        ss << path << "/" << i << ".dat";
        remove(ss.str().c_str());
    }

    return 0;
}