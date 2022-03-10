#ifndef TRIBEASE_UTIL_H
#define TRIBEASE_UTIL_H

extern "C" {
#include "cJSON.h"
#include "network.h"
}

int buildRequestHeader(char* headerBuf, size_t bufLen, const char* route,
    int bodyLen, const char* additionalHeader);

short checkTribeaseResponseStatus(cJSON* json);

char* getAuthorizationHeader(char* headerBuf, size_t bufLen, const char* token);

void getTribeaseHostCredentials(NetWorkParameters* networkParams);

#endif // TRIBEASE_UTIL_H
