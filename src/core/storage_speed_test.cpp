
#include "storage_speed_test.h"
#include "common.h"
#include "inslog.h"
#include <fcntl.h>
#include <sstream>
#include <sys/time.h>
#include "ins_util.h"

#define _MAX_BUFF_SIZE_ 100

int storage_speed_test::test(const std::string& path)
{
    LOGINFO("speed test path:%s", path.c_str());

	/* 1.检查存储空间,如果存储空间小于1GB,直接报错 */
    if (ins_util::disk_available_size(path.c_str()) < INS_MIN_SPACE_MB * 10) {
        LOGERR("storage space not enough for testing storage speed");
        return INS_ERR_NO_STORAGE_SPACE;
    }

    for (int i = 0; i < num_; i++) {
        std::stringstream ss;
        ss << path << "/" << i << ".dat";
        int fd = ::open(ss.str().c_str(), O_CREAT|O_WRONLY, 0666);
        if (fd < 0) {
            LOGERR("file:%s open fail:%d", ss.str().c_str(), fd);
        } else {
            LOGINFO("file:%s open success", ss.str().c_str());
        }
        
        fd_.push_back(fd);

        std::deque<std::shared_ptr<page_buffer>> q;
        queue_.push_back(q);
    }

    for (int i = 0; i < num_; i++) {	/* 创建num_个生产者线程和num_个消费者线程 */
        th_c_[i] = std::thread(&storage_speed_test::consume_task, this, i);
        th_p_[i] = std::thread(&storage_speed_test::produce_task, this, i);
    }

	/* 等待测速线程结束 */
    for (int i = 0; i < num_; i++) {
        INS_THREAD_JOIN(th_p_[i]);
        INS_THREAD_JOIN(th_c_[i]);
        ::close(fd_[i]);
    }

	/* 移除测速产生的临时文件 */
    for (int i = 0; i < num_; i++) {
        std::stringstream ss;
        ss << path << "/" << i << ".dat";
        remove(ss.str().c_str());
    }
    return result_;
}



void storage_speed_test::produce_task(int index)
{
    int loop_cnt = 0;
    int total_cnt;
    int size;
    struct timeval tv;
    tv.tv_sec = 0;

    if (index < 7) {   
        //7Mbps@30fps
        size = 7 * 1024 * 1024 / 8 / 30; 	/* 一帧数据量 */
        tv.tv_usec = 1000 * 1000 / 30; 		/* 一帧时间 */
        total_cnt = 120*30; 				/* 帧数 */
    } else {
        //10Mbps@60
        size = 10 * 1024 * 1024 / 8 / 60; 	/* 一帧数据量  */
        tv.tv_usec = 1000 * 1000 / 60;		/* 一帧时间 */
        total_cnt = 120 * 60; 				/* 帧数  */
    }

    LOGINFO("index:%d frame size:%d interval:%d ms", index, size, tv.tv_usec/1000);

    struct timeval start_time;
    gettimeofday(&start_time, nullptr);

    while (!quit_ && loop_cnt++ < total_cnt) {
        auto buff = std::make_shared<page_buffer>(size);
        queue_buff(index, buff);
        ins_util::sleep_s((long long)tv.tv_sec * 1000000 + tv.tv_usec);
    }

    double total_size = ((double)size * (loop_cnt - queue_[index].size())) / 1024.0 / 1024.0;

    struct timeval end_time;
    gettimeofday(&end_time, nullptr);
	
    long total_time = (end_time.tv_sec*1000*1000 + end_time.tv_usec - start_time.tv_sec*1000*1000 - start_time.tv_usec) / 1000*1000;

    double speed = total_size / total_time;

    LOGINFO("index: %d total size:%lf MB speed:%lf MB/s", index, total_size, speed);
    quit_ = true;
}



void storage_speed_test::consume_task(int index)
{
    while (!quit_) {
        auto buff = dequeue_buff(index);
        if (buff == nullptr) {
            usleep(20*1000);
            continue;
        }

        int ret = write_file(index, buff);
        if (ret != INS_OK) {
            quit_ = true;
            result_ = ret;
        }
    }
}



int storage_speed_test::write_file(int index, const std::shared_ptr<page_buffer>& buff)
{
    unsigned int block_size = 32*1024;
    unsigned int offset = 0;

    while (offset < buff->size()) {
        unsigned int size = std::min(block_size, buff->size() - offset);
        int ret = ::write(fd_[index], buff->data() + offset, size);
        if (ret < 0) {
            LOGERR("index:%d write fail", index);
            return INS_ERR_FILE_IO;
        }
        offset += size;
    } 

    return INS_OK;
}


void storage_speed_test::queue_buff(int index, const std::shared_ptr<page_buffer>& buff)
{
    std::lock_guard<std::mutex> lock(mtx_[index]);    
    if (queue_[index].size() > _MAX_BUFF_SIZE_) {
        LOGERR("index:%d queue size:%d too big", index, queue_[index].size());
        quit_ = true;
        result_ = INS_ERR_UNSPEED_STORAGE;
        return;
    }
    queue_[index].push_back(buff);
}

std::shared_ptr<page_buffer> storage_speed_test::dequeue_buff(int index)
{
    std::lock_guard<std::mutex> lock(mtx_[index]);
    if (queue_[index].empty()) 
        return nullptr;

    auto buff = queue_[index].front();
    queue_[index].pop_front();
	
    return buff;
}


