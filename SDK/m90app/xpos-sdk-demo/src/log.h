#ifndef _OPE_LOG_H
#define _OPE_LOG_H

#include <stdarg.h>

void logData(const char *date, const char *time, const char *file, const int line, const char *format, ...);

#define LOG_PRINTF(...) logData(__DATE__, __TIME__, __FILE__, __LINE__, __VA_ARGS__)

#endif
