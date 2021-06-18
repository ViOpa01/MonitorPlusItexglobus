#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vas.h"
#include "payvice.h"
#include "vasflow.h"
#include "vasbridge.h"
#include "virtualtid.h"
#include "../auxPayload.h"
#include "../menu_list.h"
#include "jsonwrapper/jsobject.h"

extern "C" {
#include "../util.h"
#include "libapi_xpos/inc/libapi_util.h"
}


int vasTransactionTypesBridge()
{
    MerchantData merchant;
    bool displayCashio = false;

	memset(&merchant, 0x00, sizeof(MerchantData));
	readMerchantData(&merchant);

    printf("Merchant type : %s\n", merchant.app_type);

	if (strcmp(merchant.app_type, "AGENCY") && strcmp(merchant.app_type, "CONVERTED")) {
        displayCashio = true;
    }

    return vasTransactionTypes(displayCashio);
}

int doVasCardTransaction(Eft* trxContext, unsigned long amount)
{
    MerchantData mParam = { 0 };
    NetWorkParameters netParam = { 0 };
    int ret = -1;
    pthread_t thread;

    trxContext->transType = EFT_PURCHASE;   // set to cash advance for upsl withdrawal
    trxContext->fromAccount = DEFAULT_ACCOUNT; //perform trxContext will update it if needed
    trxContext->toAccount = DEFAULT_ACCOUNT; //perform trxContext will update it if needed
    trxContext->isFallback = 0;
    trxContext->isVasTrans = 1;

    readMerchantData(&mParam);
    getNetParams(&netParam, CURRENT_PLATFORM, 0);

    if (trxContext->paymentInformation[0]) {
        trxContext->transType = EFT_CASHADVANCE;
    }

    if (trxContext->vas.switchMerchant) {

        char vtSecurity[97] = {'\0'};

        if (switchMerchantToVas((void*)trxContext) < 0) {
            return ret;
        }

        //TODO enable this section of code for upsl withdrawal
        if (trxContext->paymentInformation[0]) {
            if (getUpWithrawalField53(vtSecurity) < 0) {
                return -1;
            }

        }

        strncpy(trxContext->securityRelatedControlInformation, vtSecurity, sizeof(trxContext->securityRelatedControlInformation) -1);

    } else {
        MerchantParameters merchantParameters;
        char sessionKey[32 + 1] = { 0 };

        if (getSessionKey(sessionKey)) {
            printf("Error getting session key");
            return ret;
        }

        if (getParameters(&merchantParameters)) {
            printf("Error getting parameters\n");
            return ret;
        }
        strncpy(trxContext->sessionKey, sessionKey, sizeof(trxContext->sessionKey));
        strncpy(trxContext->terminalId, mParam.tid, strlen(mParam.tid));
        copyMerchantParams(trxContext, &merchantParameters);
    }

    strncpy(trxContext->posConditionCode, "00", sizeof(trxContext->posConditionCode));
    strncpy(trxContext->posPinCaptureCode, "04", sizeof(trxContext->posPinCaptureCode));
    strcpy(netParam.title, "vas");
    pthread_create(&thread, NULL, preDial, &mParam.gprsSettings);

    trxContext->vas.genAuxPayload = genAuxPayloadString;
    sprintf(trxContext->amount, "%012lu", amount);

    if((ret = performEft(trxContext, &netParam, (void*)&mParam, /*transTypeToTitle(EFT_PURCHASE)*/ "VAS")) < 0) {
        return ret;
    }

    return 0;

}

int requeryToContext(Eft* trxContext, const char* response) 
{
    /*
    {
        "error": false,
        "message": "success",
        "data": {
            "MTI": "0200",
            "processingCode": "001000",
            "amount": 5000,
            "terminalId": "2033GP25",
            "statusCode": "54",
            "maskedPan": "539983XXXXXX3775",
            "rrn": "210202113106",
            "STAN": "113118",
            "transactionTime": "2021-02-02T10:31:23.899Z",
            "handlerResponseTime": "2021-02-02T10:31:24.786Z",
            "merchantId": "203315000006045",
            "merchantAddress": "LA           LANG",
            "merchantCategoryCode": "8061",
            "messageReason": "Expired card",
            "responseCode": "54",
            "notified": "",
            "customerRef": "27613263~MTNVTU~2033GP25~MorefunH9|91200609000171|0.0.7(2021-02-02-11:31)"
        }
    }
    */
    iisys::JSObject json;

    if (!json.load(response)) {
        printf("hey 1\n");
        return -1;
    } else if (!json("error").isBool() || json("error").getBool() == true) {
        printf("Hey 2\n");
        return -1;
    }

    iisys::JSObject& data = json("data");

    if(data.isNull()) {
        printf("Hey 3\n");
        return -1;
    }

    strncpy(trxContext->responseCode, data("responseCode").getString().c_str(), 2);
    
    return 0;
}

int requeryMiddleware(Eft* trxContext, const char* tid)
{
    NetWorkParameters netParam = {'\0'};
    char path[256] = { 0 };
    char host[] = "ims.itexapp.com";
    char authorization[256] = "Authorization: ";
    int i = 0;
  
    strncpy((char*)netParam.host, host, strlen(host));
    netParam.receiveTimeout = 60000;
    strncpy(netParam.title, "Payvice", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 1;
    netParam.port = 443;
    netParam.endTag = ""; // I might need to comment it out later

    
    std::string panObj(trxContext->pan);
    if (!panObj.empty() && panObj.length() > (6 + 4)) {
        panObj.replace(panObj.begin() + 6, panObj.end() - 4, std::string(panObj.length() - 10, 'X'));
    }

    snprintf(path, sizeof(path), "/api/v1/transactions/transaction-callback?tid=%s&reference=%s&pan=%s&amount=%s"
    , tid, trxContext->rrn, panObj.c_str(), trxContext->amount);

    {
        char signature[128 + 1] = { 0 };

        std::string plainAuth = std::string(tid) + ":" + trxContext->rrn + ":" + "t9u4*&jEsLNdKo&M1A0Cp>Mco`?h.Dz<S`}L:[)UW+w)U^oz!*ROB82|6pyJ,l8";
        sha512Hex(signature, plainAuth.c_str(), plainAuth.length());

        i = (int)strlen(signature);
        while (i--) {
            signature[i] = tolower(signature[i]);
        }
        strcat(authorization, signature);
    }

    netParam.packetSize += sprintf((char*)(&netParam.packet[netParam.packetSize]), "GET %s HTTP/1.1\r\n", path);
    netParam.packetSize += sprintf((char*)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char*)(&netParam.packet[netParam.packetSize]), "%s\r\n\r\n", authorization);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -1;
    }

    std::string response = strchr((const char*)netParam.response, '{');
    if (response.empty()) {
        return -1;
    }

    return requeryToContext(trxContext, response.c_str());
}


int setupBaseHugeNetwork(NetWorkParameters* netParam, const char* path)
{
    char host[] = "basehuge.itexapp.com";

    strncpy((char*)netParam->host, host, strlen(host));
    netParam->receiveTimeout = 60000;
    strncpy(netParam->title, "Payvice", 10);
    netParam->isHttp = 1;
    netParam->isSsl = 0;
    netParam->port = 80;
    netParam->endTag = ""; // I might need to comment it out later

    netParam->packetSize += sprintf((char*)(&netParam->packet[netParam->packetSize]), "GET %s HTTP/1.1\r\n", path);
    netParam->packetSize += sprintf((char*)(&netParam->packet[netParam->packetSize]), "Host: %s:%d\r\n\r\n", netParam->host, netParam->port);

}

int performCashIn()
{
    return doCashIn();
}

int performCashOut()
{
    return doCashOut();
}

int isAgent()
{
    int ret = 0;

    const iisys::JSObject& isAgent = Payvice().object(Payvice::IS_AGENT);
    if (isAgent.isBool() && isAgent.getBool() == true) {
        ret = 1;
    }

    return ret;
}
