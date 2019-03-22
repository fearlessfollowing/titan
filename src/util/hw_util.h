#ifndef _HW_UTIL_H_
#define _HW_UTIL_H_

#include <stdint.h>
#include <string>

#define GPIO_ROOT       "/sys/class/gpio"
#define GPIO_EXPORT     GPIO_ROOT "/export"
#define GPIO_UNEXPORT   GPIO_ROOT "/unexport"
#define GPIO_DIRECTION  GPIO_ROOT "/gpio%d/direction"
#define GPIO_ACTIVELOW  GPIO_ROOT "/gpio%d/active_low"
#define GPIO_VALUE      GPIO_ROOT "/gpio%d/value"


class hw_util {
public:
    static bool 	check_35mm_mic_on();
    static bool 	check_usb_mic_on();
    static void 	switch_fan(bool b_open);
    static int32_t 	get_temp(const std::string& property);
    static int32_t 	set_volume(uint32_t volume);
    static int32_t 	get_fb_res(int32_t& w, int32_t& h);

	/*
	 * gpio related
	 */
	static bool		gpio_write_value(std::string pathname, const char *buf, int32_t count);
	static bool		gpio_is_requested(int32_t gpio);	 
	static bool		gpio_request(int32_t gpio);
	static void		gpio_free(int32_t gpio);
	static bool 	gpio_direction_output(int32_t gpio, int value);
	static bool 	gpio_set_value(int32_t gpio, int value);
	
};

#endif
