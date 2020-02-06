#include <string.h>
#include <stdio.h>

#include "vas.h"
#include "vasbridge.h"
#include "virtualtid.h"

int vasTransactionTypesBridge()
{
    return vasTransactionTypes();
}

int doVasCardTransaction(Eft* trxContext, unsigned long amount)
{
    MerchantData mParam = { 0 };
    NetWorkParameters netParam = { 0 };
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
            return -1;
    } else {
        MerchantParameters merchantParameters;
        char sessionKey[32 + 1] = { 0 };

        if (getSessionKey(sessionKey)) {
            printf("Error getting session key");
            return -1;
        }

        if (getParameters(&merchantParameters)) {
            printf("Error getting parameters\n");
            return -1;
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
    performEft(trxContext, &netParam, transTypeToTitle(EFT_PURCHASE));

    return 0;
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
