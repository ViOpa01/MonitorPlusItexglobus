/**
 * File: payWithPhone.c 
 * --------------------
 * Implements payWithPhone.h's interface.
 */

#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vas/simpio.h"
#include "vas/jsobject.h"
#include "EmvDB.h"
#include "EmvDBUtil.h"
#include "merchant.h"


#include "ussdb.h"
#include "ussdadmin.h"
#include "ussdservices.h"
#include "payWithPhone.h"

extern "C" {
#include "sha256.h"
#include "cJSON.h"
#include "util.h"
#include "network.h"
#include "nibss.h"

#include "libapi_xpos/inc/libapi_util.h"
#include "libapi_xpos/inc/libapi_gui.h"
}

typedef struct PayWithPhone {
    char ptspId[32];
    char phoneNumber[17];
    char merchantId[16];
    char terminalId[9];
    char merchantType[7];
    char date[11];
    char formattedTimestamp[32];
    unsigned long amount;
    unsigned long fee;
    char rrn[13];
    char requestId[16];

    int result; // 0 -> approved, 1 -> declined
    char responseMessage[64];
} PayWithPhone;

static char PAY_WITH_PHONE_IP[] = "https://merchant.payattitude.com/pay/phone";
static char PAY_WITH_PHONE_PTSPID[] = "ITEX";
static unsigned long PAY_WITH_PHONE_FEE = 5000L;


static short getRRN(char rrn[13])
{
	int result;

	gui_clear_dc();
	if ((result = Util_InputMethod(GUI_LINE_TOP(2), "Enter RRN", GUI_LINE_TOP(5), rrn, 12, 12, 1, 1000)) != 12)
	{
		printf("rrn input failed ret : %d\n", result);
		printf("rrn %s\n", rrn);
		return result;
	}

	printf("rrn : %s\n", rrn);

	return 0;
}

int getShaWithSalt(char* hash, const char* data, const char* key)
{
    sha256_context Context;
    unsigned char keyBin[16];
    unsigned char digest[32];
    char hashString[1024] = {0};
    char hashBcd[1024] = {0};
    size_t length = strlen(key);


    strcat(hashString, key);
    if (stringToHex(hashString + length, sizeof(hashString) - length, data, strlen(data)) < 0) {
        return -1;
    }

    length = strlen(hashString);

    Util_Asc2Bcd(hashString, hashBcd, length);

    sha256_starts(&Context);
    sha256_update(&Context, (unsigned char *)hashBcd, length /= 2);
    sha256_finish(&Context, digest);

    Util_Bcd2Asc((char*)digest, hash, sizeof(digest) * 2);

    return 0;
}

short getPayWithPhoneHash(char* payWithPhoneHash, const PayWithPhone* payWithPhone)
{
    char hashData[256] = { '\0' };
    // const char* PAY_WITH_PHONE_KEY = "E339A8D1CDD8790DBE832728E68FC1E6"; // test key
    const char* PAY_WITH_PHONE_KEY = "FCA09AF9F8C7C70D4B1EB3E496A31756";  //live key
    // size_t len = strlen(payWithPhone->date);

    sprintf(hashData, "%s%s%s%s%.2s%.2s%.2s", payWithPhone->merchantId, payWithPhone->terminalId, payWithPhone->phoneNumber, payWithPhone->rrn, payWithPhone->date + 8, payWithPhone->date, payWithPhone->date + 3);

    return getShaWithSalt(payWithPhoneHash, hashData, PAY_WITH_PHONE_KEY);
}

//TODO: Print receipt
std::map<std::string, std::string> payWithPhoneToRecord(const USSDService service, const PayWithPhone* payPhone)
{

    std::map<std::string, std::string> values;
    // char feeStr[21] = { '\0' };
    // char priceStr[21] = { '\0' };
    char amtStr[21] = { '\0' };

    // fillReceiptHeader(values);

    // sprintf(date, "%.2s/%.2s/%s", payPhone->date + 3, payPhone->date, payPhone->date + 6);

    values[USSD_DATE] = payPhone->formattedTimestamp;
    values[USSD_STATUS] = payPhone->result == 1 ? USSDB::trxStatusString(USSDB::APPROVED) : USSDB::trxStatusString(USSDB::DECLINED);

    if (payPhone->responseMessage[0]) {
        values[USSD_STATUS_MESSAGE] = payPhone->responseMessage;
    }

    values[USSD_REF] = payPhone->rrn;
    values[USSD_PHONE] = payPhone->phoneNumber;

    // sprintf(feeStr, "%.2f", payPhone->fee / 100.0);
    // values[USSD_FEE] = feeStr;

    // sprintf(priceStr, "%.2f", payPhone->amount / 100.0);
    // values["price"] = priceStr;

    sprintf(amtStr, "%lu", payPhone->amount);
    values[USSD_AMOUNT] = amtStr;

    values[USSD_SERVICE] = ussdServiceToString(service);

    return values;
}

static short getPhoneNumber(char* phoneNumber)
{
    if (getNumeric(phoneNumber, 15, 0, 30000, "Phone Number", "Enter Phone #", UI_DIALOG_TYPE_NONE) < 0)
        return -1;

    return 0;
}

static unsigned long getPayWithPhoneAmount(void)
{
    int amount;

    amount = getAmount("Pay with Phone");
    return amount < 0 ? 0 : amount;
}

const char * payWithPhoneError(const char* statusCode)
{
    if (!statusCode) {
        return "Null Code";
    } else if (!strcmp(statusCode, "00")) {
        return "Success";
    } else if (!strcmp(statusCode, "01")) {
        return "Timeout";
    } else if (!strcmp(statusCode, "02")) {
        return "Terminal Error";
    } else if (!strcmp(statusCode, "03")) {
        return "Merchant Error";
    } else if (!strcmp(statusCode, "04")) {
        return "Auth Error";
    } else if (!strcmp(statusCode, "05")) {
        return "Declined";
    } else if (!strcmp(statusCode, "06")) {
        return "Invalid";
    } else if (!strcmp(statusCode, "07")) {
        return "Invalid Amount";
    }
    
    return "Unknown Error";
}

short confirmPayWithPhone(const PayWithPhone* payPhone)
{
    char message[128] = { '\0' };

    // sprintf(message, "Mobile: %s\nAmount: %.2f, Fee: %.2f\nTotal: %.2f", payPhone->phoneNumber, payPhone->amount / 100.0f, payPhone->fee / 100.0f, (payPhone->amount + payPhone->fee) / 100.0f);
    sprintf(message, "Mobile: %s\nAmount: %.2f", payPhone->phoneNumber, payPhone->amount / 100.0f);

    if (UI_ShowOkCancelMessage(30000, "Correct?", message, UI_DIALOG_TYPE_QUESTION))
        return -1;

    return 0;
}

short getPayWithPhoneData(PayWithPhone* payPhone)
{
    MerchantData mParam;
    MerchantParameters parameter;
    char errorMessage[512] = "Missing Data: ";
    bool noError = true;

    // char rrn[13] = { '\0' };
    char nextRrn[13] = { '\0' };

    time_t now;
    struct tm* now_tm;

    memset(&mParam, 0x00, sizeof(MerchantData));
    memset(&parameter, 0x00, sizeof(MerchantParameters));

    readMerchantData(&mParam);
    getParameters(&parameter);


    // if (merchant.error()) {
    //     UI_ShowButtonMessage(5000, "MERCHANT DATA", "Merchant Not Configured", "Exit", UI_DIALOG_TYPE_CAUTION);
    //     return false;
    // }

    std::string tid = mParam.tid;
    if (tid.empty()) {
        strcat(errorMessage, noError ? "Terminal Id" : ", Terminal Id");
        noError = false;
    }

    std::string cardAcceptiorId = parameter.cardAcceptiorIdentificationCode;
    if (cardAcceptiorId.empty()) {
        strcat(errorMessage, noError ? "Merchant Id" : ", Merchant Id");
        noError = false;
    }

    std::string mcc = parameter.merchantCategoryCode;
    if (!noError) {
        UI_ShowButtonMessage(3000, "Config Error", errorMessage, "Exit", UI_DIALOG_TYPE_CAUTION);
        return -1;
    }

    payPhone->amount = getPayWithPhoneAmount();
    if (payPhone->amount <= 0)
        return -2;

    if (getPhoneNumber(payPhone->phoneNumber))
        return -3;

    if (confirmPayWithPhone(payPhone))
        return -4;

    strncpy(payPhone->ptspId, PAY_WITH_PHONE_PTSPID, sizeof(payPhone->ptspId) - 1);
    strncpy(payPhone->merchantId, cardAcceptiorId.c_str(), sizeof(payPhone->merchantId) - 1);
    strncpy(payPhone->terminalId, tid.c_str(), sizeof(payPhone->terminalId) - 1);
    strncpy(payPhone->merchantType, mcc.c_str(), sizeof(payPhone->merchantType) - 1);

    getRrn(payPhone->rrn);
    strncpy(payPhone->requestId, payPhone->rrn + 6, 5); 

    time(&now);
    now_tm = localtime(&now);
    getFormattedDateTime(payPhone->formattedTimestamp, sizeof(payPhone->formattedTimestamp));
    sprintf(payPhone->date, "%02d/%02d/20%02d", now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_year - 100);
    payPhone->fee = PAY_WITH_PHONE_FEE;

    printf("Pay phone confirmed -> %s", payPhone->rrn);

    return 0; //Return 0 for success
}

static short payWithPhoneBody(cJSON** cJSONBody, const PayWithPhone* payPhone)
{
    cJSON* json = NULL; //{"PTSPId": "ITEY", "PhoneNumber": "08094145645", "MerchantId": "2UP1OY000000378", "TerminalId": "3233P4MW", "MerchantType": "1122333444", "Date": "12/04/2019", "Amount": 500.00, "Fee": 50.00, "RRN": "1122333444" }

    *cJSONBody = cJSON_CreateObject();
    json = *cJSONBody;

    if (json == NULL)
        return -1;

    cJSON_AddItemToObject(json, "PTSPId", cJSON_CreateString(payPhone->ptspId));
    cJSON_AddItemToObject(json, "PhoneNumber", cJSON_CreateString(payPhone->phoneNumber));
    cJSON_AddItemToObject(json, "MerchantId", cJSON_CreateString(payPhone->merchantId));
    cJSON_AddItemToObject(json, "TerminalId", cJSON_CreateString(payPhone->terminalId));
    // cJSON_AddItemToObject(json, "MerchantType", cJSON_CreateString(payPhone->merchantType));
    // cJSON_AddItemToObject(json, "Date", cJSON_CreateString(payPhone->date));
    cJSON_AddItemToObject(json, "Amount", cJSON_CreateNumber(payPhone->amount / 100.0));
    cJSON_AddItemToObject(json, "Fee", cJSON_CreateNumber(payPhone->fee / 100.0));
    cJSON_AddItemToObject(json, "RRN", cJSON_CreateString(payPhone->rrn));
    cJSON_AddItemToObject(json, "RequestId", cJSON_CreateString(payPhone->requestId));

    return 0;
}

//  if (!(*parsed)) {
//     PubBeepErr();
//     toast(1000, "Unexpected Response");
//     return ret;
//   }

static short processPayWithPhoneResponse(PayWithPhone* payPhone, const char* response)
{
    const char *resp = strchr(response, '{');
    cJSON* json = cJSON_Parse(resp); //{"Success": true "Status": "1122333444"}
    const cJSON* nextTag = NULL;
    const char *message = 0;
    int ret = -1;

    if (json == NULL) {
        const char* errPtr = cJSON_GetErrorPtr();
        UI_ShowButtonMessage(2000, "Declined", errPtr != NULL ? errPtr : "Error With Response", "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    printf("json response :\n%s\n", resp);

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "statusCode");

    if (!cJSON_IsString(nextTag) || nextTag->valuestring == NULL) {
        printf("Error with the NextTag\n");
        return ret;
    }
   
    message = payWithPhoneError(nextTag->valuestring);

    if (strncmp(nextTag->valuestring, "00", 2) == 0) {
        UI_ShowButtonMessage(2000, "Approved", message, "OK", UI_DIALOG_TYPE_CONFIRMATION);
        payPhone->result = 1;
        ret = 0;
    } else {
        UI_ShowButtonMessage(2000, "Declined", message, "OK", UI_DIALOG_TYPE_WARNING);
        strncpy((char*)payPhone->responseMessage, message, sizeof(payPhone->responseMessage));
    }

    cJSON_Delete(json);
    return ret;
}

static short payWithPhonePost(NetWorkParameters *netParam, const PayWithPhone* payPhone)
{
    char hash[256] = "Hash: ";
    
    char* data = NULL;
    cJSON* json = NULL;

    

    if (payWithPhoneBody(&json, payPhone))
        return -1;

    data = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (getPayWithPhoneHash(hash + strlen(hash), payPhone) < 0) {
        return -1;
    }

    strncpy((char *)netParam->host, "merchant.payattitude.com", sizeof(netParam->host) - 1);
    netParam->receiveTimeout = 60000 * 3;
	strncpy(netParam->title, "Request", 10);
    netParam->isHttp = 1;
    netParam->isSsl = 1;
    netParam->port = 443;
    netParam->endTag = "";

    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "POST /pay/phone HTTP/1.1\r\n");
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Host: %s:%d\r\n", netParam->host, netParam->port);
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "User-Agent: Itex Morefun\r\n");
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Accept: application/json\r\n");
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Content-Type: application/json\r\n");
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "%s\r\n", hash);
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Content-Length: %zu\r\n", strlen(data));

    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "\r\n%s", data);

    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        // UI_ShowButtonMessage(2000, "Request Error", response->errorMsg[0] ? response->errorMsg : curl_easy_strerror(response->returnCode), "OK", UI_DIALOG_TYPE_WARNING);
        return -2;
    }

    return 0;
}

void payWithPhone()
{
    PayWithPhone payPhone = { 0 };
    NetWorkParameters netParam;
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    if (getPayWithPhoneData(&payPhone) < 0)
        return;

    if (payWithPhonePost(&netParam, &payPhone))
        return;

    if (netParam.responseSize < 1) {
        UI_ShowButtonMessage(5000, "No Response", "Please try again", "Ok", UI_DIALOG_TYPE_WARNING);
        return;
    }

    processPayWithPhoneResponse(&payPhone, bodyPointer((const char*)netParam.response));


    std::map<std::string, std::string> record = payWithPhoneToRecord(PAYATTITUDE_PURCHASE, &payPhone);
    USSDB::saveUssdTransaction(record);
    printUSSDReceipt(record);

    // printPayWithPhoneReceipt(&payPhone);

    //savePayWithPhone(&payWithPhone);
}
