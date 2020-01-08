
#include <stdio.h>
#include "log.h"

void logData(const char *date, const char *time, const char *file, const int line, const char *format, ...)
{
    char data[10000];
    va_list argList;
    short pos = 0;

    pos += sprintf(data, "%.6s %s ~ %s(%d): ", date, time, file, line);

    va_start(argList, format);
    vsprintf(&data[pos], format, argList);
    va_end(argList);

    puts(data);
}

