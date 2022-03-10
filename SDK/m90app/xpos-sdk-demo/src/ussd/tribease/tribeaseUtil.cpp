#include "tribeaseUtil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../vas/platform/simpio.h"

static const char* TRIBEASE_BASEURL = "197.253.19.78";
static const int TRIBEASE_PORT = 9802;

char* getAuthorizationHeader(char* headerBuf, size_t bufLen, const char* token)
{
    char authorizationHeader[0x1000] = { '\0' };
    if (sprintf(authorizationHeader, "Authorization: Bearer %s\r\n", token)
        >= bufLen) {
        UI_ShowButtonMessage(5000, "Error Getting Authorization Header (GAH)",
            " ", "Ok", UI_DIALOG_TYPE_WARNING);
        return (char*)NULL;
    }
    strncpy(headerBuf, authorizationHeader, bufLen);
    return headerBuf;
}

int buildRequestHeader(char* headerBuf, size_t bufLen, const char* route,
    int bodyLen, const char* additionalHeader)
{
    char requestHeader[0x1000] = { '\0' };
    int headerLen = 0;
    headerLen += sprintf(&requestHeader[headerLen],
        "POST %s HTTP/1.1\r\nHost: %s\r\n", route, TRIBEASE_BASEURL);
    headerLen += sprintf(
        &requestHeader[headerLen], "Content-Type: application/json\r\n");
    headerLen
        += sprintf(&requestHeader[headerLen], "Content-Length: %d\r\n%s\r\n",
            bodyLen, additionalHeader ? additionalHeader : "");
    if (headerLen >= bufLen) {
        UI_ShowButtonMessage(5000, "Error Building Request Header (BRH)", " ",
            "Ok", UI_DIALOG_TYPE_WARNING);
        return 0;
    }
    strncpy(headerBuf, requestHeader, bufLen);
    return headerLen;
}

void getTribeaseHostCredentials(NetWorkParameters* networkParams)
{
    strncpy(
        (char*)networkParams->host, TRIBEASE_BASEURL, strlen(TRIBEASE_BASEURL));
    networkParams->port = TRIBEASE_PORT;
    networkParams->endTag = "";
    networkParams->isSsl = 0;
    networkParams->isHttp = 1;
    networkParams->receiveTimeout = 60000;
    strncpy(networkParams->title, "Request", sizeof(networkParams->title));
}

short checkTribeaseResponseStatus(cJSON* json)
{
    cJSON* status;
    status = cJSON_GetObjectItemCaseSensitive(json, "status");
    if (cJSON_IsBool(status) && !cJSON_IsTrue(status)) {
        cJSON* message = cJSON_GetObjectItemCaseSensitive(json, "message");
        UI_ShowButtonMessage(5000,
            cJSON_IsString(message) ? message->valuestring : "Request Failed",
            " ", "Ok", UI_DIALOG_TYPE_WARNING);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
