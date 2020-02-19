#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "payvice.h"
#include "jsobject.h"
#include "merchant.h"


#include "vas.h"
#include "vasbridge.h"
#include "util.h"

#include "virtualtid.h"

extern "C" {
#include "libapi_xpos/inc/libapi_util.h"
}

int copyVirtualPinBlock(Eft *pTrxContext, const char* pinBlock)
{
    MerchantData mParam = {'\0'};

    char pinKeyBcd[16];
    char vPinKeyBcd[16];
    char clearKey[33] = {'\0'};

    char actualPinBlockBcd[8] = {0};
    char plainPinBlockBcd[8] = {0};

    char pinKey[32 + 1] = { 0 };
    char vPinKey[32 + 1] = { 0 };

    Payvice payvice;
    if (!loggedIn(payvice)) {
        return -1;
    }

    readMerchantData(&mParam);
    strncpy(clearKey, mParam.pKey, sizeof(mParam.pKey) - 1);
    printf("Clear Pin Key : %s\n", clearKey);
    
    memcpy(actualPinBlockBcd, pinBlock, 8);

    strncpy(pinKey, clearKey, sizeof(pinKey) - 1); 
    strncpy(vPinKey, payvice.object(Payvice::VIRTUAL)(Payvice::PKEY).getString().c_str(), sizeof(vPinKey) - 1);

    printf("virtual Pin Key : %s\n", vPinKey);

    Util_Asc2Bcd(pinKey, pinKeyBcd, strlen(pinKey));
    Util_Asc2Bcd(vPinKey, vPinKeyBcd, strlen(vPinKey));


    if (Util_Des(3, pinKeyBcd, actualPinBlockBcd, plainPinBlockBcd)) {
        return -1;
    } else if (Util_Des(2, vPinKeyBcd, plainPinBlockBcd, (char*)pTrxContext->pinDataBcd)) {
        return -1;
    }

    pTrxContext->pinDataBcdLen = 8;

    return 0;
}

bool virtualConfigurationIsSet()
{
    iisys::JSObject virtualConfig = Payvice().object(Payvice::VIRTUAL);
    if (virtualConfig.isNull()) {
        return false;
    } else if (virtualConfig(Payvice::TID).isNull() || virtualConfig(Payvice::MID).isNull() || virtualConfig(Payvice::NAME).isNull()
        || virtualConfig(Payvice::PKEY).isNull() || virtualConfig(Payvice::SKEY).isNull() || virtualConfig(Payvice::MCC).isNull()
         || virtualConfig(Payvice::CUR_CODE).isNull() )
    {
        return false;
    } 

    return true;
}

int resetVirtualConfiguration()
{
    NetWorkParameters netParam = {'\0'};
    Payvice payvice;
    std::string urlPath = "/tams/eftpos/op/xmerchant.php?tid=";

    payvice.object.erase(Payvice::VIRTUAL);
    payvice.save();

    urlPath += getDeviceTerminalId();

    setupBaseHugeNetwork(&netParam, urlPath.c_str());

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        printf("Send and recceive failed");
        return -1;
    }

    iisys::JSObject detail;
    
    if (!detail.load((const char *)bodyPointer((const char *)netParam.response))) {
        printf("Unable to parse body -> %s\n", (const char *)netParam.response);
        return -1;
    }

    payvice.object(Payvice::VIRTUAL) = detail;
    payvice.save();

    return 0;
}

void* asynchResetVirtualConfig(void*)
{
    resetVirtualConfiguration();
    return NULL;
}

void resetVirtualConfigurationAsync()
{
    pthread_t configThread;
    pthread_create(&configThread, NULL, asynchResetVirtualConfig, NULL);
}

int swithMerchantToVas(Eft* trxContext)
{
    Payvice payvice;

    if (!virtualConfigurationIsSet()) {
        resetVirtualConfiguration();
        return -1;
    }

    strncpy(trxContext->merchantType, payvice.object(Payvice::VIRTUAL)(Payvice::MCC).getString().c_str(), sizeof(trxContext->merchantType) - 1);
    strncpy(trxContext->merchantId, payvice.object(Payvice::VIRTUAL)(Payvice::MID).getString().c_str(), sizeof(trxContext->merchantId) - 1);
    strncpy(trxContext->merchantName, payvice.object(Payvice::VIRTUAL)(Payvice::NAME).getString().c_str(), sizeof(trxContext->merchantName) - 1);
    strncpy(trxContext->currencyCode, payvice.object(Payvice::VIRTUAL)(Payvice::CUR_CODE).getString().c_str(), sizeof(trxContext->currencyCode) - 1);
    strncpy(trxContext->terminalId, payvice.object(Payvice::VIRTUAL)(Payvice::TID).getString().c_str(), sizeof(trxContext->terminalId) - 1);
    strncpy(trxContext->sessionKey, payvice.object(Payvice::VIRTUAL)(Payvice::SKEY).getString().c_str(), sizeof(trxContext->sessionKey) - 1);

    return 0;
}
