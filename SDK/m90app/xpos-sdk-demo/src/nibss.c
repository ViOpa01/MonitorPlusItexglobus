/**
 * File: nibss.c 
 * -------------
 * Implements nibss.h interace.
 * @author Opeyemi Ajani.
 */

//std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//MoreFun
#include "libapi_xpos/inc/libapi_system.h"
#include "sdk_xgui.h"
#include "libapi_xpos/inc/libapi_security.h"
#include "libapi_xpos/inc/libapi_file.h"

//Itex
#include "nibss.h"

#define PTAD_KEY "F9F6FF09D77B6A78595541DB63D821FA"
#define MERCHANT_FILE "MerchantParams.dat"

static void getDateTime(char *yyyymmddhhmmss)
{
    Sys_GetDateTime(yyyymmddhhmmss);
    yyyymmddhhmmss[14] = '\0';
}

static void addGenericNetworkFields(NetworkManagement *networkMangement)
{
    getDateTime(networkMangement->yyyymmddhhmmss);
    strncpy(networkMangement->stan, &networkMangement->yyyymmddhhmmss[8], sizeof(networkMangement->stan));
}

static void addCallHomeData(NetworkManagement *networkMangement)
{
    //TODO: get the values at runtime, the hardcoded data will still work
    strncpy(networkMangement->appVersion, "1.0.6", sizeof(networkMangement->appVersion));
    strncpy(networkMangement->deviceModel, "H9", sizeof(networkMangement->deviceModel));
    strncpy(networkMangement->callHOmeData, "{\"bl\":100,\"btemp\":35,\"cloc\":{\"cid\":\"00C9778E\",\"lac\":\"7D0B\",\"mcc\":\"621\",\"mnc\":\"60\",\"ss\":\"-87dbm\"},\"coms\":\"GSM/UMTSDualMode\",\"cs\":\"NotCharging\",\"ctime\":\"2019-12-20 12:06:14\",\"hb\":\"true\",\"imsi\":\"621600087808190\",\"lTxnAt\":\"\",\"mid\":\"FBP205600444741\",\"pads\":\"\",\"ps\":\"PrinterAvailable\",\"ptad\":\"Itex Integrated Services\",\"serial\":\"346-231-236\",\"sim\":\"9mobile\",\"simID\":\"89234000089199032105\",\"ss\":\"33\",\"sv\":\"TAMSLITE v(1.0.6)Built for POSVAS onFri Dec 20 10:50:14 2019\",\"tid\":\"2070HE88\",\"tmanu\":\"Verifone\",\"tmn\":\"V240m 3GPlus\"}", sizeof(networkMangement->callHOmeData));
    strncpy(networkMangement->commsName, "MTN-NG", sizeof(networkMangement->commsName));
}

static int inSaveParameters(const MerchantParameters *merchantParameters, const char *filename)
{
    int ret = 0;
    int fp;

    const int recordSize = sizeof(MerchantParameters);

    ret = UFile_OpenCreate(filename, FILE_PRIVATE, FILE_CREAT, &fp, recordSize); //File open / create

    if (ret != UFILE_SUCCESS)
    {
        gui_messagebox_show("FileTest", "File open or create fail", "", "confirm", 0);
        return ret;
    }

    UFile_Write(fp, (char *)merchantParameters, recordSize);
    UFile_Close(fp);

    return UFILE_SUCCESS;
}

static int inGetParameters(MerchantParameters *merchantParameters, const char *filename)
{
    int ret = 0;
    int fp;

    const int recordSize = sizeof(MerchantParameters);

    ret = UFile_OpenCreate(filename, FILE_PRIVATE, FILE_RDONLY, &fp, recordSize); //File open / create

    if (ret != UFILE_SUCCESS)
    {
        gui_messagebox_show("FileTest", "File open or create fail", "", "confirm", 0);
        return ret;
    }

    UFile_Lseek(fp, 0, 0); // seek 0
    memset(merchantParameters, 0x00, recordSize);
    UFile_Read(fp, (char *)merchantParameters, recordSize);
    UFile_Close(fp);

    return UFILE_SUCCESS;
}

int getParameters(MerchantParameters *merchantParameters)
{
    int result = -1;

    result = inGetParameters(merchantParameters, MERCHANT_FILE);

    return result;
}

int saveParameters(const MerchantParameters *merchantParameters)
{
    int result = -1;

    result = inSaveParameters(merchantParameters, MERCHANT_FILE);

    return result;
}

static int getTmk(NetworkManagement *networkMangement)
{
    int result = -1;
    unsigned char packet[256];
    unsigned char response[512];

    networkMangement->type = MASTER_KEY;

    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    if (result <= 0)
        return -1;

    //TODO: Send packet to host,  get response, return negative number on error.

    result = extractNetworkManagmentResponse(networkMangement, response, result);

    return result;
}

static int getTsk(NetworkManagement *networkMangement)
{
    int result = -1;
    unsigned char packet[256];
    unsigned char response[512];

    networkMangement->type = SESSION_KEY;

    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    if (result <= 0)
        return -1;

    //TODO: Send packet to host,  get response, return negative number on error.

    result = extractNetworkManagmentResponse(networkMangement, response, result);

    return result;
}

static int getTpk(NetworkManagement *networkMangement)
{
    int result = -1;
    unsigned char packet[256];
    unsigned char response[512];

    networkMangement->type = PIN_KEY;

    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    if (result <= 0)
        return -1;

    //TODO: Send packet to host,  get response, return negative number on error.

    result = extractNetworkManagmentResponse(networkMangement, response, result);

    return result;
}

static int getParams(NetworkManagement *networkMangement)
{
    int result = -1;
    unsigned char packet[256];
    unsigned char response[512];

    networkMangement->type = PARAMETERS_DOWNLOAD;

    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    if (result <= 0)
        return -1;

    //TODO: Send packet to host,  get response, return negative number on error.

    result = extractNetworkManagmentResponse(networkMangement, response, result);

    if (!result && Sys_SetDateTime(networkMangement->merchantParameters.ctmsDateAndTime))
    {
        printf("Error syncing device with Nibss time");
    }

    return result;
}

static int sCallHome(NetworkManagement *networkMangement)
{
    int result = -1;
    unsigned char packet[256];
    unsigned char response[512];

    networkMangement->type = CALL_HOME;
    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    if (result <= 0)
        return -1;

    //TODO: Send packet to host,  get response, return negative number on error.

    result = extractNetworkManagmentResponse(networkMangement, response, result);

    return result;
}

static short injectKeys(const NetworkManagement *networkMangement, const int gid)
{
    char kvc[8]; //kvc is Key plaintext encryption eight 0x00

    memset(kvc, 0x00, sizeof(kvc));
    mksk_save_plaintext_key(MKSK_MAINKEY_TYPE, gid, networkMangement->masterKey.clearKeyBcd, kvc);

    if (strcmp(kvc, networkMangement->masterKey.checkValue))
    { //This will never happen.
        //TODO: Display error on Pos screen and wait for 8 seconds.
        printf("Master key check value failed.");
        return -1;
    }

    memset(kvc, 0x00, sizeof(kvc));
    mksk_save_encrypted_key(MKSK_PINENC_TYPE, gid, networkMangement->pinKey.encryptedKeyBcd, kvc);

    if (strcmp(kvc, networkMangement->pinKey.checkValue))
    { //This will never happen.
        //TODO: Display error on Pos screen and wait for 8 seconds.
        printf("Pinkey key check value failed.");
        return -2;
    }

    memset(kvc, 0x00, sizeof(kvc));
    mksk_save_encrypted_key(MKSK_MACENC_TYPE, gid, networkMangement->sessionKey.encryptedKeyBcd, kvc);

    if (strcmp(kvc, networkMangement->sessionKey.checkValue))
    { //This will never happen.
        //TODO: Display error on Pos screen and wait for 8 seconds.
        printf("Session key check value failed.");
        return -3;
    }

    return 0;
}

short handshake(void)
{
    NetworkManagement networkMangement;
    int maxRetry = 2;
    int i;

    memset(&networkMangement, 0x00, sizeof(NetworkManagement));
    strncpy(networkMangement.terminalId, "2070HE88", sizeof(networkMangement.terminalId));

    //Master key requires clear ptad key
    strncpy(networkMangement.clearPtadKey, PTAD_KEY, sizeof(networkMangement.clearPtadKey));

    for (i = 0; i < maxRetry; i++)
    {
        if (!getTmk(&networkMangement))
            break;
    }

    if (i == maxRetry)
        return -1;

    for (i = 0; i < maxRetry; i++)
    {
        if (!getTsk(&networkMangement))
            break;
    }

    if (i == maxRetry)
        return -2;

    for (i = 0; i < maxRetry; i++)
    {
        if (!getTpk(&networkMangement))
            break;
    }

    if (i == maxRetry)
        return -3;

    //TODO: Get device serial number at runtime
    strncpy(networkMangement.serialNumber, "346-231-236", sizeof(networkMangement.serialNumber));

    for (i = 0; i < maxRetry; i++)
    {
        if (!getParams(&networkMangement))
            break;
    }

    if (i == maxRetry)
        return -4;

    if (injectKeys(&networkMangement, 0))
    {
        return -5;
    }

    addCallHomeData(&networkMangement);

    for (i = 0; i < maxRetry; i++)
    {
        if (!sCallHome(&networkMangement))
            break;
    }

    saveParameters(&networkMangement.merchantParameters);

    return 0;
}

