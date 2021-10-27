#ifndef __NX_SYSTEM_H__
#define __NX_SYSTEM_H__

#include <switch.h>
#include "d_event.h"

// This is added for compatibility with strptime.cpp and cmdlib.cpp
char * strptime(const char *buf, const char *fmt, struct tm *timeptr);  

void STACK_ARGS nx_early_init (void);
void STACK_ARGS nx_early_deinit (void);

#endif
