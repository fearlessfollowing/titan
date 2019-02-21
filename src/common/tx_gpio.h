#ifndef __TX_GPIO_H
#define __TX_GPIO_H

int tx_gpio_init();
int tx_gpio_exit();
int tx_gpio_set_output(unsigned int gpio,int val);
int tx_gpio_set_input(unsigned int gpio);
int tx_gpio_set_value(unsigned int gpio ,unsigned char val);
int tx_gpio_get_value(unsigned int gpio);
int tx_gpio_get_dir(unsigned int gpio);

#endif
