#include "odalaunch.h"

#include <string.h>

#if !defined(_WIN32)
#include <errno.h>
#endif

#define THREAD_IMPLEMENTATION
#include "thread.h"