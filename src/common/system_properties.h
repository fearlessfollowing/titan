#ifndef _INCLUDE_SYS_SYSTEM_PROPERTIES_H
#define _INCLUDE_SYS_SYSTEM_PROPERTIES_H

#include <stdint.h>
#include <string>

int32_t system_properties_init();
std::string property_get(std::string key);
int32_t property_set(std::string key, std::string value);

#endif
