#include <stdio.h>
#include <stdarg.h>

#include "logger.h"

#define MAX_LOG_LEN 1024

void logger__(const char *file, const char *func, const int line, const char *fmt, ...)
{
    va_list arg;
    char msg[MAX_LOG_LEN];

    va_start(arg, fmt);
    (void) vsnprintf(msg, sizeof msg, fmt, arg);
    va_end(arg);

    fprintf(stderr, "[%s:%s:%d] %s\n", file, func, line, msg);
}
