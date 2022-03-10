#include "tribeaseGetAuthToken.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../vas/platform/simpio.h"
#include "merchant.h"
#include "tribeaseUtil.h"

static const char* API_KEY = "rKmd9uJt6nyUsw2XKZjLbZWr5wch9gyznFRdNHgzVjk6ZB7TA"
                             "UPYb93c5MUfW6BhL6cCj6W8rwBZ8kqEdb";

static short buildTribeaseGetAuthTokenRequest(NetWorkParameters* networkParams)
{
    char requestBody[0x100] = { '\0' };
    char requestHeader[0x100] = { '\0' };
    const char* route = "/api/v1/auth/token";
    int headerLen = 0, bodyLen = 0;
    MerchantData mParam = { 0 };

    if (readMerchantData(&mParam)) {
        return EXIT_FAILURE;
    }
    bodyLen = sprintf(requestBody,
        "{\"terminalId\": \"%s\", \"apikey\":\"%s\"}", mParam.tid, API_KEY);

    if ((headerLen = buildRequestHeader(requestHeader, sizeof(requestHeader),
             route, bodyLen, (const char*)NULL))
        == 0) {
        return EXIT_FAILURE;
    }
    sprintf((char*)networkParams->packet, "%s%s", requestHeader, requestBody);
    networkParams->packetSize = headerLen + bodyLen;
    return EXIT_SUCCESS;
}

static short copyMerchantPhoneNumberFromResponse(
    TribeaseClient* tribeaseClient, cJSON* json)
{
    cJSON *temp, *merchantData, *merchantPhoneNo;

    if (json == NULL) {
        UI_ShowButtonMessage(5000, "(TC-01)Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return EXIT_FAILURE;
    }
    merchantData = cJSON_GetObjectItemCaseSensitive(json, "merchant");
    if (!cJSON_IsObject(merchantData)) {
        UI_ShowButtonMessage(5000, "(TC-02)Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return EXIT_FAILURE;
    }
    temp = cJSON_GetObjectItemCaseSensitive(merchantData, "data");
    if (!cJSON_IsObject(temp)) {
        UI_ShowButtonMessage(5000, "(TC-03)Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return EXIT_FAILURE;
    }
    merchantPhoneNo = cJSON_GetObjectItemCaseSensitive(temp, "phone");
    if (!cJSON_IsString(merchantPhoneNo)) {
        UI_ShowButtonMessage(5000, "Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return EXIT_FAILURE;
    }

    strncpy(tribeaseClient->merchantPhoneNo, merchantPhoneNo->valuestring,
        sizeof(tribeaseClient->merchantPhoneNo));
    return EXIT_SUCCESS;
}

static short copyTokenAndMerchantPhoneNoFromResponse(
    TribeaseClient* tribeaseClient, const char* response)
{
    short ret = EXIT_FAILURE;
    cJSON *tokenJsonObj, *data;
    cJSON* json = cJSON_Parse(strchr(response, '{'));

    if (json == NULL) {
        UI_ShowButtonMessage(5000, "(TC-01)Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return ret;
    }
    if (checkTribeaseResponseStatus(json) != EXIT_SUCCESS) {
        goto clean_exit;
    }
    data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (!cJSON_IsObject(data)) {
        UI_ShowButtonMessage(5000, "(TC-02)Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        goto clean_exit;
    }
    tokenJsonObj = cJSON_GetObjectItemCaseSensitive(data, "token");
    if (!cJSON_IsString(tokenJsonObj)) {
        UI_ShowButtonMessage(5000, "(TC-03)Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        goto clean_exit;
    }
    if (copyMerchantPhoneNumberFromResponse(tribeaseClient, json) != EXIT_SUCCESS) {
        UI_ShowButtonMessage(5000, "Unexpected Response", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        goto clean_exit;
    }
    printf("Tribease Merchant: %s\n", tribeaseClient->merchantPhoneNo);

    strncpy(tribeaseClient->authToken, tokenJsonObj->valuestring,
        sizeof(tribeaseClient->authToken));
    ret = EXIT_SUCCESS;

clean_exit:
    cJSON_Delete(json);
    return ret;
}

short tribeaseGetAuthToken(TribeaseClient* tribeaseClient)
{
    NetWorkParameters networkParams = { '\0' };

    getTribeaseHostCredentials(&networkParams);

    if (buildTribeaseGetAuthTokenRequest(&networkParams) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (sendAndRecvPacket(&networkParams) != SEND_RECEIVE_SUCCESSFUL) {
        return EXIT_FAILURE;
    }
    return copyTokenAndMerchantPhoneNoFromResponse(tribeaseClient, (char*)networkParams.response);
}
