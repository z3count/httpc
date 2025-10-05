#ifndef __LOGGER_H__
#define __LOGGER_H__

void logger__(const char *file, const char *func, const int line, const char *fmt, ...);

#define logger(...) logger__(__FILE__, __func__, __LINE__, __VA_ARGS__)


#endif // __LOGGER_H__
