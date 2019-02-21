#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <linux/ioctl.h>

#include "tx_gpio.h"

#define TEGRA_GPIO_BANK_ID_A 0
#define TEGRA_GPIO_BANK_ID_B 1
#define TEGRA_GPIO_BANK_ID_C 2
#define TEGRA_GPIO_BANK_ID_D 3
#define TEGRA_GPIO_BANK_ID_E 4
#define TEGRA_GPIO_BANK_ID_F 5
#define TEGRA_GPIO_BANK_ID_G 6
#define TEGRA_GPIO_BANK_ID_H 7
#define TEGRA_GPIO_BANK_ID_I 8
#define TEGRA_GPIO_BANK_ID_J 9
#define TEGRA_GPIO_BANK_ID_K 10
#define TEGRA_GPIO_BANK_ID_L 11
#define TEGRA_GPIO_BANK_ID_M 12
#define TEGRA_GPIO_BANK_ID_N 13
#define TEGRA_GPIO_BANK_ID_O 14
#define TEGRA_GPIO_BANK_ID_P 15
#define TEGRA_GPIO_BANK_ID_Q 16
#define TEGRA_GPIO_BANK_ID_R 17
#define TEGRA_GPIO_BANK_ID_S 18
#define TEGRA_GPIO_BANK_ID_T 19
#define TEGRA_GPIO_BANK_ID_U 20
#define TEGRA_GPIO_BANK_ID_V 21
#define TEGRA_GPIO_BANK_ID_W 22
#define TEGRA_GPIO_BANK_ID_X 23
#define TEGRA_GPIO_BANK_ID_Y 24
#define TEGRA_GPIO_BANK_ID_Z 25
#define TEGRA_GPIO_BANK_ID_AA 26
#define TEGRA_GPIO_BANK_ID_BB 27
#define TEGRA_GPIO_BANK_ID_CC 28
#define TEGRA_GPIO_BANK_ID_DD 29
#define TEGRA_GPIO_BANK_ID_EE 30
#define TEGRA_GPIO_BANK_ID_FF 31

#define TX_GPIO(bank, offset) ((TEGRA_GPIO_BANK_ID_##bank * 8) + offset)

#define TGERA_GPIO_HIGHT		(1)
#define TGERA_GPIO_LOW			(0)
#define TXGPIO_CDEV_E_MAJOR     (240)
#define TXGPIO_CDEV_E_MINOR     (12)
#define DEVICE_NAME 			"tgpio"

#define GPIO_CMD_MAGIC     		 'x'       
#define GPIO_CMD_MAX_NR           (6)    

#define GPIO_CMD_DIR_OUTPUT       _IO(GPIO_CMD_MAGIC, 1)
#define GPIO_CMD_DIR_INPUT        _IO(GPIO_CMD_MAGIC, 2)
#define GPIO_CMD_GET_VALUE        _IO(GPIO_CMD_MAGIC, 3)
#define GPIO_CMD_SET_VALUE        _IO(GPIO_CMD_MAGIC, 4)
#define GPIO_CMD_GET_CFG		  _IO(GPIO_CMD_MAGIC, 5)


typedef struct _tgpio_data
{
	unsigned int  gpio;
	unsigned char dir;
	unsigned char value;
}tgpio_data;


static int g_fd = -1;

#define TX_GPIO_DEV	 "/dev/tgpio"

int tx_gpio_init()
{
	g_fd = open(TX_GPIO_DEV,O_RDWR);
	if(g_fd < 0)
	{
		return -1;
	}
	//printf("open dev ok %d.\n",g_fd);
	return 0;
}


int tx_gpio_exit()
{
	if(g_fd > 0)
	{
		close(g_fd);
	}
	//printf("close dev ok.\n");
	return 0;
}

int tx_gpio_set_output(unsigned int gpio,int val)
{
	tgpio_data d;
	d.gpio = gpio;
	d.dir  = 1;
	d.value = val;
	if(g_fd > 0)
	{
		ioctl(g_fd, GPIO_CMD_DIR_OUTPUT,  &d);
	}
	return 0;
}

int tx_gpio_set_input(unsigned int gpio)
{
	tgpio_data d;
	d.gpio = gpio;
	d.dir  = 0;
	//d.value = val;
	if(g_fd > 0)
	{
		ioctl(g_fd, GPIO_CMD_DIR_INPUT,  &d);
	}
	return 0;
}

int tx_gpio_set_value(unsigned int gpio ,unsigned char val)
{
	tgpio_data d;
	d.gpio = gpio;
	d.dir  = 1;
	d.value = val;
	if(g_fd > 0)
	{
		ioctl(g_fd, GPIO_CMD_SET_VALUE,  &d);
	}
	return 0;
}

int tx_gpio_get_value(unsigned int gpio)
{
	tgpio_data d;
	d.gpio = gpio;
	d.dir  = 1;
	if(g_fd > 0)
	{
		ioctl(g_fd, GPIO_CMD_GET_VALUE,  &d);
	}

	return (!d.value);
}

int tx_gpio_get_dir(unsigned int gpio)
{
	tgpio_data d;
	d.gpio = gpio;
	if(g_fd > 0)
	{
		ioctl(g_fd, GPIO_CMD_GET_CFG,  &d);
	}

	return d.dir;
}

#if 1
//printf("./test gpio o/i vol\n");
int main(int argc,char *argv[])
{
	int gpio;
	int val;
	int dir;
	char buff[1024];
	tx_gpio_init();
	
	if(argc == 2)
	{
		gpio = atoi(argv[1]);
		printf("gpio%d dir = %s val =%s\n",gpio,(tx_gpio_get_dir(gpio) == 0)?"out":"in",(tx_gpio_get_value(gpio) == 0)?"hi":"low");
	}	
	else if(argc == 3)
	{
		gpio = atoi(argv[1]);
		printf("gpio%d dir = %s val =%s\n",gpio,(tx_gpio_get_dir(gpio) == 0)?"out":"in",(tx_gpio_get_value(gpio) == 0)?"hi":"low");
		val  = atoi(argv[2]);
		tx_gpio_set_value(gpio,val);
	}
	else if(argc == 4)
	{//demo 85 0 0
		gpio = atoi(argv[1]);
		dir  = atoi(argv[2]);
		val  = atoi(argv[3]);
		if(dir == 1)
		{
			tx_gpio_set_output(gpio,val);	
		}
		else
		{
			tx_gpio_set_input(gpio);
		}
		tx_gpio_set_value(gpio,val);
		printf("gpio%d dir = %s val =%s\n",gpio,(tx_gpio_get_dir(gpio) == 0)?"out":"in",(tx_gpio_get_value(gpio) == 0)?"hi":"low");
	}
	tx_gpio_exit();
	return 0;
}
#endif

