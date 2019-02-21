
#include <stdio.h>
#include <sys/types.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include "common.h"
#include "ins_i2c.h"
#include "inslog.h"

ins_i2c::~ins_i2c()
{
    if (fd_ != -1)
    {
        close(fd_);   
        fd_ = -1;
    }
}

int32_t ins_i2c::open(uint32_t adapter, uint32_t addr, bool force)
{
    std::lock_guard<std::mutex> lock(mtx_);

    int32_t type = force?I2C_SLAVE_FORCE:I2C_SLAVE;

    std::stringstream ss;
    ss << "/dev/i2c-" << adapter;
    fd_ = ::open(ss.str().c_str(), O_RDWR);
    if (fd_ < 0)
    {
        LOGERR("i2c:%s open fail", ss.str().c_str());
        return INS_ERR;
    }

    if (ioctl(fd_, type, addr) < 0) 
    {
        LOGERR("i2c:%s ioctrl fail", ss.str().c_str());
        return INS_ERR;
    }

    //LOGINFO("i2c:%s addr:0x%x force:%d open success", ss.str().c_str(), addr, force);
}

int32_t ins_i2c::write(uint8_t reg, uint8_t value)
{
    std::lock_guard<std::mutex> lock(mtx_);

    uint8_t buff[2];
    buff[0] = reg;
    buff[1] = value;
    auto ret = ::write(fd_, buff, 2);
    if (ret != 2)
    {
        //LOGERR("i2c reg:0x%x write fail:%d %d", reg, ret, errno);
        return INS_ERR;
    }
    else
    {
        return INS_OK;
    }
}


int32_t ins_i2c::read(uint8_t reg, uint8_t& value)
{
    auto ret = ::write(fd_, &reg, sizeof(reg));
    if (ret != sizeof(reg))
    {
        //LOGERR("i2c read reg:0x%x write fail:%d %d", reg, ret, errno);
        return INS_ERR;
    }

    ret = ::read(fd_, &value, sizeof(value));
    if (ret != sizeof(value))
    {
        //LOGERR("i2c read fail:%d %d", ret, errno);
        return INS_ERR;
    }
    else
    {
        return INS_OK;
    }
}
