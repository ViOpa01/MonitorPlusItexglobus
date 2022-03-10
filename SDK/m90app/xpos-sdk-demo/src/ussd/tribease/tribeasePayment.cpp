#include "tribeasePayment.h"

#include <string.h>

#include "../../vas/payvice.h"
#include "../../vas/platform/simpio.h"
#include "tribeaseUtil.h"

static char* getTribeaseRef(TribeaseClient* tribeaseClient)
{
    std::string clientReference = getClientReference();

    strncpy(tribeaseClient->ref, clientReference.c_str(),
        sizeof(tribeaseClient->ref));
    return tribeaseClient->ref;
}

static int buildTribeasePaymentRequestBody(
    TribeaseClient* tribeaseClient, char* requestBodyBuf, size_t bufLen)
{
    char requestBody[0x1000] = { '\0' };
    int bodyLen = 0;

    if ((bodyLen = sprintf(requestBody,
             "{\"amount\":%.2lf,\"merchantPhoneNumber\":\"%s\","
             "\"customerPhoneNumber\":\"234%s\","
             "\"customerName\":\"%s\",\"clientReference\":\"%s\","
             "\"paymentReference\":\"%s\",\"customerPin\":%s}",
             tribeaseClient->amount / 100.0, tribeaseClient->merchantPhoneNo,
             &tribeaseClient->customerPhoneNo[1], tribeaseClient->name,
             getTribeaseRef(tribeaseClient), tribeaseClient->paymentRef,
             tribeaseClient->pin))
        >= bufLen) {
        UI_ShowButtonMessage(5000, "Error Building Request Body (BT-PRB)", " ",
            "Ok", UI_DIALOG_TYPE_WARNING);
        return 0;
    }

    strncpy(requestBodyBuf, requestBody, bufLen);
    return bodyLen;
}

static short buildTribeasePaymentRequest(
    TribeaseClient* tribeaseClient, NetWorkParameters* networkParams)
{
    char requestBody[0x200] = { '\0' };
    char requestHeader[0x500] = { '\0' };
    char authorization[0x400] = { '\0' };
    const char* route = "/api/v1/transaction/payment";
    int headerLen = 0, bodyLen = 0;

    if ((bodyLen = buildTribeasePaymentRequestBody(
             tribeaseClient, requestBody, sizeof(requestBody)))
        == 0) {
        return EXIT_FAILURE;
    }
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

static short copyDataFromPaymentResponse(
    TribeaseClient* tribeaseClient, const char* response)
{
    short ret = EXIT_FAILURE;
    cJSON *message, *transactionRef;
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
    transactionRef = cJSON_GetObjectItemCaseSensitive(json, "transactionRef");
    if (!cJSON_IsString(message) || !cJSON_IsString(transactionRef)) {
        UI_ShowButtonMessage(5000, "Unexpected Response (TC-08)", " ", "Ok",
            UI_DIALOG_TYPE_WARNING);
        goto clean_exit;
    }

    strncpy(tribeaseClient->responseMessage, message->valuestring,
        sizeof(tribeaseClient->responseMessage));
    strncpy(tribeaseClient->transactionRef, transactionRef->valuestring,
        sizeof(tribeaseClient->transactionRef));
    ret = EXIT_SUCCESS;

clean_exit:
    cJSON_Delete(json);
    return ret;
}

static short getTribeasePin(TribeaseClient* tribeaseClient)
{
    std::string pin;
    if (getPin(pin, "TRIBEASE PIN") != EMV_CT_PIN_INPUT_OKAY) {
        memset(tribeaseClient->pin, '\0', sizeof(tribeaseClient->pin));
        return EXIT_FAILURE;
    }
    strncpy(tribeaseClient->pin, pin.c_str(), sizeof(tribeaseClient->pin));
    return EXIT_SUCCESS;
}

short tribeasePayment(TribeaseClient* tribeaseClient)
{
    NetWorkParameters networkParams = { '\0' };

    if (getTribeasePin(tribeaseClient) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    getTribeaseHostCredentials(&networkParams);

    if (buildTribeasePaymentRequest(tribeaseClient, &networkParams)
        != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (sendAndRecvPacket(&networkParams) != SEND_RECEIVE_SUCCESSFUL) {
        return EXIT_FAILURE;
    }
    return copyDataFromPaymentResponse(
        tribeaseClient, (char*)networkParams.response);
}
