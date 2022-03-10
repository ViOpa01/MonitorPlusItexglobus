#include "tribeaseValidation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../vas/platform/simpio.h"
#include "tribeaseUtil.h"

static short buildTribeaseValidationRequest(
    TribeaseClient* tribeaseClient, NetWorkParameters* networkParams)
{
    char requestBody[0x100] = { '\0' };
    char requestHeader[0x500] = { '\0' };
    char authorization[0x400] = { '\0' };
    const char* route = "/api/v1/transaction/validate";
    int headerLen = 0, bodyLen = 0;

    bodyLen = sprintf(requestBody,
        "{\"amount\":%.2lf,\"merchantPhoneNumber\":\"%s\","
        "\"customerPhoneNumber\":\"234%s\"}",
        tribeaseClient->amount / 100.0, tribeaseClient->merchantPhoneNo,
        &tribeaseClient->customerPhoneNo[1]);

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

static short copyTribeaseCLientDataFromResponse(
    TribeaseClient* tribeaseClient, const char* response)
{
    short ret = EXIT_FAILURE;
    cJSON *name, *paymentRef, *data;
    cJSON* json = cJSON_Parse(strchr(response, '{'));

    if (json == NULL) {
        UI_ShowButtonMessage(5000, "Unexpected Response (TC-04)", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        return ret;
    }
    if (checkTribeaseResponseStatus(json) != EXIT_SUCCESS) {
        goto clean_exit;
    }
    data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (!cJSON_IsObject(data)) {
        UI_ShowButtonMessage(5000, "Unexpected Response (TC-05)", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        goto clean_exit;
    }
    name = cJSON_GetObjectItemCaseSensitive(data, "name");
    paymentRef = cJSON_GetObjectItemCaseSensitive(data, "payment_reference");
    if (!cJSON_IsString(name) || !cJSON_IsString(paymentRef)) {
        UI_ShowButtonMessage(5000, "Unexpected Response (TC-06)", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        goto clean_exit;
    }

    strncpy(
        tribeaseClient->name, name->valuestring, sizeof(tribeaseClient->name));
    strncpy(tribeaseClient->paymentRef, paymentRef->valuestring,
        sizeof(tribeaseClient->paymentRef));
    ret = EXIT_SUCCESS;

clean_exit:
    cJSON_Delete(json);
    return ret;
}

short tribeaseValidation(TribeaseClient* tribeaseClient)
{
    NetWorkParameters networkParams = { '\0' };

    getTribeaseHostCredentials(&networkParams);

    if (buildTribeaseValidationRequest(tribeaseClient, &networkParams)
        != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (sendAndRecvPacket(&networkParams) != SEND_RECEIVE_SUCCESSFUL) {
        return EXIT_FAILURE;
    }
    return copyTribeaseCLientDataFromResponse(
        tribeaseClient, (char*)networkParams.response);
}
