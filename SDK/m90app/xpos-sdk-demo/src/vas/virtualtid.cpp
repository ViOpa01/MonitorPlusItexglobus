#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "payvice.h"
#include "jsobject.h"


#include "virtualtid.h"

int copyVirtualPinBlock(Eft *pTrxContext, const char* pinBlock)
{
    unsigned char pinKeyBcd[16];
    unsigned char vPinKeyBcd[16];

    unsigned char actualPinBlockBcd[8] = {0};
    unsigned char plainPinBlockBcd[8] = {0};

    Payvice payvice;
    if (!loggedIn(payvice)) {
        return -1;
    }
    
    // memcpy(actualPinBlockBcd, pinBlock->pPINBlock, pinBlock->ucPINBlockLen);

    // std::string pinKey = Merchant().object(config::PKEY).getString();
    // std::string vPinKey = payvice.object(Payvice::VIRTUAL)(Payvice::PKEY).getString();
/*
    ASCII2BCD(pinKey.c_str(), pinKeyBcd, sizeof(pinKeyBcd));
    ASCII2BCD(vPinKey.c_str(), vPinKeyBcd, sizeof(vPinKeyBcd));

    if (DES(TDES2KD, pinKeyBcd, actualPinBlockBcd, plainPinBlockBcd)) {
        return -1;
    } else if (DES(TDES2KE, vPinKeyBcd, plainPinBlockBcd, pTrxContext->pinblockBcd)) {
        return -1;
    }
*/
    // pTrxContext->pinblockLen = pinBlock->ucPINBlockLen;

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
    // CURL* curl_handle;
    // Payvice payvice;
    // CURLcode res;
    // char errorMsg[CURL_ERROR_SIZE];
    // std::string body;
    // std::string urlPath = "http://197.253.19.75/tams/eftpos/op/xmerchant.php?tid=";
    // LogManager log("TLITE");

    // payvice.object.erase(Payvice::VIRTUAL);
    // payvice.save();

    // curl_handle = curl_easy_init();

    // if (!curl_handle) {
    //     return -1;
    // }

    // CurlScopeGuard curlGuard(curl_handle);


    // urlPath += Merchant().object(config::TID).getString();

    // curl_easy_setopt(curl_handle, CURLOPT_URL, urlPath.c_str());

    // curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 30);
    // curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 60);
    // curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorMsg);

    // curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curlStringCallback);
    // curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&body);

    // curl_easy_setopt(curl_handle, CURLOPT_DEBUGFUNCTION, curlTrace);
    // /* the DEBUGFUNCTION has no effect until we enable VERBOSE */
    // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

    // res = curl_easy_perform(curl_handle);

    // if (res != CURLE_OK) {
    //     printf("Error -> %s", errorMsg);
    //     return -1;
    // }

    // iisys::JSObject detail;
    
    // if (!detail.load(body)) {
    //     printf("Unable to parse body -> %s", body.c_str());
    //     return -1;
    // }

    // payvice.object(Payvice::VIRTUAL) = detail;
    // payvice.save();

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

// int swithMerchantToVas(Merchant& merchant)
// {
//     Payvice payvice;

//     if (!virtualConfigurationIsSet()) {
//         resetVirtualConfiguration();
//         return -1;
//     }

//     merchant.object(config::TID) = payvice.object(Payvice::VIRTUAL)(Payvice::TID);
//     merchant.object(config::MCC) = payvice.object(Payvice::VIRTUAL)(Payvice::MCC);
//     merchant.object(config::MID) = payvice.object(Payvice::VIRTUAL)(Payvice::MID);
//     merchant.object(config::NAME) = payvice.object(Payvice::VIRTUAL)(Payvice::NAME);
//     merchant.object(config::CUR_CODE) = payvice.object(Payvice::VIRTUAL)(Payvice::CUR_CODE);
//     merchant.object(config::SKEY) = payvice.object(Payvice::VIRTUAL)(Payvice::SKEY);

//     return 0;
// }
