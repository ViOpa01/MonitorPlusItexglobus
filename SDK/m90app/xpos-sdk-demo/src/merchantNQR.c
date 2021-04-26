#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "appInfo.h"
#include "merchant.h"
#include "merchantNQR.h"

#include "../Receipt.h"

#include "cJSON.h"
#include "util.h"
#include "network.h"
#include "log.h"
#include "../sdk_showqr.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "pages/inputamount_page.h"

#define NQR_IP "197.253.19.76"
// #define NQR_PORT 1880
#define NQR_PORT 8019

#define SHA256_NQR_KEY_TEST "N2Q0Yjg5YTg5MWI0OWZjMzRkNDM2MDQzMTQwZWJmNWQ1ZTkzN2ZhOGJkNTg3YTk5YWY4NjRkMWY3NTZmN2U0Nw==" //(testKey)
#define SHA256_NQR_KEY_LIVE "YTRjNjcxYzcwNDc2NTViOWRiMzRlZjcwNzc2NjFkOWYwOTMyMTZlMWFjMWNmZWY2ZTljNWQ2NTk2NGI0OTg1ZA==" //(LIVEkey)
#define NQR_SIGNATURE "aa281eca062cf493407d8a3c2d4411835a4a424d1a492de903e363bc4b3f6b59"



char * getCliRef(char* clientRef)
{
    const char modelTag[4] = "08";
    const char serialTag[4] = "09";
    const char referenceTag[4] = "11";
    const char terminalIdTag[4] = "12";

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


static short uiGetMerchantNQRAmount(unsigned long * amount)
{
    unsigned long amt = inputamount_page("PURCHASE AMOUNT", 10, 90000);

    if (amt == INPUT_INPUT_RET_QUIT) return *amount = amt;

    *amount = amt;

    return 0;
}


int getMerchantQRCode(MerchantNQR *nqr)
{
    int i = 0;
    int ret;
    unsigned long amount = 0;
    char request[1024] = {'\0'};
    char response[2048] = {'\0'};
    char itexBin[128] = {'\0'};
    char encryptedItexKey[128] = {'\0'};
    char* jsonStr = 0;
    char buff[125] = {'\0'};

    cJSON* json = NULL;
    cJSON* responseJson = NULL;
    cJSON* qrData = NULL;
    cJSON* qrCode = NULL;
    cJSON* productCode = NULL;
    cJSON* responseCode = NULL;
    cJSON* responseMessage = NULL;
    cJSON* merchantNo = NULL;
    cJSON* subMerchantNo = NULL;
    cJSON* merchantName = NULL;
    cJSON* orderNo = NULL;
    cJSON* orderSn = NULL;



    NetWorkParameters netParam;
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    getCliRef(nqr->clientReference);

    json = cJSON_CreateObject();
    if (!json) {
        gui_messagebox_show("Error", "Can't Create CJSON Object", "", "", 0);
        return -1;
    }

    cJSON_AddItemToObject(json, "codeType", cJSON_CreateString("dynamic"));
    cJSON_AddItemToObject(json, "amount", cJSON_CreateString(nqr->amount));
    cJSON_AddItemToObject(json, "channel", cJSON_CreateString("LINUXPOS"));
    cJSON_AddItemToObject(json, "service", cJSON_CreateString("PURCHASE"));
    cJSON_AddItemToObject(json, "tid", cJSON_CreateString(nqr->tid));   
    cJSON_AddItemToObject(json, "clientReference", cJSON_CreateString(nqr->clientReference));   

    jsonStr = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    hmac_sha256(jsonStr, strlen(jsonStr), SHA256_NQR_KEY_LIVE, strlen(SHA256_NQR_KEY_LIVE), itexBin);

    bin2hex(itexBin, encryptedItexKey, strlen(itexBin));

    LOG_PRINTF("\nEncrypted Key : %s\n", encryptedItexKey);

    for(i=0; i<=strlen(encryptedItexKey); i++){
        if(encryptedItexKey[i] >=65 && encryptedItexKey[i] <= 90)
        encryptedItexKey[i] = encryptedItexKey[i] + 32;
    }

    strncpy((char *)netParam.host, NQR_IP, sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000 * 3;
	strncpy(netParam.title, "Request", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = NQR_PORT;
    netParam.endTag = "";

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST /api/v1/vas/nqr/ptsp/code/generate HTTP/1.1\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "X-ITEX-KEY: %s\r\n", encryptedItexKey);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n\r\n", strlen(jsonStr));
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "%s\r\n", jsonStr);


    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL){
        return -2;
    }

    free(jsonStr);

    if (!netParam.responseSize) {
        gui_messagebox_show("Error", "No Response Received", "", "OK", 2000);
        cJSON_Delete(json);
        return -3;
    }

    if (!(responseJson = strchr(netParam.response, '{')))    {
        gui_messagebox_show("Error", "Invalid Response Received", "", "OK", 2000);
        cJSON_Delete(json);
        return -4;
    }

    json = cJSON_Parse(responseJson);
    if (cJSON_IsNull(json))    {
        gui_messagebox_show("Error", "Invalid Response Received", "", "OK", 2000);
        cJSON_Delete(json);
        return -5;
    }

    responseCode = cJSON_GetObjectItemCaseSensitive(json, "responseCode");
    if (cJSON_IsNull(responseCode) || !cJSON_IsString(responseCode)) {
        gui_messagebox_show("Error", "Invalid Response Code", "", "OK", 2000);
        cJSON_Delete(json);
        return -6;
    }

    responseMessage = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsNull(responseMessage) || !cJSON_IsString(responseMessage)) {
        gui_messagebox_show("Error", "Invalid Response Message", "", "OK", 2000);
        cJSON_Delete(json);
        return -6;
    }

    qrData = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (cJSON_IsNull(qrData) || !cJSON_IsObject(qrData))    {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "OK", 2000);
        cJSON_Delete(json);
        return -7;
    }

    qrCode = cJSON_GetObjectItemCaseSensitive(qrData, "qrCode");
    if (cJSON_IsNull(qrCode) || !cJSON_IsString(qrCode))    {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "OK", 2000);
        cJSON_Delete(json);
        return -8;
    }


    LOG_PRINTF("1");

    merchantNo = cJSON_GetObjectItemCaseSensitive(qrData, "merchantNo");
    if (cJSON_IsNull(merchantNo) || !cJSON_IsString(merchantNo)) {
        gui_messagebox_show("Error", "Invalid Merchant No", "", "OK", 2000);
        cJSON_Delete(json);
        return -6;
    }

    productCode = cJSON_GetObjectItemCaseSensitive(qrData, "productCode");
    if (cJSON_IsNull(productCode) || !cJSON_IsString(productCode)) {
        gui_messagebox_show("Error", "Invalid Product Code", "", "OK", 2000);
        cJSON_Delete(json);
        return -6;
    }

    LOG_PRINTF("2");


    merchantName = cJSON_GetObjectItemCaseSensitive(qrData, "merchantName");
    if (cJSON_IsNull(merchantName) || !cJSON_IsString(merchantName))    {
        gui_messagebox_show("Error", "Invalid Merchant Name", "", "OK", 2000);
        cJSON_Delete(json);
        return -9;
    }

    orderNo = cJSON_GetObjectItemCaseSensitive(qrData, "orderNo");
    if (cJSON_IsNull(orderNo) || !cJSON_IsString(orderNo))    {
        gui_messagebox_show("Error", "Invalid Order No", "", "OK", 2000);
        cJSON_Delete(json);
        return -9;
    }

    orderSn = cJSON_GetObjectItemCaseSensitive(qrData, "orderSn");
    if (cJSON_IsNull(orderSn) || !cJSON_IsString(orderSn))    {
        gui_messagebox_show("Error", "Invalid Order SN", "", "OK", 2000);
        cJSON_Delete(json);
        return -9;
    }

    LOG_PRINTF("3");

    strncpy(nqr->qrCode, qrCode->valuestring, sizeof(nqr->qrCode) - 1);
    strncpy(nqr->productCode, productCode->valuestring, sizeof(nqr->productCode) - 1);
    strncpy(nqr->responseCode, responseCode->valuestring, sizeof(nqr->responseCode) - 1);
    strncpy(nqr->responseMessage, responseMessage->valuestring, sizeof(nqr->responseMessage) - 1);
    strncpy(nqr->merchantName, merchantName->valuestring, sizeof(nqr->merchantName) - 1);
    strncpy(nqr->merchantNo, merchantNo->valuestring, sizeof(nqr->merchantNo) - 1);
    strncpy(nqr->orderNo, orderNo->valuestring, sizeof(nqr->orderNo) - 1);
    strncpy(nqr->orderSn, orderSn->valuestring, sizeof(nqr->orderSn) - 1);

    LOG_PRINTF("After getting the struct values");


    if (strcmp(responseCode->valuestring, "00") > 0 || strcmp(responseCode->valuestring, "00") < 0) {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "OK", 2000);
        cJSON_Delete(json);
        return -10;
    }

    LOG_PRINTF("Before Delete Json");

    cJSON_Delete(json);

    LOG_PRINTF("After Delete Json");


    return 0;
}

int checkNQRStatus(MerchantNQR * nqr)
{
    int i = 0;
        char request[1024] = {'\0'};
    char response[2048] = {'\0'};
    char itexBin[128] = {'\0'};
    char encryptedItexKey[128] = {'\0'};
    char* jsonStr = 0;
    char* responseJson = 0;

    cJSON* json;
    cJSON* responseCode;
    cJSON* responseMessage;
    // cJSON* status;

    NetWorkParameters netParam;
    memset(&netParam, 0x00, sizeof(NetWorkParameters));


    json = cJSON_CreateObject();
    if (!json) {
        gui_messagebox_show("Error", "Can't Create CJSON Object", "", "", 0);
        return -1;
    }

    cJSON_AddItemToObject(json, "service", cJSON_CreateString("NQR"));
    cJSON_AddItemToObject(json, "clientReference", cJSON_CreateString(nqr->clientReference));   
    cJSON_AddItemToObject(json, "productCode", cJSON_CreateString(nqr->productCode));
    cJSON_AddItemToObject(json, "tid", cJSON_CreateString(nqr->tid));   

    jsonStr = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    hmac_sha256(jsonStr, strlen(jsonStr), SHA256_NQR_KEY_LIVE, strlen(SHA256_NQR_KEY_LIVE), itexBin);

    bin2hex(itexBin, encryptedItexKey, strlen(itexBin));

    LOG_PRINTF("\nEncrypted Key : %s\n", encryptedItexKey);

    for(i=0; i<=strlen(encryptedItexKey); i++){
        if(encryptedItexKey[i] >=65 && encryptedItexKey[i] <= 90)
        encryptedItexKey[i] = encryptedItexKey[i] + 32;
    }

    strncpy((char *)netParam.host, NQR_IP, sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000 * 3;
	strncpy(netParam.title, "Request", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = NQR_PORT;
    netParam.endTag = "";

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST /api/v1/vas/nqr/ptsp/transaction/status HTTP/1.1\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "X-ITEX-KEY: %s\r\n", encryptedItexKey);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n\r\n", strlen(jsonStr));
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "%s\r\n", jsonStr);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL){
        return -2;
    }

    if (!netParam.responseSize) {
        gui_messagebox_show("Error", "No Response Received", "", "", 2000);
        cJSON_Delete(json);
        return -3;
    }
    if (!(responseJson = strchr(netParam.response, '{'))) {
        gui_messagebox_show("Error", "Invalid Response Received", "", "", 2000);
        cJSON_Delete(json);
        return -4;
    }

    json = cJSON_Parse(responseJson);
    if (cJSON_IsNull(json))  {
        gui_messagebox_show("Error", "Invalid Response JSON", "", "", 2000);
        cJSON_Delete(json);
        return -5;
    }

    responseCode = cJSON_GetObjectItemCaseSensitive(json, "responseCode");
    if (cJSON_IsNull(responseCode) || !cJSON_IsString(responseCode)) {
        gui_messagebox_show("Error", "Invalid Response Code", "", "", 2000);
        cJSON_Delete(json);
        return -6;
    }

    responseMessage = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsNull(responseMessage) || !cJSON_IsString(responseMessage)) {
        gui_messagebox_show("Error", "Invalid Response Message", "", "", 2000);
        cJSON_Delete(json);
        return -7;
    }

    memset(&nqr->responseCode, '\0', sizeof(nqr->responseCode));
    memset(&nqr->responseMessage, '\0', sizeof(nqr->responseMessage));
    strncpy(nqr->responseCode, responseCode->valuestring, sizeof(nqr->responseCode) - 1);
    strncpy(nqr->responseMessage, responseMessage->valuestring, sizeof(nqr->responseMessage) - 1);

    gui_messagebox_show("PURCHASE", nqr->responseMessage, "", "", 2000);

    free(jsonStr);

    cJSON_Delete(json);

    return 0;
}

int queryNQREODTransaction(MerchantNQR * nqr)
{
    int i = 0;
    char buff[128] = {'\0'};
    char request[1024] = {'\0'};
    char response[2048] = {'\0'};
    char itexBin[128] = {'\0'};
    char date[10] = {'\0'};
    char encryptedItexKey[128] = {'\0'};
    char* jsonStr = 0;
    char* responseJson = 0;

    char* menu = 0;

    cJSON* json;
    cJSON* responseCode;
    cJSON* responseMessage;
    cJSON* data;
    cJSON* totalCount;
    cJSON* totalSum;
    cJSON* eod;
    cJSON* eodElement = NULL;

    NetWorkParameters netParam;
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    strncpy(date, nqr->dateAndTime, 8);

    // printf("\ndate is : %s\n", date);


    json = cJSON_CreateObject();
    if (!json) {
        gui_messagebox_show("Error", "Can't Create CJSON Object", "", "", 0);
        return -1;
    }

    cJSON_AddItemToObject(json, "date", cJSON_CreateString(date));
    cJSON_AddItemToObject(json, "tid", cJSON_CreateString(nqr->tid));     

    jsonStr = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    hmac_sha256(jsonStr, strlen(jsonStr), SHA256_NQR_KEY_LIVE, strlen(SHA256_NQR_KEY_LIVE), itexBin);

    bin2hex(itexBin, encryptedItexKey, strlen(itexBin));

    LOG_PRINTF("\nEncrypted Key : %s\n", encryptedItexKey);

    for(i=0; i<=strlen(encryptedItexKey); i++){
        if(encryptedItexKey[i] >=65 && encryptedItexKey[i] <= 90)
        encryptedItexKey[i] = encryptedItexKey[i] + 32;
    }

    strncpy((char *)netParam.host, NQR_IP, sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000 * 3;
	strncpy(netParam.title, "Request", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = NQR_PORT;
    netParam.endTag = "";

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST /api/v1/vas/nqr/ptsp/eod HTTP/1.1\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "X-ITEX-KEY: %s\r\n", encryptedItexKey);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n\r\n", strlen(jsonStr));
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "%s\r\n", jsonStr);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL){
        return -2;
    }

    if (!netParam.responseSize) {
        gui_messagebox_show("Error", "No Response Received", "", "", 2000);
        cJSON_Delete(json);
        return -3;
    }

    if (!(responseJson = strchr(netParam.response, '{'))) {
        gui_messagebox_show("Error", "Invalid Response Received", "", "", 2000);
        cJSON_Delete(json);
        return -4;
    }

    json = cJSON_Parse(responseJson);
    if (cJSON_IsNull(json))  {
        gui_messagebox_show("Error", "Invalid Response JSON", "", "", 2000);
        cJSON_Delete(json);
        return -5;
    }

    responseCode = cJSON_GetObjectItemCaseSensitive(json, "responseCode");
    if (cJSON_IsNull(responseCode) || !cJSON_IsString(responseCode)) {
        gui_messagebox_show("Error", "Invalid Response Code", "", "", 2000);
        cJSON_Delete(json);
        return -6;
    }

    responseMessage = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsNull(responseMessage) || !cJSON_IsString(responseMessage)) {
        gui_messagebox_show("Error", "Invalid Response Message", "", "", 2000);
        cJSON_Delete(json);
        return -7;
    }
    
    totalSum = cJSON_GetObjectItemCaseSensitive(json, "totalSum");
    if (cJSON_IsNull(totalSum) || !cJSON_IsNumber(totalSum)) {
        gui_messagebox_show("Error", "No Transaction Found", "", "", 2000);
        cJSON_Delete(json);
        return -8;
    }

    totalCount = cJSON_GetObjectItemCaseSensitive(json, "totalCount");
    if (cJSON_IsNull(totalCount) || !cJSON_IsNumber(totalCount)) {
        gui_messagebox_show("Error", "Invalid Total Count", "", "", 2000);
        cJSON_Delete(json);
        return -9;
    }

    data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (cJSON_IsNull(data) || !cJSON_IsObject(data))    {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "OK", 2000);
        cJSON_Delete(json);
        return -10;
    }

    eod = cJSON_GetObjectItemCaseSensitive(data, "eod");
    if (cJSON_IsNull(eod) || !cJSON_IsArray(eod))    {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "OK", 2000);
        cJSON_Delete(json);
        return -11;
    }

    printPtspMerchantEodReceiptHead(date);

    cJSON_ArrayForEach(eodElement, eod){
        cJSON* time;
        cJSON* status;
        cJSON* gateway;
        cJSON* amount;
        cJSON* rrn;

        time = cJSON_GetObjectItemCaseSensitive(eodElement, "time");
        status = cJSON_GetObjectItemCaseSensitive(eodElement, "status");
        gateway = cJSON_GetObjectItemCaseSensitive(eodElement, "gateway");
        amount = cJSON_GetObjectItemCaseSensitive(eodElement, "amount");
        rrn = cJSON_GetObjectItemCaseSensitive(eodElement, "rrn");

        if (cJSON_IsNull(time) || !cJSON_IsString(time)){
            gui_messagebox_show("Error", "Invalid Time", "", "OK", 2000);
            cJSON_Delete(eodElement);
            return -9;
        }
        if (cJSON_IsNull(status) || !cJSON_IsString(status)){
            gui_messagebox_show("Error", "Invalid Status", "", "OK", 2000);
            cJSON_Delete(eodElement);
            return -10;
        }
        if (cJSON_IsNull(gateway) || !cJSON_IsString(gateway)){
            gui_messagebox_show("Error", "Invalid Gateway", "", "OK", 2000);
            cJSON_Delete(eodElement);
            return -11;
        }
        if (cJSON_IsNull(amount) || !cJSON_IsString(amount)){
            gui_messagebox_show("Error", "Invalid Amount", "", "OK", 2000);
            cJSON_Delete(eodElement);
            return -12;
        }
        if (cJSON_IsNull(rrn) || !cJSON_IsString(rrn)){
            gui_messagebox_show("Error", "Invalid RRN", "", "OK", 2000);
            cJSON_Delete(eodElement);
            return -13;
        }
        
        memset(&nqr->dateAndTime, '\0', sizeof(nqr->dateAndTime));
        memset(&nqr->amount, '\0', sizeof(nqr->amount));
        memset(&nqr->rrn, '\0', sizeof(nqr->rrn));

        strncpy(nqr->dateAndTime, time->valuestring, sizeof(nqr->dateAndTime) - 1);
        strncpy(nqr->amount, amount->valuestring, sizeof(nqr->amount) - 1);
        strncpy(nqr->rrn, rrn->valuestring, sizeof(nqr->rrn) - 1);

        printPtspMerchantEodBody(&nqr);        
    }

    memset(buff, '\0', sizeof(buff));
    sprintf(buff, "%d", totalSum->valueint);

    printPtspMerchantEodReceiptFooter(buff, totalCount->valueint);
    
    free(jsonStr);

    cJSON_Delete(json);

    return 0;
}

void fetchNQREOD()
{
    short ret = -1;

    NetWorkParameters netParam;
    MerchantData mParams;
    MerchantNQR nqr;
    memset(&nqr, 0x00, sizeof(MerchantNQR));
    memset(&mParams, 0x00, sizeof(MerchantData));

    readMerchantData(&mParams);

    if (mParams.tid[0]) {
        strncpy(nqr.tid, mParams.tid, 14);
    }
    
    getDateAndTime(nqr.dateAndTime);

    ret = queryNQREODTransaction(&nqr);
}


int getLastNQRTransaction(MerchantNQR * nqr)
{
    int i = 0;
    char request[1024] = {'\0'};
    char response[2048] = {'\0'};
    char itexBin[128] = {'\0'};
    char encryptedItexKey[128] = {'\0'};
    char* jsonStr = 0;
    char* responseJson = 0;

    cJSON* json;
    cJSON* responseCode;
    cJSON* responseMessage;
    cJSON* data;
    cJSON* transactions;
    cJSON* transElements;


    NetWorkParameters netParam;
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    json = cJSON_CreateObject();
    if (!json) {
        gui_messagebox_show("Error", "Can't Create CJSON Object", "", "", 0);
        return -1;
    }

    cJSON_AddItemToObject(json, "limit", cJSON_CreateString("1"));
    cJSON_AddItemToObject(json, "tid", cJSON_CreateString(nqr->tid));   

    jsonStr = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    hmac_sha256(jsonStr, strlen(jsonStr), SHA256_NQR_KEY_LIVE, strlen(SHA256_NQR_KEY_LIVE), itexBin);

    bin2hex(itexBin, encryptedItexKey, strlen(itexBin));

    LOG_PRINTF("\nEncrypted Key : %s\n", encryptedItexKey);

    for(i=0; i<=strlen(encryptedItexKey); i++){
        if(encryptedItexKey[i] >=65 && encryptedItexKey[i] <= 90)
        encryptedItexKey[i] = encryptedItexKey[i] + 32;
    }

    strncpy((char *)netParam.host, NQR_IP, sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000 * 3;
	strncpy(netParam.title, "Request", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = NQR_PORT;
    netParam.endTag = "";

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST /api/v1/vas/nqr/ptsp/latest/transactions HTTP/1.1\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "X-ITEX-KEY: %s\r\n", encryptedItexKey);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n\r\n", strlen(jsonStr));
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "%s\r\n", jsonStr);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL){
        return -2;
    }

    if (!netParam.responseSize) {
        gui_messagebox_show("Error", "No Response Received", "", "", 2000);
        free(jsonStr);
        cJSON_Delete(json);
        return -3;
    }

    if (!(responseJson = strchr(netParam.response, '{'))) {
        gui_messagebox_show("Error", "Invalid Response Received", "", "", 2000);
        free(jsonStr);
        cJSON_Delete(json);
        return -4;
    }

    json = cJSON_Parse(responseJson);
    if (cJSON_IsNull(json))  {
        gui_messagebox_show("Error", "Invalid Response JSON", "", "", 2000);
        free(jsonStr);
        cJSON_Delete(json);
        return -5;
    }

    responseCode = cJSON_GetObjectItemCaseSensitive(json, "responseCode");
    if (cJSON_IsNull(responseCode) || !cJSON_IsString(responseCode)) {
        gui_messagebox_show("Error", "Invalid Response Code", "", "", 2000);
        free(jsonStr);
        cJSON_Delete(json);
        return -6;
    }

    responseMessage = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsNull(responseMessage) || !cJSON_IsString(responseMessage)) {
        gui_messagebox_show("Error", "Invalid Response Message", "", "", 2000);
        free(jsonStr);
        cJSON_Delete(json);
        return -7;
    }

    data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (cJSON_IsNull(data) || !cJSON_IsObject(data)) {
        gui_messagebox_show("Error", "Invalid Data Response", "", "", 2000);
        free(jsonStr);
        cJSON_Delete(json);
        return -8;
    }

    transactions = cJSON_GetObjectItemCaseSensitive(data, "transactions");
    if (cJSON_IsNull(transactions) || !cJSON_IsArray(transactions) || !transactions->child ) {
        printf("I am here\n\n");
        gui_messagebox_show("Error", "Transaction Not Found", "", "", 2000);
        free(jsonStr);
        cJSON_Delete(json);
        return -9;
    }

    LOG_PRINTF("Transactions : %s", transactions->valuestring);

    printf("\n\nI am here before for loop\n\n");


    cJSON_ArrayForEach(transElements, transactions){
        cJSON* status;
        cJSON* orderSn;
        cJSON* amount;
        cJSON* rrn;
        cJSON* transactionDate;
        cJSON* merchantName;
        cJSON* merchantNo;

        amount = cJSON_GetObjectItemCaseSensitive(transElements, "amount");
        status = cJSON_GetObjectItemCaseSensitive(transElements, "status");
        rrn = cJSON_GetObjectItemCaseSensitive(transElements, "rrn");
        orderSn = cJSON_GetObjectItemCaseSensitive(transElements, "orderSn");
        transactionDate = cJSON_GetObjectItemCaseSensitive(transElements, "transactionDate");
        merchantName = cJSON_GetObjectItemCaseSensitive(transElements, "merchantName");
        merchantNo = cJSON_GetObjectItemCaseSensitive(transElements, "merchantNo");


        if (cJSON_IsNull(transactionDate) || !cJSON_IsString(transactionDate)){
            gui_messagebox_show("Error", "Invalid Date", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -9;
        }
        if (cJSON_IsNull(status) || !cJSON_IsString(status)){
            gui_messagebox_show("Error", "Invalid Status", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -10;
        }
        if (cJSON_IsNull(orderSn) || !cJSON_IsString(orderSn)){
            gui_messagebox_show("Error", "Invalid Order Serial No", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -11;
        }
        if (cJSON_IsNull(amount) || !cJSON_IsString(amount)){
            gui_messagebox_show("Error", "Invalid Amount", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -12;
        }
        if (cJSON_IsNull(rrn) || !cJSON_IsString(rrn)){
            gui_messagebox_show("Error", "Invalid RRN", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -13;
        }        
        if (cJSON_IsNull(merchantName) || !cJSON_IsString(merchantName)){
            gui_messagebox_show("Error", "Invalid Merchant Name", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -13;
        }               
        if (cJSON_IsNull(merchantNo) || !cJSON_IsString(merchantNo)){
            gui_messagebox_show("Error", "Invalid Merchant No", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -13;
        }

        if (!transactionDate || !status || !orderSn || !amount || !rrn || !merchantNo || !merchantName)
        {
            gui_messagebox_show("Error", "No Transaction Found", "", "OK", 2000);
            cJSON_Delete(transElements);
            return -14;
        }

        strncpy(nqr->dateAndTime, transactionDate->valuestring, sizeof(nqr->dateAndTime) - 1);
        strncpy(nqr->responseCode, responseCode->valuestring, sizeof(nqr->responseCode) - 1);
        strncpy(nqr->responseMessage, responseMessage->valuestring, sizeof(nqr->responseMessage) - 1);
        strncpy(nqr->merchantName, merchantName->valuestring, sizeof(nqr->merchantName) - 1);
        strncpy(nqr->merchantNo, merchantNo->valuestring, sizeof(nqr->merchantNo) - 1);
        strncpy(nqr->rrn, rrn->valuestring, sizeof(nqr->rrn) - 1);
        strncpy(nqr->orderSn, orderSn->valuestring, sizeof(nqr->orderSn) - 1);
        strncpy(nqr->status, status->valuestring, sizeof(nqr->status) - 1);
    }

    LOG_PRINTF("Message : %s", responseMessage->valuestring);

    free(jsonStr);

    cJSON_Delete(json);

    return 0;
}

void doMerchantNQRTransaction()
{
    short ret = -1;
    int completed = 0;
    unsigned long amount = 0;

    NetWorkParameters netParam;
    MerchantData mParams;
    MerchantNQR nqr;
    memset(&nqr, 0x00, sizeof(MerchantNQR));
    memset(&mParams, 0x00, sizeof(MerchantData));

    readMerchantData(&mParams);

    if (mParams.tid[0]) {
        strncpy(nqr.tid, mParams.tid, 14);
    }

    ret = uiGetMerchantNQRAmount(&amount);
    if (ret < 0) {
        return ;
    }

    if (!amount){
        return ;
    }

    if (amount < 100) {
        gui_messagebox_show("PURCHASE", "Amount is less than NGN1.00", "", "", 2000);
        return ;
    }

    sprintf(nqr.amount, "%d", amount / 100);
    
    getDateAndTime(nqr.dateAndTime);
    getRrn(nqr.rrn);
    ret = getMerchantQRCode(&nqr);
    LOG_PRINTF("Get QR Returns : %d", ret);
    if (ret < 0) {
        return ;
    }

    completed = displayQr(nqr.qrCode);
    if (completed == 1){
        checkNQRStatus(&nqr);
        printf("\nThis is after the print NQRCode\n");
        return ;
    } else {
        printNQRCode(nqr.qrCode);
        printf("\nThis is after the print NQRCode\n");
        return;
    }

    printNQRReceipts(&nqr, 0);

}

void lastNQRTransaction()
{
    short ret = -1;

    MerchantNQR nqr;
    MerchantData mParams;
    memset(&mParams, 0x00, sizeof(MerchantData));
    memset(&nqr, 0x00, sizeof(MerchantNQR));

    readMerchantData(&mParams);

    if (mParams.tid[0]) {
        strncpy(nqr.tid, mParams.tid, 14);
    }

    ret = getLastNQRTransaction(&nqr);

    LOG_PRINTF("Get Last Transactions Returns : %d", ret);

    if (ret < 0) return;

    printLastNQRReceipts(&nqr, 0);
    
}