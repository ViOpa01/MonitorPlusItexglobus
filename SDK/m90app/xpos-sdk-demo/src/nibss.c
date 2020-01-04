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

//internal
#include "libapi_xpos/inc/libapi_system.h"
#include "sdk_xgui.h"

//Itex
#include "nibss.h"

//shared
#include "Nibss8583.h"

#define PTAD_KEY "F9F6FF09D77B6A78595541DB63D821FA"


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

    if (!result && Sys_SetDateTime(networkMangement->merchantParameters.ctmsDateAndTime)) {
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

    addCallHomeData(&networkMangement);

    for (i = 0; i < maxRetry; i++)
    {
        if (!sCallHome(&networkMangement))
            break;
    }
}
