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
#include "network.h"
#include "merchant.h"
#include "itexFile.h"
#include "util.h"




#define PTAD_KEY "F9F6FF09D77B6A78595541DB63D821FA" //POSVAS LIVE
#define PTAD_KEY "DBCC87EE50A6810682FAD28B1190F578" //EPMS LIVE KEYS
//#define PTAD_KEY "DBEECACCB4210977ACE73A1D873CA59F" //TEST KEY

#define MERCHANT_FILE "MerchantParams.dat"
#define SESSION_KEY_FILE "SessionKey.dat"

// 197.253.19.75 live
// 197.253.19.78 tests
// Port : EPMS -> 5001(ssl), 5000 (plain)
// Port : POSVAS -> 5003(ssl), 5004(plain)

// defined here this temporarily 
#define NIBSS_IS_SSL 1
#define NIBSS_HOST "197.253.19.75"
#define NIBSS_PORT  5003


static void initNibssParameters(NetWorkParameters * netParam, const MerchantData * mParam)
{
	strncpy(netParam->host, mParam->nibss_ip, strlen(mParam->nibss_ip));
	netParam->port = mParam->nibss_port;
	netParam->isSsl = 1;
	netParam->isHttp = 0;
	netParam->receiveTimeout = 1000;
	strncpy(netParam->title, "Nibss", 10);
	strncpy(netParam->apn, "CMNET", 10);
	netParam->netLinkTimeout = 30000;

}

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
/*
static int inSaveParameters(const void *parameters, const char *filename, const int recordSize)
{
    int ret = 0;
    int fp;

    ret = UFile_OpenCreate(filename, FILE_PRIVATE, FILE_CREAT, &fp, recordSize); //File open / create

    if (ret != UFILE_SUCCESS)
    {
        gui_messagebox_show("FileTest", "File open or create fail", "", "confirm", 0);
        return ret;
    }

    UFile_Write(fp, (char *)parameters, recordSize);
    UFile_Close(fp);

    return UFILE_SUCCESS;
}

static int inGetParameters(void *parameters, const char *filename, const int recordSize)
{
    int ret = 0;
    int fp;

    ret = UFile_OpenCreate(filename, FILE_PRIVATE, FILE_RDONLY, &fp, recordSize); //File open / create

    if (ret != UFILE_SUCCESS)
    {
        gui_messagebox_show("FileTest", "File open or create fail", "", "confirm", 0);
        return ret;
    }

    UFile_Lseek(fp, 0, 0); // seek 0
    memset((char *) parameters, 0x00, recordSize);
    UFile_Read(fp, (char *)parameters, recordSize);
    UFile_Close(fp);

    return UFILE_SUCCESS;
}

*/
int getSessionKey(char sessionKey[33])
{
    int result = -1;
    NetworkKey sessionKeyStruct;

    result = getRecord(&sessionKeyStruct, SESSION_KEY_FILE, sizeof(NetworkKey), sizeof(NetworkKey));

    if (result) {
        fprintf(stdout, "Error getting session key\n\n");
        return -1;
    }

    strncpy(sessionKey, sessionKeyStruct.clearKey, 32);

    sessionKey[32] = 0;

    return result;
}

int getParameters(MerchantParameters *merchantParameters)
{
    int result = -1;

    result = getRecord(merchantParameters, MERCHANT_FILE, sizeof(MerchantParameters), sizeof(MerchantParameters));

    return result;
}

int saveParameters(const MerchantParameters *merchantParameters)
{
    int result = -1;

    result = saveRecord(merchantParameters, MERCHANT_FILE, sizeof(MerchantParameters), sizeof(MerchantParameters));

    return result;
}

static short handleDe39(char * responseCode, char * responseDesc)
{
    if (strncmp(responseCode, "00", 2)) {
        gui_messagebox_show(responseCode , responseDesc, "" , "" , 0);   
      return -1;
    }

    return 0;
}

static int getTmk(NetworkManagement *networkMangement, NetWorkParameters *netParam)
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

    netParam->packetSize = result;
    memcpy(netParam->packet, packet, result);

    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -2;
    }

    printf("Master key response: \n%s\n", &netParam->response[2]);

    result = extractNetworkManagmentResponse(networkMangement, netParam->response/*response*/, netParam->responseSize);

    printf("********\n");   // Log to know where issue is

    if (handleDe39(networkMangement->responseCode, networkMangement->responseDesc)) {
        return -3;
    }


    return result;
}

static int saveSessionKey(const NetworkKey * sessionKey)
{
     int result = -1;

    result = saveRecord(sessionKey, SESSION_KEY_FILE, sizeof(NetworkKey), sizeof(NetworkKey));

    return result;
}

static int getTsk(NetworkManagement *networkMangement, NetWorkParameters *netParam)
{
    int result = -1;
    unsigned char packet[256];
    unsigned char response[512];    // Will later remove it

    networkMangement->type = SESSION_KEY;

    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    if (result <= 0)
        return -1;

    //TODO: Send packet to host,  get response, return negative number on error.

    netParam->packetSize = result;
    memcpy(netParam->packet, packet, result);
    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -2;
    }

    printf("Session key response: \n%s\n", &netParam->response[2]);

    result = extractNetworkManagmentResponse(networkMangement, netParam->response/*response*/, netParam->responseSize);

    if (result) {
        return -2;
    }

    if (handleDe39(networkMangement->responseCode, networkMangement->responseDesc)) {
        return -3;
    }

    saveSessionKey(&networkMangement->sessionKey);
    

    return result;
}

static int getTpk(NetworkManagement *networkMangement, NetWorkParameters *netParam)
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

    netParam->packetSize = result;
    memcpy(netParam->packet, packet, result);
    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -2;
    }

    printf("Pin key response: \n%s\n", &netParam->response[2]);

    result = extractNetworkManagmentResponse(networkMangement, netParam->response, netParam->responseSize);

    if (result) {
        return -2;
    }

    if (handleDe39(networkMangement->responseCode, networkMangement->responseDesc)) {
        return -3;
    }
    
    return result;
}

static int getParams(NetworkManagement *networkMangement, NetWorkParameters *netParam)
{
    int result = -1;
    unsigned char packet[256];
    unsigned char response[512];

    networkMangement->type = PARAMETERS_DOWNLOAD;

    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    printf("Clear Session Key -> %s\n", networkMangement->sessionKey.clearKey);

    if (result <= 0)
        return -1;

    //TODO: Send packet to host,  get response, return negative number on error.

    netParam->packetSize = result;
    memcpy(netParam->packet, packet, result);

    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -2;
    }

    printf("Parameter key response: \n%s\n", &netParam->response[2]);

    result = extractNetworkManagmentResponse(networkMangement, netParam->response, netParam->responseSize);

    if (result) {
        return -2;
    }

    if (handleDe39(networkMangement->responseCode, networkMangement->responseDesc)) {
        return -3;
    }

    if (Sys_SetDateTime(networkMangement->merchantParameters.ctmsDateAndTime))
    {
        printf("Error syncing device with Nibss time");
        return -3;
    }

    return result;
}

static int sCallHome(NetworkManagement *networkMangement, NetWorkParameters *netParam)
{
    int result = -1;
    unsigned char packet[1024];
    unsigned char response[512];

    networkMangement->type = CALL_HOME;
    addGenericNetworkFields(networkMangement);
    result = createIsoNetworkPacket(packet, sizeof(packet), networkMangement);

    if (result <= 0) {
        fprintf(stderr, "Callhome -> Error creating packet....\n");
        return -1;
    }

    //TODO: Send packet to host,  get response, return negative number on error.

    netParam->packetSize = result;
    memcpy(netParam->packet, packet, result);
    
    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
         fprintf(stderr, "Callhome -> Error send and receive failed....\n");
        return -2;
    }


    result = extractNetworkManagmentResponse(networkMangement, netParam->response/*response*/, netParam->responseSize);

    if (result) {
        fprintf(stderr, "Callhome -> Error extracting response....\n");
        return -2;
    }

    if (handleDe39(networkMangement->responseCode, networkMangement->responseDesc)) {
        return -3;
    }

    return 0;
}

static short injectKeys(const NetworkManagement *networkMangement, const int gid)
{
    char kvc[8]; //kvc is Key plaintext encryption eight 0x00

    memset(kvc, 0x00, sizeof(kvc));
    mksk_save_plaintext_key(MKSK_MAINKEY_TYPE, gid, networkMangement->masterKey.clearKeyBcd, kvc);


    if (memcmp(kvc, networkMangement->masterKey.checkValueBcd, networkMangement->masterKey.checkValueBcdLen))
    { //This will never happen.
        //TODO: Display error on Pos screen and wait for 8 seconds.
        char buffer[256];

        sprintf(buffer, "Master key check value failed: expected -> %s, actual -> %s", networkMangement->masterKey.checkValue, kvc);
        gui_messagebox_show("ERROR" , buffer, "" , "" , 30000);
        printf("Master key check value failed.");
        return -1;
    }
    

    memset(kvc, 0x00, sizeof(kvc));
    mksk_save_encrypted_key(MKSK_PINENC_TYPE, gid, networkMangement->pinKey.encryptedKeyBcd, kvc);

    
    if (memcmp(kvc, networkMangement->pinKey.checkValueBcd, networkMangement->pinKey.checkValueBcdLen))
    { //This will never happen.
        //TODO: Display error on Pos screen and wait for 8 seconds.
        gui_messagebox_show("ERROR" , "Pin key check value failed.", "" , "" , 8000);
        printf("Pinkey key check value failed.");
        return -2;
    }
    

    memset(kvc, 0x00, sizeof(kvc));
    mksk_save_encrypted_key(MKSK_MACENC_TYPE, gid, networkMangement->sessionKey.encryptedKeyBcd, kvc);

    
    if (memcmp(kvc, networkMangement->sessionKey.checkValueBcd, networkMangement->sessionKey.checkValueBcdLen))
    { //This will never happen.
        //TODO: Display error on Pos screen and wait for 8 seconds.
        gui_messagebox_show("ERROR" , "Session key check value failed.", "" , "" , 8000);
        printf("Session key check value failed.");
        return -3;
    }
    
    return 0;
}



short uiGetParameters(void)
{
    NetworkManagement networkMangement;
    NetWorkParameters netParam = {0};
    MerchantData mParam = {0};
    char terminalSerialNumber[22] = {'\0'};
    char tid[9] = {'\0'};
    int maxRetry = 2;
    int i;
    int ret;
    char sessionKey[33] = {'\0'};

    if(readMerchantData(&mParam)) return -2;

     if (!mParam.is_prepped) { //terminal not preped, parameter not allowed
        gui_messagebox_show("ERROR" , "Please Prep first", "" , "" , 0);
        return -1;
    }

    if(mParam.tid[0])
    {
        strncpy(tid, mParam.tid, strlen(mParam.tid));
    }

    memset(&networkMangement, 0x00, sizeof(NetworkManagement));
    strncpy(networkMangement.terminalId, tid, sizeof(networkMangement.terminalId));

    getNetParams(&netParam, CURRENT_PATFORM, 0);

    //TODO: Get device serial number at runtime
    getTerminalSn(terminalSerialNumber);
    strncpy(networkMangement.serialNumber, terminalSerialNumber/*"346-231-236"*/, sizeof(networkMangement.serialNumber));
    getSessionKey(sessionKey);

    memcpy(networkMangement.sessionKey.clearKey, sessionKey, strlen(sessionKey));

    for (i = 0; i < maxRetry; i++)
    {
        if (!getParams(&networkMangement, &netParam))
            break;
    }

    if (i == maxRetry) {
        gui_messagebox_show("FAILED" , "Parameters Failed!", "" , "" , 3000);
        return -2;
    }

    saveMerchantData(&mParam);
    saveParameters(&networkMangement.merchantParameters);

    Util_Beep(1);

    return 0;
}

short uiCallHome(void)
{
    NetworkManagement networkMangement;
    NetWorkParameters netParam = {0};
    MerchantData mParam = {0};
    char terminalSerialNumber[22] = {'\0'};
    char tid[9] = {'\0'};
    int maxRetry = 2;
    int i;
    int ret;
    char sessionKey[33] = {'\0'};

    if(readMerchantData(&mParam)) {
        fprintf(stderr, "Error reading parameters\n");
        return -2;
    }

    if (!mParam.is_prepped) { //terminal not preped, parameter not allowed
        gui_messagebox_show("ERROR" , "Please Prep first", "" , "" , 0);
        return -1;
    }

    if(mParam.tid[0])
    {
        strncpy(tid, mParam.tid, strlen(mParam.tid));
    }

    memset(&networkMangement, 0x00, sizeof(NetworkManagement));
    strncpy(networkMangement.terminalId, tid, sizeof(networkMangement.terminalId));

    getSessionKey(sessionKey);

    memcpy(networkMangement.sessionKey.clearKey, sessionKey, strlen(sessionKey));

    getNetParams(&netParam, CURRENT_PATFORM, 0);
    
    addCallHomeData(&networkMangement);

   
    for (i = 0; i < maxRetry; i++)
    {
        if (!sCallHome(&networkMangement, &netParam))
            break;
    }

    if (i == maxRetry) {
         gui_messagebox_show("FAILED" , "Call Home failed", "" , "" , 2000);
         return -2;
    }

     Util_Beep(2);
    return 0;
}



short uiHandshake(void)
{
    NetworkManagement networkMangement;
    NetWorkParameters netParam = {0};
    MerchantData mParam = {0};
    char terminalSerialNumber[22] = {'\0'};
    char tid[9] = {'\0'};
    int maxRetry = 2;
    int i;
    int ret;

    if(getMerchantData())
    {
        gui_messagebox_show("MERCHANT" , "Incomplete merchant data", "" , "" , 1);
        return -1; 
    } 

    if(readMerchantData(&mParam)) return -2;

    if(mParam.tid[0])
    {
        strncpy(tid, mParam.tid, strlen(mParam.tid));
    }

    memset(&networkMangement, 0x00, sizeof(NetworkManagement));
    strncpy(networkMangement.terminalId, tid, sizeof(networkMangement.terminalId));

    //Master key requires clear ptad key
    strncpy(networkMangement.clearPtadKey, PTAD_KEY, sizeof(networkMangement.clearPtadKey));

    getNetParams(&netParam, CURRENT_PATFORM, 0);
    
    
    for (i = 0; i < maxRetry; i++)
    {
        if (!getTmk(&networkMangement, &netParam))
            break;
    }

    gui_messagebox_show("MESSAGE" , "Master Key Ok!", "" , "" , 1000);

    if (i == maxRetry)
        return -1;

    //printf("Clear Tmk -> '%s'\n", networkMangement.masterKey.clearKey);
    //Sys_Delay(5000);

    for (i = 0; i < maxRetry; i++)
    {
        if (!getTsk(&networkMangement, &netParam))
            break;
    }

    if (i == maxRetry)
        return -2;

    gui_messagebox_show("MESSAGE" , "Session Key Ok!", "" , "" , 1000);

    
    for (i = 0; i < maxRetry; i++)
    {
        if (!getTpk(&networkMangement, &netParam))
            break;
    }

    if (i == maxRetry)
        return -3;
    
    gui_messagebox_show("MESSAGE" , "PIN KEY OK", "" , "" , 1000);

    //TODO: Get device serial number at runtime
    getTerminalSn(terminalSerialNumber);
    strncpy(networkMangement.serialNumber, terminalSerialNumber/*"346-231-236"*/, sizeof(networkMangement.serialNumber));

   
    for (i = 0; i < maxRetry; i++)
    {
        if (!getParams(&networkMangement, &netParam))
            break;
    }

    if (i == maxRetry)
        return -4;

     gui_messagebox_show("MESSAGE" , "Parameters Ok!", "" , "" , 1000);

    if (injectKeys(&networkMangement, 0))
    {
        return -5;
    }

    gui_messagebox_show("MESSAGE" , "Inject Keys Ok!", "" , "" , 1000);

    addCallHomeData(&networkMangement);

   
    for (i = 0; i < maxRetry; i++)
    {
        if (!sCallHome(&networkMangement, &netParam))
            break;
    }

    if (i < maxRetry) {
         gui_messagebox_show("MESSAGE" , "Call Home Ok", "" , "" , 1000);
    }


    mParam.is_prepped = 1;
    saveMerchantData(&mParam);
    saveParameters(&networkMangement.merchantParameters);

     Util_Beep(2);
    return 0;
}





