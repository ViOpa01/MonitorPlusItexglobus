#ifndef VAS_UTILITY_H
#define VAS_UTILITY_H

#include <string>
#include "../jsonwrapper/jsobject.h"

namespace vasimpl {


void formattedDateTime(char* dateTime, size_t len);
int formatAmount(std::string& ulAmount);
iisys::JSObject getState();

int hex2bin(const char *pcInBuffer, char *pcOutBuffer, int iLen);
void generateRandomString(char* output, const size_t ouputSize);

std::string to_string(unsigned long);
std::string cardReference();

}


#endif
