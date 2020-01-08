#ifndef _OPE_LOG_H
#define _OPE_LOG_H

#include <stdarg.h>

#define LOG_PRINTF(...) logData(__DATE__, __TIME__, __FILE__, __LINE__, __VA_ARGS__)

#endif
