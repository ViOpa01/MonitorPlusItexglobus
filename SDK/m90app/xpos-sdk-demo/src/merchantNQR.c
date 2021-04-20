#include <stdio.h>
#include <string.h>

#include "appInfo.h"
#include "merchantNQR.h"
#include "cJSON.h"
#include "util.h"
#include "network.h"
#include "log.h"

#include "libapi_xpos/inc/libapi_gui.h"
#include "pages/inputamount_page.h"

#include "merchant.h"

#define NQR_IP "197.253.19.76"
#define NQR_PORT 1880

#define SHA256_NQR_KEY_TEST "N2Q0Yjg5YTg5MWI0OWZjMzRkNDM2MDQzMTQwZWJmNWQ1ZTkzN2ZhOGJkNTg3YTk5YWY4NjRkMWY3NTZmN2U0Nw==" //(testKey)
#define SHA256_NQR_KEY_LIVE "YTRjNjcxYzcwNDc2NTViOWRiMzRlZjcwNzc2NjFkOWYwOTMyMTZlMWFjMWNmZWY2ZTljNWQ2NTk2NGI0OTg1ZA==" //(LIVEkey)
#define NQR_SIGNATURE "aa281eca062cf493407d8a3c2d4411835a4a424d1a492de903e363bc4b3f6b59"

char * getCliRef(char* clientRef)
{
    const char* modelTag = "08";
    const char* serialTag = "09";
    const char* referenceTag = "11";
    const char* terminalIdTag = "12";

    char reference[14] = {'\0'};
    char deviceModel[32] = {'\0'};
    char tid[9] = {'\0'};
    char deviceSerial[16] = {'\0'};

    memset(clientRef, '\0', sizeof(clientRef));

    MerchantData mParam;    
    memset(&mParam, 0x00, sizeof(MerchantData));
    readMerchantData(&mParam);
    memset(tid, 0, sizeof(tid));
    strcpy(tid, mParam.tid);

    getTerminalSn(deviceSerial);
    getRrn(reference);
    sprintf(deviceModel, "%s%s", TERMINAL_MANUFACTURER, APP_MODEL);

    int i = 0;
    i += sprintf(&clientRef[i], "%s%02d%s", modelTag, strlen(deviceModel), deviceModel);
    i += sprintf(&clientRef[i], "%s%02d%s", serialTag, strlen(deviceSerial), deviceSerial);
    i += sprintf(&clientRef[i], "%s%02d%s", terminalIdTag, strlen(tid), tid);
    i += sprintf(&clientRef[i], "%s%02d%s", referenceTag, strlen(referenceTag), reference);

    return clientRef;
}


int getMerchantQRCode(NetWorkParameters *netParam, MerchantNQR *nqr)
{
    char request[1024] = {'\0'};
    char response[2048] = {'\0'};
    char itexKey[] = "uytdfsdfgghj";
    // char encryptedItexKey[128] = "bad17ac413051e22060aaff575986cf3493c6b5ff27d67c272ade78eb4a27773";/*{'\0'};*/
    char encryptedItexKey[128] = {'\0'};
    char* jsonStr = 0;


    cJSON* json = NULL;

    hmac_sha256(itexKey, strlen(itexKey), SHA256_NQR_KEY_LIVE, strlen(SHA256_NQR_KEY_LIVE), encryptedItexKey);


    LOG_PRINTF("\nEncrypted Key : %s\n", encryptedItexKey);

    getCliRef(nqr->clientReference);

    json = cJSON_CreateObject();
    if (!json) {
        gui_messagebox_show("Error", "Can't Create CJSON Object", "", "", 2000);
        return -1;
    }

    cJSON_AddItemToObject(json, "codeType", cJSON_CreateString("dynamic"));
    cJSON_AddItemToObject(json, "amount", cJSON_CreateString("200"));
    cJSON_AddItemToObject(json, "channel", cJSON_CreateString("LINUXPOS"));
    cJSON_AddItemToObject(json, "service", cJSON_CreateString("PURCHASE"));
    cJSON_AddItemToObject(json, "tid", cJSON_CreateString(nqr->tid));   
    cJSON_AddItemToObject(json, "clientReference", cJSON_CreateString(nqr->clientReference));   

    jsonStr = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    strncpy((char *)netParam->host, NQR_IP, sizeof(netParam->host) - 1);
    netParam->receiveTimeout = 60000 * 3;
	strncpy(netParam->title, "Request", 10);
    netParam->isHttp = 1;
    netParam->isSsl = 0;
    netParam->port = NQR_PORT;
    netParam->endTag = "";

    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "POST /api/v1/vas/nqr/ptsp/code/generate HTTP/1.1\r\n");
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Host: %s:%d\r\n", netParam->host, netParam->port);
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "X-ITEX-KEY:%s\r\n", encryptedItexKey);
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Content-Type: application/json\r\n");
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "%s\r\n", jsonStr);
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Content-Length: %zu\r\n", strlen(jsonStr));

    printf("\nJson : %s\n", jsonStr);

    LOG_PRINTF("\n PAcket : %s\n", netParam->packet);


    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL){
        return -2;
    }

    return 0;
}

int checkNQRStatus(NetWorkParameters * netParams, MerchantNQR * merchantParams)
{
    char request[1024] = {'\0'};
    char response[2048] = {'\0'};
    char itexKey[] = "765thjkl";
    char encryptedItexKey[128] = {'\0'};
    char* jsonStr = 0;

    //do the request

    return 0;
}

void doMerchantNQRTransaction()
{
    int ret = -1;

    NetWorkParameters netParam;
    MerchantData mParams;
    MerchantNQR nqr;
    memset(&nqr, 0x00, sizeof(MerchantNQR));
    memset(&mParams, 0x00, sizeof(MerchantData));
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    readMerchantData(&mParams);

    if (mParams.tid[0]) {
        strncpy(nqr.tid, mParams.tid, 14);
    }

    nqr.amount = inputamount_page("Input Amount", 7, 10000);

    getMerchantQRCode(&netParam, &nqr);

    printf("After Request");

}