#include <string.h>
#include <stdio.h>

#include "vas.h"
#include "vasbridge.h"
#include "virtualtid.h"

extern "C" {
#include "util.h"
}

int vasTransactionTypesBridge()
{
    return vasTransactionTypes();
}

int doVasCardTransaction(Eft* trxContext, unsigned long amount)
{
    MerchantData mParam = { 0 };
    NetWorkParameters netParam = { 0 };
    int ret = -1;
    pthread_t thread;

    trxContext->transType = EFT_PURCHASE;
    trxContext->fromAccount = DEFAULT_ACCOUNT; //perform trxContext will update it if needed
    trxContext->toAccount = DEFAULT_ACCOUNT; //perform trxContext will update it if needed
    trxContext->isFallback = 0;
    trxContext->isVasTrans = 1;

    readMerchantData(&mParam);
    getNetParams(&netParam, CURRENT_PLATFORM, 0);

    if (trxContext->switchMerchant) {
        if (swithMerchantToVas(trxContext) < 0)
            return ret;
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

    // populateEchoData(trxContext->echoData);  TODO: Maybe append this later

    strncpy(trxContext->posConditionCode, "00", sizeof(trxContext->posConditionCode));
    strncpy(trxContext->posPinCaptureCode, "04", sizeof(trxContext->posPinCaptureCode));
    strcpy(netParam.title, transTypeToTitle(EFT_PURCHASE));
    pthread_create(&thread, NULL, preDial, &mParam.gprsSettings);

    sprintf(trxContext->amount, "%012lu", amount);
    if((ret = performEft(trxContext, &netParam, transTypeToTitle(EFT_PURCHASE))) < 0) {
        return ret;
    }

    return 0;
}

int requeryToContext(Eft* trxContext, const char* response) 
{
    // {
    //     "error": false,
    //     "message": "success",
    //     "data": {
    //         "MTI": "0200",
    //         "processingCode": "001000",
    //         "amount": 100,
    //         "terminalId": "2070HE88",
    //         "statusCode": "92",
    //         "maskedPan": "541333XXXXXX0011",
    //         "rrn": "191204084500",
    //         "STAN": "084509",
    //         "transactionTime": "2019-12-04T07:46:34.514Z",
    //         "handlerResponseTime": "2019-12-04T07:46:37.023Z",
    //         "merchantId": "FBP205600444741",
    //         "merchantAddress": "KD LANG",
    //         "merchantCategoryCode": "5621",
    //         "messageReason": "Routing error",
    //         "responseCode": "92",
    //         "notified": "",
    //         "customerRef": "paga~07038085747~PAX|32645884|7.8.17PG"
    //     }
    // }
    iisys::JSObject json;

    if (!json.load(response)) {
        return -1;
    } else if (!json("error").isBool() || json("error").getBool() == true) {
        return -1;
    }

    iisys::JSObject& data = json("data");

    if(data.isNull()) {
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

    snprintf(path, sizeof(path), "/api/v1/transactions/transaction-callback?tid=%s&reference=%s&pan=%s&amount=%lu"
    , tid, trxContext->rrn, panObj.c_str(), trxContext->amount);

    {
        std::string plainAuth = std::string(tid) + ":" + trxContext->rrn + ":" + "t9u4*&jEsLNdKo&M1A0Cp>Mco`?h.Dz<S`}L:[)UW+w)U^oz!*ROB82|6pyJ,l8";
        sha512Hex(authorization + strlen(authorization), plainAuth.c_str(), plainAuth.length());
    }

    netParam.packetSize += sprintf((char*)(&netParam.packet[netParam.packetSize]), "GET %s HTTP/1.1\r\n", path);
    netParam.packetSize += sprintf((char*)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char*)(&netParam.packet[netParam.packetSize]), "%s\r\n\r\n", authorization);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -1;
    }


    return requeryToContext(trxContext, (char *)netParam.response);
}


short setupBaseHugeNetwork(NetWorkParameters* netParam, const char* path)
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
