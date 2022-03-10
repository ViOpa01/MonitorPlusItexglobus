#include "tribeaseValidatePaymentToken.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../vas/platform/simpio.h"
#include "tribeaseUtil.h"

static short buildTribeaseValidatePaymentTokenRequest(
    TribeaseClient* tribeaseClient, NetWorkParameters* networkParams)
{
    char requestBody[0x100] = { '\0' };
    char requestHeader[0x500] = { '\0' };
    char authorization[0x400] = { '\0' };
    const char* route = "/api/v1/payment/token/verify";
    int headerLen = 0, bodyLen = 0;

    bodyLen = sprintf(requestBody, "{\"token\":%d}", tribeaseClient->token);

    if ((headerLen = buildRequestHeader(requestHeader, sizeof(requestHeader),
             route, bodyLen,
             getAuthorizationHeader(authorization, sizeof(authorization),
                 tribeaseClient->authToken)))
        == 0) {
        return EXIT_FAILURE;
    }
    sprintf((char*)networkParams->packet, "%s%s", requestHeader, requestBody);
    networkParams->packetSize = headerLen + bodyLen;
    return EXIT_SUCCESS;
}

static short copyDataFromPaymentTokenResponse(
    TribeaseClient* tribeaseClient, const char* response)
{
    short ret = EXIT_FAILURE;
    cJSON* message;
    // cJSON *message, *transactionRef;
    cJSON* json = cJSON_Parse(strchr(response, '{'));

    if (json == NULL) {
        UI_ShowButtonMessage(5000, "Unexpected Response (TC-07)", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return ret;
    }
    if (checkTribeaseResponseStatus(json) != EXIT_SUCCESS) {
        tribeaseClient->transactionStatus = 0;
        goto clean_exit;
    }
    tribeaseClient->transactionStatus = 1;
    message = cJSON_GetObjectItemCaseSensitive(json, "message");
    // transactionRef = cJSON_GetObjectItemCaseSensitive(json,
    // "transactionRef");
    if (!cJSON_IsString(message)) {
        // if (!cJSON_IsString(message) || !cJSON_IsString(transactionRef)) {
        UI_ShowButtonMessage(5000, "Unexpected Response (TC-08)", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        goto clean_exit;
    }

    strncpy(tribeaseClient->responseMessage, message->valuestring,
        sizeof(tribeaseClient->responseMessage));
    // strncpy(tribeaseClient->transactionRef, transactionRef->valuestring,
    // sizeof(tribeaseClient->transactionRef));
    ret = EXIT_SUCCESS;

clean_exit:
    cJSON_Delete(json);
    return ret;
}

short tribeaseValidatePaymentToken(TribeaseClient* tribeaseClient)
{
    NetWorkParameters networkParams = { '\0' };
    const int TRIES = 3;
    char tokenBuf[32] = { '\0' };
    int i = 0;

    getTribeaseHostCredentials(&networkParams);
    if (buildTribeaseValidatePaymentTokenRequest(tribeaseClient, &networkParams)
        != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    sprintf(tokenBuf, "%d", tribeaseClient->token);

    for (i = 0; i < TRIES; i++) {
        UI_ShowButtonMessage(
            60000, "TRIBEASE TOKEN", tokenBuf, "Ok", UI_DIALOG_TYPE_WARNING);
        sendAndRecvPacket(&networkParams);
        copyDataFromPaymentTokenResponse(
            tribeaseClient, (char*)networkParams.response);
        if (tribeaseClient->transactionStatus) {
            break;
        }
    }
    return EXIT_SUCCESS;
}
