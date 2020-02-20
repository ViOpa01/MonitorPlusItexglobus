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
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_print.h"

#include "cJSON.h"

//Itex
#include "nibss.h"
#include "network.h"
#include "merchant.h"
#include "Receipt.h"
#include "itexFile.h"
#include "util.h"
#include "network.h"
#include "appInfo.h"
#include "remoteLogo.h"



#define MERCHANT_FILE "MerchantParams.dat"
#define SESSION_KEY_FILE "SessionKey.dat"

// 197.253.19.75 live
// 197.253.19.78 tests
// Port : EPMS -> 5001(ssl), 5000 (plain)
// Port : POSVAS -> 5003(ssl), 5004(plain)

// defined here this temporarily 
#define NIBSS_IS_SSL 1Skye
#define NIBSS_HOST "197.253.19.75"
#define NIBSS_PORT  5003

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
    MerchantParameters parameter = {'\0'};
    MerchantData mParam = {'\0'};
    Network netProfile = {'\0'};
    char *callHomeJsonStr;
    cJSON *callHomeJson;
    cJSON *cloc;

    char softwareVersion[64] = {'\0'};
    char mid[20] = {'\0'};
    char tid[10] = {'\0'};
    char terminalSn[24] = {'\0'};
    char dateAndTime[24] = {'\0'};
    char imsi[20] = {'\0'};
    char cellId[12] = {'\0'};
    char simId[24] = {'\0'};
    int signalLevel = 0;
    char lac[12] = {'\0'};
    char ss[4] = {'\0'};


    callHomeJson = cJSON_CreateObject();
    cloc = cJSON_CreateObject();

    if (!callHomeJson || !cloc) {
        gui_messagebox_show("ERROR", "Can't create json object", "", "", 0);
        return;
    }

    getParameters(&parameter);
    readMerchantData(&mParam);
    getTerminalSn(terminalSn);

    strncpy(mid, parameter.cardAcceptiorIdentificationCode, sizeof(mid));
    strncpy(tid, mParam.tid, 8);
    getFormattedDateTime(dateAndTime, sizeof(dateAndTime));
    sprintf(softwareVersion, "TAMSLITE %s Built for %s", APP_VER, mParam.platform_label);   // "TAMSLITE v(1.0.6)Built for POSVAS onFri Dec 20 10:50:14 2019"
    sprintf(cellId, "%d", getCellId());
    sprintf(lac, "%d", getLocationAreaCode());
    sprintf(simId, "%s", getSimId());
    getImsi(imsi);
    imsiToNetProfile(&netProfile, imsi);
    
    signalLevel = getSignalLevel();
    signalLevel *=  25;
    sprintf(ss, "%d", signalLevel);

    printf("IMSI : %s\n", imsi);
    printf("Cell id : %s\n Signal level : %d\n lac : %s\n", cellId, signalLevel, lac);
    printf("SIM Operator : %s\n", netProfile.operatorName);

    cJSON_AddItemToObject(cloc, "cid", cJSON_CreateString(cellId));
    cJSON_AddItemToObject(cloc, "lac", cJSON_CreateString(lac));
    cJSON_AddItemToObject(cloc, "mcc", cJSON_CreateString("621"));
    cJSON_AddItemToObject(cloc, "mnc", cJSON_CreateString("60"));
    cJSON_AddItemToObject(cloc, "ss", cJSON_CreateString("-87dbm"));

    cJSON_AddItemToObject(callHomeJson, "bl", cJSON_CreateNumber(100));
    cJSON_AddItemToObject(callHomeJson, "btemp", cJSON_CreateNumber(35));
    cJSON_AddItemToObject(callHomeJson, "cloc", cloc);
    cJSON_AddItemToObject(callHomeJson, "coms", cJSON_CreateString("GSM/UMTSDualMode"));
    cJSON_AddItemToObject(callHomeJson, "cs", cJSON_CreateString("NotCharging"));
    cJSON_AddItemToObject(callHomeJson, "ctime", cJSON_CreateString(dateAndTime));
    cJSON_AddItemToObject(callHomeJson, "hb", cJSON_CreateString("true"));
    cJSON_AddItemToObject(callHomeJson, "imsi", cJSON_CreateString(imsi));
    cJSON_AddItemToObject(callHomeJson, "lTxnAt", cJSON_CreateString(""));
    cJSON_AddItemToObject(callHomeJson, "mid", cJSON_CreateString(parameter.cardAcceptiorIdentificationCode));
    cJSON_AddItemToObject(callHomeJson, "pads", cJSON_CreateString(""));
    cJSON_AddItemToObject(callHomeJson, "ps", cJSON_CreateString("PrinterAvailable"));
    cJSON_AddItemToObject(callHomeJson, "ptad", cJSON_CreateString("Itex Integrated Services"));
    cJSON_AddItemToObject(callHomeJson, "serial", cJSON_CreateString(terminalSn));
    cJSON_AddItemToObject(callHomeJson, "sim", cJSON_CreateString(netProfile.operatorName)); 
    cJSON_AddItemToObject(callHomeJson, "simID", cJSON_CreateString(getSimId()));
    cJSON_AddItemToObject(callHomeJson, "ss", cJSON_CreateString(ss));
    cJSON_AddItemToObject(callHomeJson, "sv", cJSON_CreateString(softwareVersion));
    cJSON_AddItemToObject(callHomeJson, "tid", cJSON_CreateString(tid));
    cJSON_AddItemToObject(callHomeJson, "tmanu", cJSON_CreateString("Morefun"));
    cJSON_AddItemToObject(callHomeJson, "tmn", cJSON_CreateString(APP_MODEL));

    callHomeJsonStr = cJSON_PrintUnformatted(callHomeJson);
    memcpy(networkMangement->callHOmeData, callHomeJsonStr, sizeof(networkMangement->callHOmeData));
    
    strncpy(networkMangement->appVersion, APP_VER, sizeof(networkMangement->appVersion));
    strncpy(networkMangement->deviceModel, APP_MODEL, sizeof(networkMangement->deviceModel));
    strncpy(networkMangement->commsName, netProfile.operatorName, sizeof(networkMangement->commsName));

// strncpy(networkMangement->callHOmeData, "{\"bl\":100,\"btemp\":35,\"cloc\":{\"cid\":\"00C9778E\",\"lac\":\"7D0B\",\"mcc\":\"621\",\"mnc\":\"60\",\"ss\":\"-87dbm\"},\"coms\":\"GSM/UMTSDualMode\",\"cs\":\"NotCharging\",\"ctime\":\"2019-12-20 12:06:14\",\"hb\":\"true\",\"imsi\":\"621600087808190\",\"lTxnAt\":\"\",\"mid\":\"FBP205600444741\",\"pads\":\"\",\"ps\":\"PrinterAvailable\",\"ptad\":\"Itex Integrated Services\",\"serial\":\"346-231-236\",\"sim\":\"9mobile\",\"simID\":\"89234000089199032105\",\"ss\":\"33\",\"sv\":\"TAMSLITE v(1.0.6)Built for POSVAS onFri Dec 20 10:50:14 2019\",\"tid\":\"2070HE88\",\"tmanu\":\"Morefun\",\"tmn\":\"V240m 3GPlus\"}", sizeof(networkMangement->callHOmeData));

    free(callHomeJsonStr);
}

short isDevMode(const enum NetType netType)
{
    switch (netType) {
        case NET_EPMS_SSL_TEST: case NET_EPMS_PLAIN_TEST: 
        case NET_POSVAS_SSL_TEST: NET_POSVAS_PLAIN_TEST:
        case UPSL_DIRECT_TEST:
            return 1;

        default: 
            return 0;
    }
}

static const char * platformToKey(enum NetType netType)
{

    platformAutoSwitch(&netType);

    switch (netType) {
        case NET_POSVAS_SSL: case NET_POSVAS_PLAIN:
            return "F9F6FF09D77B6A78595541DB63D821FA";

        case NET_EPMS_PLAIN: case NET_EPMS_SSL:
            return "DBCC87EE50A6810682FAD28B1190F578";

        case NET_EPMS_SSL_TEST: case NET_EPMS_PLAIN_TEST: 
        case NET_POSVAS_SSL_TEST: NET_POSVAS_PLAIN_TEST:
            return "DBEECACCB4210977ACE73A1D873CA59F";

        case UPSL_DIRECT_TEST:
            return "0"; //TODO: add upsl key

        default:
            return NULL;

    }
}

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

short handleDe39(char * responseCode, char * responseDesc)
{
    if (!isApprovedResponse(responseCode)) {
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

    strcpy(netParam->title, "MASTER KEY");
    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -2;
    }

    printf("Master key response: \n%s\n", &netParam->response[2]);

    result = extractNetworkManagmentResponse(networkMangement, netParam->response/*response*/, netParam->responseSize);

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
    strcpy(netParam->title, "SESSION KEY");
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

    strcpy(netParam->title, "PIN KEY");
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

    strcpy(netParam->title, "PARAMETERS");
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
    
    strcpy(netParam->title, "CALLHOME");
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


static void showHexData(char*title,  char * data, int size)
{
	char msg[256]={0};
	int i;

	for(i = 0;i < size; i ++){
		sprintf(msg + strlen(msg), "%02X ", data[i]);
	}
	gui_messagebox_show(title, msg, "", "confirm" , 0);
}

static short injectKeys(const NetworkManagement *networkMangement, const int gid)
{
		// Test key in plaintext
	//char maink[TEST_GUI_KEY_SIZE] ={0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
	char pink[16] ={0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22};
	char mack[16] ={0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33};
	char magk[16] ={0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44};
	char ind[16] = {0x12,0x34};



	char keyciphertext[16];
	char outd[16];
	char kvc[8];		//kvc is Key plaintext encryption eight 0x00
	//int gid =0;		// Key index 0-9

	char pinblock[32]={0};
	char expectedPinblock[32];
	char ksn[32]={0};
	int ret;
	int pinlen = 6;
	int timeover = 30000;
	char *pan = "5178685092355984";


	// Save the terminal master key plaintext
	mksk_save_plaintext_key(MKSK_MAINKEY_TYPE, gid, networkMangement->masterKey.clearKeyBcd, kvc);	

	//Here you can check the kvc, it is main key plaintext encryption eight 0x00,
	//showHexData("MAIN KEY KVC", kvc , 4);

	// Simulate server encryption key
	Util_Des(2, networkMangement->masterKey.clearKeyBcd, (char *)networkMangement->pinKey.clearKeyBcd, (char *)keyciphertext);
	Util_Des(2, networkMangement->masterKey.clearKeyBcd, (char *)&networkMangement->pinKey.clearKeyBcd[8], (char *)keyciphertext+8);

    printf("Clear pin key -> '%s'\n", networkMangement->pinKey.clearKey);

	//Save the pin key ciphertext
	mksk_save_encrypted_key(MKSK_PINENC_TYPE, gid, keyciphertext, kvc);

	//Here you can check the kvc, it is pin key plaintext encryption eight 0x00,
	//showHexData("PIN KEY KVC", kvc , 4);


	// Same save magkey and mackey
	Util_Des(2, networkMangement->masterKey.clearKeyBcd, (char *)mack, (char *)keyciphertext);
	Util_Des(2, networkMangement->masterKey.clearKeyBcd, (char *)mack+8, (char *)keyciphertext+8);

	mksk_save_encrypted_key(MKSK_MACENC_TYPE, gid, keyciphertext, kvc);


	Util_Des(2, networkMangement->masterKey.clearKeyBcd, (char *)magk, (char *)keyciphertext);
	Util_Des(2, networkMangement->masterKey.clearKeyBcd, (char *)magk+8, (char *)keyciphertext+8);
	mksk_save_encrypted_key(MKSK_MAGDEC_TYPE, gid, keyciphertext, kvc);


    /*
    while (1) {
        //Pin Operations
        sec_set_pin_mode(1, 6);
        ret = _input_pin_page("input pin", 0, pinlen, timeover);
        if(ret == 0){
            sec_encrypt_pin_proc(SEC_MKSK_FIELD, SEC_PIN_FORMAT0, 0, pan, pinblock, 0);
        }
        sec_set_pin_mode(0, 0);
        
        showHexData("pinblock", pinblock, 16);
        showHexData("ksn", ksn, 10);
    }
    */
    
	
    return 0;
}

/*
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

    printf("Pin Key -> %s, kcv -> %s", networkMangement->pinKey.clearKey, networkMangement->pinKey.checkValue);

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
*/



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

    getNetParams(&netParam, CURRENT_PLATFORM, 0);

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
    // char terminalSerialNumber[22] = {'\0'};
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

    getNetParams(&netParam, CURRENT_PLATFORM, 0);
    
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

static short getTid(char tid[9])
{
	char msgPrompt[35] = {'\0'};
    const int tidSize = 8;
    int result = -1;

	if (*tid) {
		sprintf(msgPrompt, "Enter Tid(%s)", tid);
	} else {
		strcpy(msgPrompt, "Enter Tid");
	}
	gui_clear_dc();
	
	result = Util_InputText(GUI_LINE_TOP(0), msgPrompt, GUI_LINE_TOP(2), tid, tidSize, tidSize, 1, 1 ,18000);

    return result == tidSize ? 0 : -1;
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

    
    if (!isDevMode(CURRENT_PLATFORM)) {
         if(getMerchantData()) {
            gui_messagebox_show("MERCHANT" , "Incomplete merchant data", "" , "" , 1);
            return -1; 
        }
    } 
    else 
    {
        if (getTid(tid)) return -1;
    }

    if(readMerchantData(&mParam)) return -2;

    if (isDevMode(CURRENT_PLATFORM)) {
        strncpy(mParam.tid, tid, sizeof(mParam.tid));
    }

    if(mParam.tid[0])
    {
        strncpy(tid, mParam.tid, strlen(mParam.tid));
    }

    memset(&networkMangement, 0x00, sizeof(NetworkManagement));
    strncpy(networkMangement.terminalId, tid, sizeof(networkMangement.terminalId));

    //Master key requires clear ptad key
    strncpy(networkMangement.clearPtadKey, platformToKey(CURRENT_PLATFORM), sizeof(networkMangement.clearPtadKey));

    getNetParams(&netParam, CURRENT_PLATFORM, 0);
    
    
    for (i = 0; i < maxRetry; i++)
    {
        if (!getTmk(&networkMangement, &netParam))
            break;
    }

    if (i == maxRetry)
        return -1;

    gui_messagebox_show("MESSAGE" , "Master Key Ok!", "" , "" , 1000);

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
    strncpy(networkMangement.serialNumber, terminalSerialNumber, sizeof(networkMangement.serialNumber));

   
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

    strncpy(mParam.pKey, networkMangement.pinKey.clearKey, sizeof(mParam.pKey) - 1);
    saveMerchantData(&mParam);
    saveParameters(&networkMangement.merchantParameters);

    Util_Beep(2);

    for (i = 0; i < maxRetry; i++)
    {
        //TODO: make network lib tread save and put this on a thread.
        if (!downloadRemoteLogo(mParam.tid))
            break;
    }

    if (i < maxRetry) {
         gui_messagebox_show("MESSAGE" , "Logo Ok", "" , "" , 1000);
    }

    printHandshakeReceipt(&mParam);
;
    return 0;
}

short autoHandshake(void)
{
	MerchantData merchantData;

	memset(&merchantData, 0x00, sizeof(MerchantData));
	readMerchantData(&merchantData);

	if (merchantData.is_prepped) return 0;
	return uiHandshake();
}




