#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "appInfo.h"
#include "merchant.h"
#include "merchantNQR.h"

#include "Receipt.h"

#include "cJSON.h"
#include "util.h"
#include "network.h"
#include "log.h"
#include "sdk_showqr.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "pages/inputamount_page.h"

#define NQR_IP "197.253.19.76"
// #define NQR_PORT 1880
#define NQR_PORT 8019

#define SHA256_NQR_KEY_TEST "N2Q0Yjg5YTg5MWI0OWZjMzRkNDM2MDQzMTQwZWJmNWQ1ZTkzN2ZhOGJkNTg3YTk5YWY4NjRkMWY3NTZmN2U0Nw==" //(testKey)
#define SHA256_NQR_KEY_LIVE "YTRjNjcxYzcwNDc2NTViOWRiMzRlZjcwNzc2NjFkOWYwOTMyMTZlMWFjMWNmZWY2ZTljNWQ2NTk2NGI0OTg1ZA==" //(LIVEkey)
#define NQR_SIGNATURE "aa281eca062cf493407d8a3c2d4411835a4a424d1a492de903e363bc4b3f6b59"


int getNumberOfTransaction(int timeout, const char* title)
{
    char number[32] = { 0 };
    int result = -1;

    gui_clear_dc();
    result = Util_InputMethod(GUI_LINE_TOP(0), (char *)title, GUI_LINE_TOP(2), number, 1, sizeof(number) - 1, 1, timeout);

    if (result <= 0) {
        return -1;
    }

    return atoi(number);
}

static int confirmYesOrNo(const char* title, const char* text)
{
    int result = 0;

    gui_clear_dc();
	result = gui_messagebox_show((char *)title, (char *)text, "Cancel", "Ok", 0);

    return result == 1 ? result : 0;
}

static int displayTransMenu(char** menus, const char *size)
{
    int option = -1;

    option = gui_select_page_ex("Select Trans", menus, size, 30000, 0); // if exit : -1, timout : -2
	
    if (option < 0) {
        return -1;
    }

    return option;
}

static int processTransResponse(cJSON *trans, MerchantNQR* nqr)
{
    cJSON* itm = NULL;

    itm = cJSON_GetObjectItemCaseSensitive(trans, "status");
    if (cJSON_IsNull(itm) || !cJSON_IsString(itm)) {
        gui_messagebox_show("Error", "Invalid Status", "", "", 2000);
        cJSON_Delete(trans);
        return -1;
    }
    strncpy(nqr->status, itm->valuestring, sizeof(nqr->status) - 1);

    itm = cJSON_GetObjectItemCaseSensitive(trans, "transactionDate");
    if (cJSON_IsNull(itm) || !cJSON_IsString(itm)) {
        gui_messagebox_show("Error", "Invalid Date", "", "", 2000);
        cJSON_Delete(trans);
        return -1;
    }
    strncpy(nqr->dateAndTime, itm->valuestring, sizeof(nqr->dateAndTime) - 1);
    
    itm = cJSON_GetObjectItemCaseSensitive(trans, "orderSn");
    if (cJSON_IsNull(itm) || !cJSON_IsString(itm)) {
        gui_messagebox_show("Error", "Invalid Order Serial No", "", "", 2000);
        cJSON_Delete(trans);
        return -1;
    }
    strncpy(nqr->orderSn, itm->valuestring, sizeof(nqr->orderSn) - 1);

    itm = cJSON_GetObjectItemCaseSensitive(trans, "amount");
    if (cJSON_IsNull(itm) || !cJSON_IsString(itm)) {
        gui_messagebox_show("Error", "Invalid Amount", "", "", 2000);
        cJSON_Delete(trans);
        return -1;
    }
    strncpy(nqr->amount, itm->valuestring, sizeof(nqr->amount) - 1);

    itm = cJSON_GetObjectItemCaseSensitive(trans, "rrn");
    if (cJSON_IsNull(itm) || !cJSON_IsString(itm)) {
        gui_messagebox_show("Error", "Invalid RRN", "", "", 2000);
        cJSON_Delete(trans);
        return -1;
    }     
    strncpy(nqr->rrn, itm->valuestring, sizeof(nqr->rrn) - 1);

    itm = cJSON_GetObjectItemCaseSensitive(trans, "merchantName");
    if (cJSON_IsNull(itm) || !cJSON_IsString(itm)) {
        gui_messagebox_show("Error", "Invalid Merchant Name", "", "", 2000);
        cJSON_Delete(trans);
        return -1;
    }      
    strncpy(nqr->merchantName, itm->valuestring, sizeof(nqr->merchantName) - 1);

    itm = cJSON_GetObjectItemCaseSensitive(trans, "merchantNo");
    if (cJSON_IsNull(itm) || !cJSON_IsString(itm)) {
        gui_messagebox_show("Error", "Invalid Merchant No", "", "", 2000);
        cJSON_Delete(trans);
        return -1;
    }
    strncpy(nqr->merchantNo, itm->valuestring, sizeof(nqr->merchantNo) - 1);

    LOG_PRINTF("Response Message : %s", nqr->responseMessage);
    LOG_PRINTF("Response Code : %s", nqr->responseCode);
    LOG_PRINTF("Amount : %s", nqr->amount);

    return 0;
}

int processNqrMenu(cJSON* transactions, MerchantNQR *nqr, const size_t size)
{
    int index;
    int ret = -1;
    char *menus[size];
    MenuItems menuItems[size];
    short nextOption = 0;

    memset(menus, 0x00, sizeof(menus));
    memset(menuItems, 0x00, sizeof(menuItems));

    for (index = 0; index < size; index++) {

        cJSON* amount;
        cJSON* transactionDate;
        char date[12] = {'\0'};

        cJSON* object = cJSON_GetArrayItem(transactions, index);
        memset(nqr, 0, sizeof(MerchantNQR));

        if (index > (size - 1))
            return ret;

        amount = cJSON_GetObjectItemCaseSensitive(object, "amount");
        transactionDate = cJSON_GetObjectItemCaseSensitive(object, "transactionDate");

        if (cJSON_IsNull(amount) || cJSON_IsNull(transactionDate)) {
            gui_messagebox_show("Error", "Invalid Null", "", "", 2000);
            return ret;
        }

        strncpy(date, transactionDate->valuestring, 10);
        sprintf(menuItems[index].label, "N%s %s", amount->valuestring, date);
        menus[index] = menuItems[index].label;

    }

    while (1) {
        int selection = -1;
        
        selection = displayTransMenu(menus, size);
        LOG_PRINTF("selection value %d", selection);

        if (selection < 0) {
            ret = -1;
            break;
        } else {

            cJSON* trans = NULL;
            if (selection > size) {
                ret = -1;
                break;
            }

            trans = cJSON_GetArrayItem(transactions, selection);

            LOG_PRINTF("Got this array item %s", cJSON_PrintUnformatted(trans));
            ret = processTransResponse(trans, nqr);

            if (ret) {
                return -1;
            } else {

                char buff[64] = {'\0'};
                sprintf(buff, "Amount: %s\nTXN Datetime:\n%s ", nqr->amount, nqr->dateAndTime);
                if (!confirmYesOrNo("CONFIRM TXN", buff)) {
                    gui_messagebox_show("Error", "TXN ABORT", "", "", 2000);
                    continue;
                } else {
                    ret = 0;
                    break;
                }
            }
        }
    }

    return ret;
}

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

void getNqrSignature(const char* input, char* output)
{
    char signature[SHA256_DIGEST_LENGTH] = {'\0'};
    int i = 0;

    hmac_sha256(input, strlen(input), SHA256_NQR_KEY_LIVE, strlen(SHA256_NQR_KEY_LIVE), signature);
    bin2hex((unsigned char*)signature, output, sizeof(signature));

    LOG_PRINTF("\nEncrypted Key : %s\n", output);

    for(i=0; i<=strlen(output); i++){
        if(output[i] >=65 && output[i] <= 90)
        output[i] = output[i] + 32;
    }

}


int getMerchantQRCode(MerchantNQR *nqr)
{
    int ret;
    unsigned long amount = 0;
    char encryptedItexKey[2 * SHA256_DIGEST_LENGTH + 1] = {'\0'};
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

    getNqrSignature(jsonStr, encryptedItexKey);

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

    free(jsonStr);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL){
        cJSON_Delete(json);
        return -2;
    }


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

    strncpy(nqr->qrCode, qrCode->valuestring, sizeof(nqr->qrCode) - 1);
    strncpy(nqr->productCode, productCode->valuestring, sizeof(nqr->productCode) - 1);
    strncpy(nqr->responseCode, responseCode->valuestring, sizeof(nqr->responseCode) - 1);
    strncpy(nqr->responseMessage, responseMessage->valuestring, sizeof(nqr->responseMessage) - 1);
    strncpy(nqr->merchantName, merchantName->valuestring, sizeof(nqr->merchantName) - 1);
    strncpy(nqr->merchantNo, merchantNo->valuestring, sizeof(nqr->merchantNo) - 1);
    strncpy(nqr->orderNo, orderNo->valuestring, sizeof(nqr->orderNo) - 1);
    strncpy(nqr->orderSn, orderSn->valuestring, sizeof(nqr->orderSn) - 1);

    if (strcmp(responseCode->valuestring, "00") > 0 || strcmp(responseCode->valuestring, "00") < 0) {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "OK", 2000);
        cJSON_Delete(json);
        return -10;
    }

    cJSON_Delete(json);

    return 0;
}

int checkNQRStatus(MerchantNQR * nqr)
{
    char encryptedItexKey[2 * SHA256_DIGEST_LENGTH + 1] = {'\0'};
    char* jsonStr = 0;
    char* responseJson = 0;

    cJSON* json;
    cJSON* responseCode;
    cJSON* responseMessage;

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

    getNqrSignature(jsonStr, encryptedItexKey);

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

    free(jsonStr);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        cJSON_Delete(json);
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

    cJSON_Delete(json);

    return 0;
}

int queryNqrEodTransaction(MerchantNQR * nqr)
{
    char encryptedItexKey[2 * SHA256_DIGEST_LENGTH + 1] = {'\0'};
    int total = 0;
    char* jsonStr = 0;
    char* responseJson = 0;

    cJSON* json;
    cJSON* responseCode;
    cJSON* responseMessage;
    cJSON* data;
    cJSON* eod;
    cJSON* totalCount;
    cJSON* totalSum;
    cJSON* eodElement = NULL;

    NetWorkParameters netParam;
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    json = cJSON_CreateObject();
    if (!json) {
        gui_messagebox_show("Error", "Can't Create CJSON Object", "", "", 0);
        return -1;
    }

    cJSON_AddItemToObject(json, "date", cJSON_CreateString(nqr->dateAndTime));
    cJSON_AddItemToObject(json, "tid", cJSON_CreateString(nqr->tid));     

    jsonStr = cJSON_PrintUnformatted(json);

    getNqrSignature(jsonStr, encryptedItexKey);

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

    free(jsonStr);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        cJSON_Delete(json);
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
/*
    {
        "responseCode": "00",
        "message": "EOD Retrieved",
        "data": {
            "responseCode": "00",
            "message": "EOD Retrieved",
            "eod": [
                {
                    "time": "09:36:58",
                    "status": "processed",
                    "gateway": "NQR",
                    "amount": "30.00",
                    "rrn": "202104687990807003832817536874"
                },
                {
                    "time": "09:34:28",
                    "status": "processed",
                    "gateway": "NQR",
                    "amount": "20.00",
                    "rrn": "202104506304024624035374478870"
                }
            ],
            "totalCount": 2,
            "totalSum": "50.00"
        }
    }
*/
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

    if (strcmp(responseCode->valuestring, "00")) {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "", 2000);
        cJSON_Delete(json);
        return -8;
    }

    data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (cJSON_IsNull(data) || !cJSON_IsObject(data))    {
        gui_messagebox_show("Error", "Invalid Response Data", "", "OK", 2000);
        cJSON_Delete(json);
        return -10;
    }

    eod = cJSON_GetObjectItemCaseSensitive(data, "eod");
    if (cJSON_IsNull(eod) || !cJSON_IsArray(eod))    {
        gui_messagebox_show("Error", "EOD Object not found!", "", "OK", 2000);
        cJSON_Delete(json);
        return -11;
    }

    totalCount = cJSON_GetObjectItemCaseSensitive(data, "totalCount");
    if (cJSON_IsNull(totalCount))    {
        gui_messagebox_show("Error", "Total Trans not found!", "", "OK", 2000);
        cJSON_Delete(json);
        return -12;
    } else if (cJSON_IsNumber(totalCount)) {
        total = totalCount->valueint;
    } else if (cJSON_IsString(totalCount)) {
        total = atoi(totalCount->valuestring);
    }

    totalSum = cJSON_GetObjectItemCaseSensitive(data, "totalSum");
    if (cJSON_IsNull(totalSum))    {
        gui_messagebox_show("Error", "Total Sum not found!", "", "OK", 2000);
        cJSON_Delete(json);
        return -13;
    } else if (!cJSON_IsString(totalSum)) {
        gui_messagebox_show("Error", "Invalid Total sum!", "", "OK", 2000);
        cJSON_Delete(json);
        return -13;
    }

    printPtspMerchantEodReceiptHead(nqr->dateAndTime);

    cJSON_ArrayForEach(eodElement, eod){
        cJSON* time;
        cJSON* status;
        cJSON* gateway;
        cJSON* amount;
        cJSON* rrn;
        char isApproved = 'D';

        time = cJSON_GetObjectItemCaseSensitive(eodElement, "time");
        status = cJSON_GetObjectItemCaseSensitive(eodElement, "status");
        gateway = cJSON_GetObjectItemCaseSensitive(eodElement, "gateway");
        amount = cJSON_GetObjectItemCaseSensitive(eodElement, "amount");
        rrn = cJSON_GetObjectItemCaseSensitive(eodElement, "rrn");

        if (cJSON_IsNull(time) || !cJSON_IsString(time)){
            gui_messagebox_show("Error", "Invalid Time", "", "OK", 2000);
            cJSON_Delete(json);
            return -9;
        }
        if (cJSON_IsNull(status) || !cJSON_IsString(status)){
            gui_messagebox_show("Error", "Invalid Status", "", "OK", 2000);
            cJSON_Delete(json);
            return -10;
        }
        if (cJSON_IsNull(gateway) || !cJSON_IsString(gateway)){
            gui_messagebox_show("Error", "Invalid Gateway", "", "OK", 2000);
            cJSON_Delete(json);
            return -11;
        }
        if (cJSON_IsNull(amount) || !cJSON_IsString(amount)){
            gui_messagebox_show("Error", "Invalid Amount", "", "OK", 2000);
            cJSON_Delete(json);
            return -12;
        }
        if (cJSON_IsNull(rrn) || !cJSON_IsString(rrn)){
            gui_messagebox_show("Error", "Invalid RRN", "", "OK", 2000);
            cJSON_Delete(json);
            return -13;
        }
        
        memset(&nqr->dateAndTime, '\0', sizeof(nqr->dateAndTime));
        memset(&nqr->amount, '\0', sizeof(nqr->amount));
        memset(&nqr->rrn, '\0', sizeof(nqr->rrn));

        strncpy(nqr->dateAndTime, time->valuestring, sizeof(nqr->dateAndTime) - 1);
        strncpy(nqr->amount, amount->valuestring, sizeof(nqr->amount) - 1);
        strncpy(nqr->rrn, rrn->valuestring, sizeof(nqr->rrn) - 1);

        if (strcmp(status->valuestring, "processed") == 0) {
            isApproved = 'A';
        }

        printPtspMerchantEodBody(nqr, isApproved);        
    }

    printPtspMerchantEodReceiptFooter(totalSum->valuestring, total);
    
    return 0;
}

void fetchNQREOD()
{
    MerchantData mParams;
    MerchantNQR nqr;
    memset(&nqr, 0x00, sizeof(MerchantNQR));
    memset(&mParams, 0x00, sizeof(MerchantData));

    readMerchantData(&mParams);

    if (mParams.tid[0]) {
        strncpy(nqr.tid, mParams.tid, 14);
    }
    
    memset(nqr.dateAndTime, 0x00, sizeof(nqr.dateAndTime));
    getDate(nqr.dateAndTime);
    printf("Date : %s\n", nqr.dateAndTime);


    queryNqrEodTransaction(&nqr);
}


int getLastNQRTransaction(MerchantNQR *nqr)
{
    char encryptedItexKey[2 * SHA256_DIGEST_LENGTH + 1] = {'\0'};
    char* jsonStr = 0;
    char* responseJson = 0;
    char buff[64] = {'\0'};
    int limit = 0;

    cJSON* json;
    cJSON* responseCode;
    cJSON* responseMessage;
    cJSON* data;
    cJSON* transactions;


    NetWorkParameters netParam;
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    limit = getNumberOfTransaction(30000, "NO OF TXNS(1-50)");

    if (limit < 0) {
        return -1;
    } else if (limit == 0 || limit > 50) {
        gui_messagebox_show("Error", "Enter a number between 1 and 50", "", "", 3000);
        return -1;
    }

    sprintf(buff, "Get the last %d successful Trans?", limit);
    if (!confirmYesOrNo("CONFIRM", buff)) {
        return -1;
    }

    json = cJSON_CreateObject();
    if (!json) {
        gui_messagebox_show("Error", "Can't Create CJSON Object", "", "", 0);
        return -1;
    }

    memset(buff, 0x00, sizeof(buff));
    sprintf(buff, "%d", limit);

    cJSON_AddItemToObject(json, "limit", cJSON_CreateString(buff));
    cJSON_AddItemToObject(json, "tid", cJSON_CreateString(nqr->tid));   

    jsonStr = cJSON_PrintUnformatted(json);

    getNqrSignature(jsonStr, encryptedItexKey);

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

    free(jsonStr);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        cJSON_Delete(json);
        return -1;
    }

    if (!netParam.responseSize) {
        gui_messagebox_show("Error", "No Response Received", "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }

    if (!(responseJson = strchr(netParam.response, '{'))) {
        gui_messagebox_show("Error", "Invalid Response Received", "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }

    json = cJSON_Parse(responseJson);
    if (cJSON_IsNull(json))  {
        gui_messagebox_show("Error", "Invalid Response JSON", "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }

    responseCode = cJSON_GetObjectItemCaseSensitive(json, "responseCode");
    if (cJSON_IsNull(responseCode) || !cJSON_IsString(responseCode)) {
        gui_messagebox_show("Error", "Invalid Response Code", "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }
    strncpy(nqr->responseCode, responseCode->valuestring, sizeof(nqr->responseCode) - 1);

    responseMessage = cJSON_GetObjectItemCaseSensitive(json, "message");
    if (cJSON_IsNull(responseMessage) || !cJSON_IsString(responseMessage)) {
        gui_messagebox_show("Error", "Invalid Response Message", "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }

     if (strcmp(responseCode->valuestring, "00")) {
        gui_messagebox_show("Error", responseMessage->valuestring, "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }
    strncpy(nqr->responseMessage, responseMessage->valuestring, sizeof(nqr->responseMessage) - 1);


/*
    {
        "responseCode": "00",
        "message": "Recent Transactions Retrieved",
        "data": {
            "responseCode": "00",
            "message": "Recent Transactions Retrieved",
            "transactions": [
                {
                    "amount": "50.00",
                    "status": "processed",
                    "rrn": "202104278226010079674405040184",
                    "orderSn": "110013210429141437338355275902",
                    "merchantName": "Shanu Oluwasegun M",
                    "subMerchantName": "Shanu Oluwasegun M - 2058LS73",
                    "merchantNo": "M0000326606",
                    "subMerchantNo": "S0000270727",
                    "clientReference": "0809MorefunH909149120060900017812082058LS731102210429141435",
                    "transactionDate": "2021-04-29 14:17:21"
                }
            ]
        }
    }
*/

    data = cJSON_GetObjectItemCaseSensitive(json, "data");
    if (cJSON_IsNull(data) || !cJSON_IsObject(data)) {
        gui_messagebox_show("Error", "Invalid Data Response", "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }

    transactions = cJSON_GetObjectItemCaseSensitive(data, "transactions");
    if (cJSON_IsNull(transactions) || !cJSON_IsArray(transactions)) {
        gui_messagebox_show("Error", "Transaction Not Found", "", "", 2000);
        cJSON_Delete(json);
        return -1;
    }

    return processNqrMenu(transactions, nqr, cJSON_GetArraySize(transactions));
}

void doMerchantNQRTransaction()
{
    short ret = -1;
    int completed = 0;
    unsigned long amount = 0;

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

    sprintf(nqr.amount, "%.2f", amount / 100.0);
    
    getDateAndTime(nqr.dateAndTime);
    getRrn(nqr.rrn);
    ret = getMerchantQRCode(&nqr);
    if (ret < 0) {
        LOG_PRINTF("Get QR Returns : %d", ret);
        return ;
    }

    completed = displayQr(nqr.qrCode, nqr.rrn);
    if (completed != 1){
        printNQRCode(nqr.qrCode);
        return;
    }

    checkNQRStatus(&nqr);
    printNQRReceipts(&nqr, 0);
}

void lastNQRTransaction()
{
    MerchantNQR nqr;
    MerchantData mParams;
    memset(&mParams, 0x00, sizeof(MerchantData));
    memset(&nqr, 0x00, sizeof(MerchantNQR));

    readMerchantData(&mParams);

    if (mParams.tid[0]) {
        strncpy(nqr.tid, mParams.tid, 14);
    }

    if (getLastNQRTransaction(&nqr)) return;

    printNQRLastTXNReceipt(&nqr, REPRINT_COPY);

}