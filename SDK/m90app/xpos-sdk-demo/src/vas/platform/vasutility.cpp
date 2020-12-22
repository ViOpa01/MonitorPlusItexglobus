#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "vasutility.h"

extern "C" {
#include "../../util.h"
}

void vasimpl::formattedDateTime(char* dateTime, size_t len)
{
    time_t now = time(NULL);
    strftime(dateTime, len, "%Y-%m-%d %H:%M:%S", localtime(&now));
}

iisys::JSObject vasimpl::getState()
{
    return iisys::JSObject();
}

int vasimpl::formatAmount(std::string& ulAmount)
{
    int position;

    if (ulAmount.empty())
        return -1;

    position = ulAmount.length();
    if ((position -= 2) > 0) {
        ulAmount.insert(position, ".");
    } else if (position == 0) {
        ulAmount.insert(0, "0.");
    } else {
        ulAmount.insert(0, "0.0");
    }

    while ((position -= 3) > 0) {
        ulAmount.insert(position, ",");
    }

    return 0;
}

int vasimpl::hex2bin(const char* pcInBuffer, char* pcOutBuffer, int iLen)
{
    int c;
    char* p;
    int iBytes = 0;
    char hextable[] = "0123456789ABCDEF";
    int nibble = 0;
    int nibble_val;

    while (iBytes < iLen) {
        c = *pcInBuffer++;
        if (c == 0)
            break;

        c = toupper(c);

        p = strchr(hextable, c);
        if (p) {
            if (nibble & 1) {
                iBytes++;
                *pcOutBuffer = (nibble_val << 4 | (p - hextable));
                pcOutBuffer++;
            } //if
            else {
                nibble_val = (p - hextable);
            } //else
            nibble++;
        } //if
        else {
            nibble = 0;
        } //else
    } //while

    return iBytes;
} //hex2bin

void vasimpl::generateRandomString(char* output, const size_t ouputSize)
{
    int i = 0;
    char charset[] = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789";
    static int init = 0;

    if (!init) {
        unsigned long seed = time(NULL);
        srand(seed);
        init++;
    }

    for (i = 0; i < ouputSize - 1; ++i) {
        *(output + i) = charset[rand() % (sizeof(charset) - 1)];
    }
    *(output + i) = 0;
}

std::string vasimpl::to_string(unsigned long val)
{
    char str[32] = { 0 };

    snprintf(str, sizeof(str), "%lu", val);
    
    return std::string(str);
}

std::string vasimpl::cardReference()
{
    char timestamp[32] = { 0 };
/*
    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);

    snprintf(timestamp, sizeof(timestamp), "%d%02d%02d%02d%02d%02d"
    , abs(now_tm->tm_year - 100), now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);

    printf("tm year : %d\n", now_tm->tm_year);
`*/
    getRrn(timestamp);
    printf("card reference : %s\n", timestamp);
    return std::string(timestamp);

}
