#include "ins_battery.h"
//#include "inslog.h"
#include "common.h"
#include <unistd.h>

#define BATTERY_I2C_BUS 		2	
#define BATTERY_I2C_SLAVE_ADDR	0x55


enum 
{
    READ_CONTROL = 0,
    READ_ATRATE,
    READ_ATRATE_TOEMPTY,
    READ_TEMPERATURE,
    READ_VOLTAGE,
    READ_BATTERY_STATUS = 5,
    READ_CURRENT,
    READ_REMAIN_CAPACITY_mAh,
    READ_FULLCHARGE_CAPACITY_mAh,
    READ_AVERAGE_CURRENT,
    READ_AVERAGETIME_TOEMPTY = 10,
    READ_AVERAGETIMETOFULL,
    READ_STANDBY_CURRENT,
    READ_STANDBYTIME_TOEMPTY,
    READ_MAXLOADCURRENT,
    READ_MAXLOADTIME_TOEMPTY = 15,
    READ_AVERAGE_POWER,
    READ_INTERNAL_TEMPRATURE,
    READ_CYCLE_COUNT,
    READ_RELATIVE_STATE_OF_CHARGE,
    READ_STATE_OF_HEALTH = 20,
    READ_CHARGE_VOL,
    READ_CHARGE_CUR,
    READ_DESIGN_CAP,
    READ_BLOCK_DATA_CMD,
};

static const uint8_t _reg_arr[][2] = 
{
	{0x00,0x01},
	{0x02,0x03},
	{0x04,0x05},
	{0x06,0x07},
	{0x08,0x09},
	{0x0a,0x0b}, //5
	{0x0c,0x0d},
	{0x10,0x11},
	{0x12,0x13},
	{0x14,0x15},
	{0x16,0x17},//10
	{0x18,0x19},
	{0x1a,0x1b},
	{0x1c,0x1d},
	{0x1e,0x1f},
	{0x20,0x21},//15
	{0x24,0x25},
	{0x28,0x29},
	{0x2a,0x2b},
	{0x2c,0x2d},
	{0x2e,0x2f},//20
	{0x30,0x31},
	{0x32,0x33},
	{0x3c,0x3d},
	{0x3e,0x3f},
};

#define MIN_DEG 		(-40.00)
#define MAX_DEG 		(110.00)

// int32_t ins_battery::open()
// {
//     return i2c_.open(BATTERY_I2C_BUS, BATTERY_I2C_SLAVE_ADDR);
// }

int32_t ins_battery::read_temp(double& temp)
{
    int16_t val = 0;
    int32_t ret = 0;
    double inner_temp = MAX_DEG;
    double outer_temp = MAX_DEG;

    ins_i2c i2c;
    ret = i2c.open(BATTERY_I2C_BUS, BATTERY_I2C_SLAVE_ADDR);
    RETURN_IF_NOT_OK(ret);
	
    for (int32_t i = 0; i < 3; i++) {
        ret = read_value(i2c, READ_TEMPERATURE, val);
        RETURN_IF_NOT_OK(ret);
        inner_temp = convert_k_to_c(val);

        //LOGINFO("inner temp:%lf %d", inner_temp, val);
        if (inner_temp < MIN_DEG || inner_temp >= MAX_DEG) {
            usleep(10*1000);
            continue;
        } else {
            break;
        }
    }

    if (inner_temp < MIN_DEG || inner_temp >= MAX_DEG) return INS_ERR;

    for (int32_t i = 0; i < 3; i++) {
        ret = read_value(i2c, READ_INTERNAL_TEMPRATURE, val);
        RETURN_IF_NOT_OK(ret);
        outer_temp = convert_k_to_c(val);
		
        //LOGINFO("outter temp:%lf %d", outer_temp, val);
        if (outer_temp < MIN_DEG || outer_temp >= MAX_DEG) {
            usleep(10*1000);
            continue;
        }
        else
        {
            break;
        }
    }

    if (outer_temp < MIN_DEG || outer_temp >= MAX_DEG) return INS_ERR;

    temp = std::max(inner_temp, outer_temp);

    //printf("batter temp:%lf\n", temp);

    return INS_OK;
}

int32_t ins_battery::read_value(ins_i2c& i2c, int32_t type, int16_t& val)
{ 
    uint8_t low_half = 0;
    uint8_t high_half = 0;

    auto ret = i2c.read(_reg_arr[type][0], low_half);
    RETURN_IF_NOT_OK(ret);

    ret = i2c.read(_reg_arr[type][1], high_half);
    RETURN_IF_NOT_OK(ret);

    val = (int16_t)(high_half << 8 | low_half);

    return INS_OK;
}

double ins_battery::convert_k_to_c(int16_t k)
{
    double tmp = (double)k/10.0 - 273.15;
    tmp = ((double)((int)( (tmp + 0.005) * 100))) / 100;
	return tmp;
}
