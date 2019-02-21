#include "gyro_data_parser.h"
#include "inslog.h"
#include "common.h" 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

gyro_data_parser::~gyro_data_parser()
{
    close();
}

void gyro_data_parser::close()
{
    std::lock_guard<std::mutex> lock(mtx_);

    if (fd_ != -1)
    {
        ::close(fd_);
        fd_ = -1;
    }
    eof_ = true;

    LOGINFO("gyro data file:%s close", filename_.c_str());
}

int gyro_data_parser::open(const std::string& filename)
{
    filename_ = filename;
    fd_ = ::open(filename_.c_str(), O_RDONLY, 0666);
    if (fd_ < 0)
    {
        LOGERR("gyro data file:%s open fial:%d", filename_.c_str(), errno);
        return INS_ERR_GYRO_DATA_DAMAGE;
    }
    else
    {
        LOGINFO("gyro data file:%s open success", filename_.c_str());
        return INS_OK;
    }
}

int gyro_data_parser::reopen()
{
    std::lock_guard<std::mutex> lock(mtx_);

    fd_ = ::open(filename_.c_str(), O_RDONLY, 0666);
    if (fd_ < 0)
    {
        LOGERR("gyro data file:%s open fial:%d", filename_.c_str(), errno);
        return INS_ERR_GYRO_DATA_DAMAGE;
    }
    else
    {
        LOGINFO("gyro data file:%s reopen success", filename_.c_str());
    }

    int ret = lseek(fd_, pos_, SEEK_SET);
    if (ret < 0)
    {
        LOGERR("file:%s lseek ret:%d", filename_.c_str(), ret);
        ::close(fd_);
        fd_ = -1;
        return INS_ERR_GYRO_DATA_DAMAGE;
    }

    eof_ = false;
    return INS_OK;
}

int gyro_data_parser::get_next(ins_g_a_data& data)
{
    if (eof_) return INS_ERR_OVER;
    
    int ret = seek_to_frame_start();
    RETURN_IF_NOT_OK(ret);

    double buff[7];
    ret = read_data((char*)buff, sizeof(buff));
    RETURN_IF_NOT_OK(ret);

    data.pts = buff[0]*1000000.0;
    data.gyro_x = buff[1];
    data.gyro_y = buff[2];
    data.gyro_z = buff[3];
    data.accel_x = buff[4];
    data.accel_y = buff[5];
    data.accel_z = buff[6];

    // printf("pts:%lf gyro:%lf %lf %lf accel:%lf %lf %lf\n", 
    //     buff[0], 
    //     buff[1], 
    //     buff[2],
    //     buff[3], 
    //     buff[4], 
    //     buff[5],
    //     buff[6]);
        
    return INS_OK;
}

int gyro_data_parser::seek_to_frame_start()
{
    char buff[8];
    int ret = read_data(buff, sizeof(buff));
    RETURN_IF_NOT_OK(ret);

    while (1)
    {
        if (is_start_flag(buff, 8)) return INS_OK;
        LOGERR("gyro data not start with ff(8), find next");
        memmove(buff, buff+1, 7);
        ret = read_data(&buff[7], sizeof(char));
        RETURN_IF_NOT_OK(ret);
    }
}

int gyro_data_parser::read_data(char* buff, unsigned size)
{
    std::lock_guard<std::mutex> lock(mtx_);

    auto ret = ::read(fd_, buff, size);
    if (ret < size)
    {
        eof_ = true;
        ::close(fd_);
        fd_ = -1;
        LOGERR("read err:%d", errno);
        return INS_ERR_OVER;
    }
    pos_ += ret;
    return INS_OK;
}

bool gyro_data_parser::is_start_flag(char* data, unsigned size)
{
    for (unsigned i = 0; i < size; i++)
    {
        if (data[i] != 0xff) return false;
    }

    return true;
}