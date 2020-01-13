//#include "libapi_xpos/inc/libapi_file.h"
//#include "libapi_xpos/inc/libapi_gui.h"



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include "appInfo.h"
#include "merchant.h"

extern "C" {
#include "network.h"
#include "util.h"
#include "itexFile.h"
#include "ezxml.h"
#include "cJSON.h"
#include "libapi_xpos/inc/libapi_gui.h"

}

#define ITEX_TAMS_PUBLIC_IP "basehuge.itexapp.com"
#define ITEX_TASM_PUBLIC_PORT "80"
#define ITEX_TASM_SSL_PORT "443"
#define DEFAULT_TID "2070AS89"



using namespace std;

static void initTamsParameters(NetWorkParameters * netParam)
{

	netParam->receiveTimeout = 10000;
	strncpy(netParam->title, "TAMS", 10);
	//strncpy(netParam->apn, "CMNET", 10);
    strncpy(netParam->apn, "web.gprs.mtnnigeria.net", sizeof(netParam->apn));
	netParam->netLinkTimeout = 30000;

    netParam->isHttp = 1;
    netParam->isSsl = 0;
}


static short getMerchantDetails(NetWorkParameters * netParam, char *buffer)
{

    char terminalSn[22] = {0};
    char path[0x500] = {'\0'};
    string add;


    strncpy((char *)netParam->host, ITEX_TAMS_PUBLIC_IP, strlen(ITEX_TAMS_PUBLIC_IP));
    netParam->port = atoi(ITEX_TASM_PUBLIC_PORT);

    getTerminalSn(terminalSn);

    sprintf(path, "tams/eftpos/devinterface/transactionadvice.php?action=TAMS_WEBAPI&termID=%s&posUID=%s&ver=%s%s&model=%s&control=TamsSecurity", DEFAULT_TID, terminalSn, APP_NAME, APP_VER, APP_MODEL);
    
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "GET /%s\r\n", path);
 	netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Host: %s:%d", netParam->host, netParam->port);
	netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "%s", "\r\n\r\n");

    printf("request : \n%s\n", netParam->packet);
    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -1;
    }

    memcpy(buffer, netParam->response, strlen((char *)netParam->response));
    return 0;
}

static short parseJsonFile(char *buff, cJSON **parsed)
{
    cJSON *status = NULL;
	int ret = -1;


    const char *json = strchr(buff, '{');

	if (!json) {
		printf("Error Passing json content\n");
		return ret;
	}

    *parsed = cJSON_Parse(json);

    if (!(*parsed)) {
        printf("Invalid content\n");
        return ret;
    }

	status = cJSON_GetObjectItemCaseSensitive(*parsed, "status");

    if(cJSON_IsNumber(status))
    {
        ret = status->valueint;
        printf("Parameter status : %d\n", status->valueint);

    }

    return ret;
   
}

int saveMerchantDataXml(const char* merchantXml)
{
    // takes xml
    // Parse it
    // Save it

    ezxml_t root;
    ezxml_t tran;

    char itexPosMessage[14] = {0};
    MerchantData merchant = { 0 };
    std::string ip_and_port;    // POSVASPUBLIC_SSL, EMPSPUBLIC_SSL
    std::string ip_and_port_plain;  // POSVASPUBLIC, EMPSPUBLIC
    int pos = 0;
    int ret = -1;

    char  rawResponse[0x1000] = {'\0'};

    merchant.status = 0;

    memcpy(rawResponse, merchantXml, strlen(merchantXml));
    root = ezxml_parse_str(rawResponse, strlen(rawResponse));

    printf("Raw response is >>>> %s\n", rawResponse);

    if (!root) {
        printf("\nError, Please Try Again\n");
        ezxml_free(root);
        return ret;
    }

    tran = ezxml_child(root, "tran");

    merchant.status = atoi(ezxml_child(tran, "status")->txt);
    printf("Status : %d\n", merchant.status);

    if(ezxml_child(tran, "message") != NULL)
       strncpy(itexPosMessage, ezxml_child(tran, "message")->txt, sizeof(itexPosMessage) - 1);

    int len = strlen(itexPosMessage);    
    if (len == 8 && isdigit(itexPosMessage[0])) {
        memset(merchant.tid, '\0', sizeof(merchant.tid));
        strncpy(merchant.tid, itexPosMessage, strlen(itexPosMessage));

    } else {
        ret = saveMerchantData(&merchant);
        gui_messagebox_show("VIOLATION", "This Terminal does not belong to Itex", "", "", 0);
        return -1;
    }    
    
    printf("tid : %s\n", merchant.tid);

    strncpy(merchant.address, ezxml_child(tran, "Address")->txt, sizeof(merchant.address) - 1);
    printf("Address : %s\n", merchant.address);

    strncpy(merchant.rrn, ezxml_child(tran, "RRN")->txt, sizeof(merchant.rrn) - 1);
    printf("rrn : %s\n", merchant.rrn);

    strncpy(merchant.tid, ezxml_child(tran, "message")->txt, sizeof(merchant.tid) - 1);
    printf("rrn : %s\n", merchant.tid);
    
    strncpy(merchant.stamp_label, ezxml_child(tran, "STAMP_LABEL")->txt, sizeof(merchant.stamp_label) - 1);
    printf("Stamp Label : %s\n", merchant.stamp_label);
    
    merchant.stamp_duty_threshold = atoi(ezxml_child(tran, "STAMP_DUTY_THRESHOLD")->txt);
    printf("Stamp threshold : %d\n", merchant.stamp_duty_threshold);

    merchant.stamp_duty = atoi(ezxml_child(tran, "STAMP_DUTY")->txt);
    printf("Stamb duty : %d\n", merchant.stamp_duty);

    strncpy(merchant.port_type, ezxml_child(tran, "PORT_TYPE")->txt, sizeof(merchant.port_type) - 1);
    printf("Port Type : %s\n", merchant.port_type);
    
    strncpy(merchant.nibss_platform, ezxml_child(tran, "PREFIX")->txt, sizeof(merchant.nibss_platform) - 1);
    printf("Platform : %s\n", merchant.nibss_platform);


    if(strcmp(merchant.nibss_platform, "POSVAS") == 0)
    {
        // get POSVAS IP and PORT SSL
        ip_and_port.append(ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);
        printf("ip and port ssl : %s\n", ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);

        ip_and_port_plain.append(ezxml_child(tran, "POSVASPUBLIC")->txt);
        printf("ip and port plain : %s\n", ezxml_child(tran, "POSVASPUBLIC")->txt);


    } else if(strcmp(merchant.nibss_platform, "EPMS") == 0)
    {
        // EPMS IP and PORT SSL
        ip_and_port.append(ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);
        printf("ip and port ssl : %s\n", ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);

        ip_and_port_plain.append(ezxml_child(tran, "EPMSPUBLIC")->txt);
        printf("ip and port plain : %s\n", ezxml_child(tran, "EPMSPUBLIC")->txt);
    }

    pos = ip_and_port.find(';');
    strncpy(merchant.nibss_ip, ip_and_port.substr(0, pos).c_str(), sizeof(merchant.nibss_ip) - 1);
    printf("ip and port : %s\n", merchant.nibss_ip);

    merchant.nibss_port = atoi(ip_and_port.substr(pos + 1, std::string::npos).c_str());
    printf("ssl port is : %d\n", merchant.nibss_port);

    pos = 0;
    pos = ip_and_port_plain.find(';');
    // strncpy(merchant.nibss_ip, ip_and_port_plain.substr(0, pos).c_str(), sizeof(merchant.nibss_ip) - 1); same ip for all
    // printf("ip and port : %s\n", merchant.nibss_ip);

    merchant.nibss_plain_port = atoi(ip_and_port_plain.substr(pos + 1, std::string::npos).c_str());
    printf("plain port is : %d\n", merchant.nibss_plain_port);

    merchant.account_selection = atoi(ezxml_child(tran, "accountSelection")->txt);
    printf("Account Selection : %d\n", merchant.account_selection);

    strncpy(merchant.phone_no, ezxml_child(tran, "phone")->txt, sizeof(merchant.phone_no) - 1);
    printf("Platform : %s\n", merchant.nibss_platform);
    printf("Phone no : %s\n", merchant.phone_no);

    merchant.is_prepped = 0;

    ezxml_free(root);

    
    ret = saveMerchantData(&merchant);

    return ret;
    
}

int readMerchantData(MerchantData* merchant)
{
    // Read json
    // populate the data

    cJSON *json;
    cJSON *jsonAddress, *jsonRrn, *jsonStatus, *jsonTID, *jsonStampLabel, *jsonStampDuty, *jsonStampDutyThreshold;
    cJSON *jsonPlatform, *jsonNibssIp, *jsonNibssPort, *jsonPortType, *jsonPhone, *jsonAccntSelection, *jsonIsPrep, *jsonNibssPlainPort;
   
    char buffer[1024] = {'\0'};
    int ret = -1;

    if(ret = getRecord((void *)buffer, MERCHANT_DETAIL_FILE, sizeof(buffer), 0))
    {
        printf("File read Error\n");
        return ret;
    }

    printf("After read record :\n%s\n", (char *)buffer);

    // if(merchant->status = parseJsonFile(buffer, &json) == -1) return;
    parseJsonFile(buffer, &json);
    
    jsonAddress = cJSON_GetObjectItemCaseSensitive(json, "address");
    jsonRrn = cJSON_GetObjectItemCaseSensitive(json, "rrn");
    jsonTID = cJSON_GetObjectItemCaseSensitive(json, "tid");
    jsonStampLabel = cJSON_GetObjectItemCaseSensitive(json, "stamp_label"); // String
    jsonStampDuty = cJSON_GetObjectItemCaseSensitive(json, "stamp_duty");   // Int
    jsonIsPrep = cJSON_GetObjectItemCaseSensitive(json, "is_prepped");   // Int
    jsonStampDutyThreshold = cJSON_GetObjectItemCaseSensitive(json, "stamp_threshold");    // Int
    jsonPlatform = cJSON_GetObjectItemCaseSensitive(json, "prefix");    // String
    jsonPhone = cJSON_GetObjectItemCaseSensitive(json, "phone_no"); // String
    jsonNibssIp = cJSON_GetObjectItemCaseSensitive(json, "ip");    // String
    jsonNibssPort = cJSON_GetObjectItemCaseSensitive(json, "port");    // Int
    jsonNibssPlainPort = cJSON_GetObjectItemCaseSensitive(json, "plain_port");    // Int
    jsonPortType = cJSON_GetObjectItemCaseSensitive(json, "port_type"); // String
    jsonAccntSelection = cJSON_GetObjectItemCaseSensitive(json, "account_selection"); // Int

    if(cJSON_IsString(jsonAddress))
    {
        strncpy(merchant->address, jsonAddress->valuestring, sizeof(merchant->address) - 1);
        printf("Address : %s\n", merchant->address);
    }

    if(cJSON_IsString(jsonRrn))
    {
        strncpy(merchant->rrn, jsonRrn->valuestring, sizeof(merchant->rrn) - 1);
        printf("rrn : %s\n", merchant->rrn);
    }

    if(cJSON_IsString(jsonTID))
    {
        strncpy(merchant->tid, jsonTID->valuestring, sizeof(merchant->tid) - 1);
        printf("TID : %s\n", merchant->tid);
    }

    if(cJSON_IsString(jsonStampLabel))
    {
        memcpy(merchant->stamp_label, jsonStampLabel->valuestring, sizeof(merchant->stamp_label) - 1);
        printf("Stamp Label : %s\n", merchant->stamp_label);
    }

    if(cJSON_IsNumber(jsonStampDuty))
    {
        merchant->stamp_duty = jsonStampDuty->valueint;
        printf("Stamp duty : %d\n", merchant->stamp_duty);
    }

    if(cJSON_IsNumber(jsonIsPrep))
    {
        merchant->is_prepped = jsonIsPrep->valueint;
        printf("Is prepped : %d\n", merchant->is_prepped);
    }

    if(cJSON_IsNumber(jsonStampDutyThreshold))
    {
        merchant->stamp_duty_threshold = jsonStampDutyThreshold->valueint;
        printf("Stamp duty threshold: %d\n", merchant->stamp_duty_threshold);
    }

    if(cJSON_IsString(jsonPlatform))
    {
        strncpy(merchant->nibss_platform, jsonPlatform->valuestring, sizeof(merchant->nibss_platform) - 1);
        printf("Nibss platform : %s\n", merchant->nibss_platform);
    }

    if(cJSON_IsString(jsonNibssIp))
    {
        strncpy(merchant->nibss_ip, jsonNibssIp->valuestring, sizeof(merchant->nibss_ip) - 1);
        printf("Nibss ip : %s\n", merchant->nibss_ip);
    }

    if(cJSON_IsNumber(jsonNibssPort))
    {
        merchant->nibss_port = jsonNibssPort->valueint;
        printf("Nibss port : %d\n", merchant->nibss_port);
    }

    if(cJSON_IsNumber(jsonNibssPlainPort))
    {
        merchant->nibss_plain_port = jsonNibssPlainPort->valueint;
        printf("Nibss plain port : %d\n", merchant->nibss_plain_port);
    }

    if(cJSON_IsString(jsonPhone))
    {
        strncpy(merchant->phone_no, jsonPhone->valuestring, sizeof(merchant->phone_no) - 1);
        printf("Customer Phone : %s\n", merchant->phone_no);
    }

    if(cJSON_IsString(jsonPortType))
    {
        strncpy(merchant->port_type, jsonPortType->valuestring, sizeof(merchant->port_type) - 1);
        printf("Port Type : %s\n", merchant->port_type);
    }

    if(cJSON_IsNumber(jsonAccntSelection))
    {
        merchant->account_selection = jsonAccntSelection->valueint;
        printf("Account Selections : %d\n", merchant->account_selection);
    }

    cJSON_Delete(json);

    return 0;

}

int saveMerchantData(const MerchantData* merchant)
{
    // json equivalent of merchant

    cJSON *json, *requestJson;
    char *requestJsonStr;
    char jsonData[1024] = {'\0'};
    int ret = -1;

    requestJson = cJSON_CreateObject();

    if (!requestJson) {
        
        printf("Can't create json object\n");
        return ret;
    }

    // save json to file

    cJSON_AddItemToObject(requestJson, "address", cJSON_CreateString(merchant->address));
    cJSON_AddItemToObject(requestJson, "rrn", cJSON_CreateString(merchant->rrn));
    cJSON_AddItemToObject(requestJson, "tid", cJSON_CreateString(merchant->tid));
    cJSON_AddItemToObject(requestJson, "status", cJSON_CreateNumber(merchant->status));
    cJSON_AddItemToObject(requestJson, "stamp_label", cJSON_CreateString(merchant->stamp_label));
    cJSON_AddItemToObject(requestJson, "stamp_threshold", cJSON_CreateNumber(merchant->stamp_duty_threshold));
    cJSON_AddItemToObject(requestJson, "stamp_duty", cJSON_CreateNumber(merchant->stamp_duty));
    cJSON_AddItemToObject(requestJson, "port_type", cJSON_CreateString(merchant->port_type));
    cJSON_AddItemToObject(requestJson, "prefix", cJSON_CreateString(merchant->nibss_platform));
    cJSON_AddItemToObject(requestJson, "ip", cJSON_CreateString(merchant->nibss_ip));
    cJSON_AddItemToObject(requestJson, "port", cJSON_CreateNumber(merchant->nibss_port));
    cJSON_AddItemToObject(requestJson, "plain_port", cJSON_CreateNumber(merchant->nibss_plain_port));
    cJSON_AddItemToObject(requestJson, "phone_no", cJSON_CreateString(merchant->phone_no));
    cJSON_AddItemToObject(requestJson, "is_prepped", cJSON_CreateNumber(merchant->is_prepped));

    cJSON_AddItemToObject(requestJson, "account_selection", cJSON_CreateNumber(merchant->account_selection));


    requestJsonStr = cJSON_PrintUnformatted(requestJson);
    memcpy(jsonData, requestJsonStr, sizeof(jsonData));

    printf("Parameter in json format :\n%s\n", jsonData);

    // 4. Save the json file
    ret = saveRecord((void *)jsonData, MERCHANT_DETAIL_FILE, sizeof(jsonData), 0);

    return ret;
}

int getMerchantData()
{
    NetWorkParameters netParam;
    int ret = -1;
    char responseXml[0x1000] = {'\0'};

    memset(&netParam, 0, sizeof(NetWorkParameters));
    initTamsParameters(&netParam);
    
    if(getMerchantDetails(&netParam, responseXml)) return ret;

    if ((ret = saveMerchantDataXml(responseXml)) != 0) {
        // error
        printf("Error saving merchant data, ret : %d\n", ret);
        return ret;
    }

    return ret;
}




