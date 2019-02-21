
#ifndef _INS_SIGNAL_H_
#define _INS_SIGNAL_H_

#include <stdint.h>

void wait_signal_terminate();
int32_t singleton(const char* lock_file);
void set_sigal_handle(int32_t sig);

#endif