#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#ifdef _VRXEVO
#include <eoslog.h>
#include <svc_sec.h>
#else
#include <unistd.h>
#endif

#include "payvice.h"
#include "vas.h"
#include "virtualtid.h"
#include "vasbridge.h"
#include "platform/vasdevice.h"

extern "C" {
#include "../util.h"
#include "../Nibss8583.h"
#include "libapi_xpos/inc/libapi_util.h"
}

// encrypted pinblock, terminal pinkey, output new vas pinblock

int getVirtualPinBlock(unsigned char* virtualPinBlock, const unsigned char* actualPinBlock, const char* pinkey)
{
    char pinKeyBcd[16];
    char vPinKeyBcd[16];
    char clearKey[33] = {'\0'};

    char actualPinBlockBcd[8] = {0};
    char plainPinBlockBcd[8] = {0};

    char vPinKey[32 + 1] = { 0 };

    Payvice payvice;
    if (!loggedIn(payvice)) {
        return -1;
    }

    strncpy(clearKey, pinkey, strlen(pinkey));
    printf("Clear Pin Key : %s\n", clearKey);

    strncpy(vPinKey, payvice.object(Payvice::VIRTUAL)(Payvice::PKEY).getString().c_str(), sizeof(vPinKey) - 1);
    printf("virtual Pin Key : %s\n", vPinKey);

    memcpy(actualPinBlockBcd, actualPinBlock, 8);

    Util_Asc2Bcd(clearKey, pinKeyBcd, strlen(clearKey));
    Util_Asc2Bcd(vPinKey, vPinKeyBcd, strlen(vPinKey));

    if (Util_Des(3, pinKeyBcd, actualPinBlockBcd, plainPinBlockBcd)) {
        return -1;
    } else if (Util_Des(2, vPinKeyBcd, plainPinBlockBcd, (char*)virtualPinBlock)) {
        return -1;
    }

    return 0;
}

bool virtualConfigurationIsSet()
{
    iisys::JSObject virtualConfig = Payvice().object(Payvice::VIRTUAL);
    if (!virtualConfig.isObject()) {
        return false;
    } else if (!virtualConfig(Payvice::TID).isString() || !virtualConfig(Payvice::MID).isString() || !virtualConfig(Payvice::NAME).isString()
        || !virtualConfig(Payvice::PKEY).isString() || !virtualConfig(Payvice::SKEY).isString() || !virtualConfig(Payvice::MCC).isString()
         || !virtualConfig(Payvice::CUR_CODE).isString() )
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

    if(itexIsMerchant()) return 0;

    urlPath += vasimpl::getDeviceTerminalId();

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

int itexIsMerchant()
{
    MerchantData mParam = {'\0'};
    readMerchantData(&mParam);

    if (strcmp(mParam.app_type, "MERCHANT") == 0 || strcmp(mParam.app_type, "CONVERTED") == 0) {
        return 0;
    }

    return 1;
}

int switchMerchantToVas(void* merchant)
{
    Payvice payvice;

    if (!virtualConfigurationIsSet()) {
        resetVirtualConfiguration();
        return -1;
    }

    Eft *trxContext = (Eft*)merchant;

    strncpy(trxContext->merchantType, payvice.object(Payvice::VIRTUAL)(Payvice::MCC).getString().c_str(), sizeof(trxContext->merchantType) - 1);
    strncpy(trxContext->merchantId, payvice.object(Payvice::VIRTUAL)(Payvice::MID).getString().c_str(), sizeof(trxContext->merchantId) - 1);
    strncpy(trxContext->merchantName, payvice.object(Payvice::VIRTUAL)(Payvice::NAME).getString().c_str(), sizeof(trxContext->merchantName) - 1);
    strncpy(trxContext->currencyCode, payvice.object(Payvice::VIRTUAL)(Payvice::CUR_CODE).getString().c_str(), sizeof(trxContext->currencyCode) - 1);
    strncpy(trxContext->terminalId, payvice.object(Payvice::VIRTUAL)(Payvice::TID).getString().c_str(), sizeof(trxContext->terminalId) - 1);
    strncpy(trxContext->sessionKey, payvice.object(Payvice::VIRTUAL)(Payvice::SKEY).getString().c_str(), sizeof(trxContext->sessionKey) - 1);

    return 0;
}

// int getUpWithrawalField53(char (&vtSecurity)[97], const Payvice& payvice);
short getUpWithrawalField53(char vtSecurity[97])
{

    char vPinKeyBcd[16];
    char vSessionKeyBcd[16];
    char mwDESKeyBcd[16];

    char encPinKeyBcd[16];
    char encSessionKeyBcd[16];

    memset(vPinKeyBcd, 0, sizeof(vPinKeyBcd));
    memset(vSessionKeyBcd, 0, sizeof(vSessionKeyBcd));
    memset(mwDESKeyBcd, 0, sizeof(mwDESKeyBcd));

    Payvice payvice;
    if (!loggedIn(payvice)) {
        return -1;
    }

    const iisys::JSObject& virtualConfig = payvice.object(Payvice::VIRTUAL);
    const std::string sessionKey = virtualConfig(Payvice::SKEY).getString();
    const std::string pinKey = virtualConfig(Payvice::PKEY).getString();
    const std::string mwKey = virtualConfig("mw3DESkey").getString();

    Util_Asc2Bcd((char*)pinKey.c_str(), vPinKeyBcd, pinKey.length());
    Util_Asc2Bcd((char*)sessionKey.c_str(), vSessionKeyBcd, sessionKey.length());
    Util_Asc2Bcd((char*)mwKey.c_str(), mwDESKeyBcd, mwKey.length());

    if (Util_Des(2, mwDESKeyBcd, vPinKeyBcd, encPinKeyBcd) ) {
        return -1;
    } else if (Util_Des(2, mwDESKeyBcd, vPinKeyBcd+8, encPinKeyBcd+8)) {
        return -1;
    }

    if (Util_Des(2, mwDESKeyBcd, vSessionKeyBcd, encSessionKeyBcd) ) {
        return -1;
    } else if (Util_Des(2, mwDESKeyBcd, vSessionKeyBcd+8, encSessionKeyBcd+8)) {
        return -1;
    }

    char encPinKeyHex[32 + 1] = { 0 };
    char encSessionKeyHex[32 + 1] = { 0 };

    Util_Bcd2Asc(encPinKeyBcd, encPinKeyHex, sizeof(encPinKeyHex) -1);
    Util_Bcd2Asc(encSessionKeyBcd, encSessionKeyHex, sizeof(encSessionKeyHex) -1);

    sprintf(vtSecurity, "%.32s0000%.32s", encPinKeyHex, encSessionKeyHex);
    pad(vtSecurity, 'F', 97 - 1, 1); 

    return 0;
}
