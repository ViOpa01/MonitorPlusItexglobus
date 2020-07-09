//#include "libapi_xpos/inc/libapi_file.h"
//#include "libapi_xpos/inc/libapi_gui.h"



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include "appInfo.h"
#include "merchant.h"
#include "EftDbImpl.h"
#include "vas/virtualtid.h"

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
    netParam->isHttp = 1;
    netParam->isSsl = 0;
}

static short getMerchantDetails(NetWorkParameters * netParam, char *buffer, const int size)
{

    char terminalSn[22] = {0};
    char path[0x500] = {'\0'};
   
    strncpy((char *)netParam->host, ITEX_TAMS_PUBLIC_IP, strlen(ITEX_TAMS_PUBLIC_IP));
    netParam->port = atoi(ITEX_TASM_PUBLIC_PORT);
    netParam->endTag = "</efttran>";

    getTerminalSn(terminalSn);

    sprintf(path, "tams/eftpos/devinterface/transactionadvice.php?action=TAMS_WEBAPI&termID=%s&posUID=%s&ver=%s%s&model=%s&control=TamsSecurity", DEFAULT_TID, terminalSn, APP_NAME, APP_VER, APP_MODEL);
    
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "GET /%s\r\n", path);
 	netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Host: %s:%d", netParam->host, netParam->port);
	netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "%s", "\r\n\r\n");

    printf("request : \n%s\n", netParam->packet);
    if (sendAndRecvPacket(netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -1;
    }

    memcpy(buffer, netParam->response, size);
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

static int saveMerchantDataXml(const char* merchantXml, const int size)
{

    ezxml_t root;
    ezxml_t tran;

    char itexPosMessage[14] = {0};
    MerchantData merchant = { 0 };
    std::string ip_and_port;    // POSVASPUBLIC_SSL, EMPSPUBLIC_SSL
    std::string ip_and_port_plain;  // POSVASPUBLIC, EMPSPUBLIC
    std::string address_name;
    int pos = 0;
    int ret = -1;

    char  rawResponse[0x1000] = {'\0'};

    merchant.status = 0;

    memcpy(rawResponse, merchantXml, size);
    root = ezxml_parse_str(rawResponse, strlen(rawResponse));

    printf("Raw response is >>>> %s\n", rawResponse);

    if (!root) {
        gui_messagebox_show("ERROR", "Invalid response from Tams", "", "", 3000);
        ezxml_free(root);
        printf("\nError, Please Try Again\n");
        return ret;
    }

    tran = ezxml_child(root, "tran");

    if (!tran) {
        gui_messagebox_show("ERROR", "Error parsing response", "", "", 3000);
        ezxml_free(tran);
        printf("\nError parsing tran, Please Try Again\n");
        return ret;
    }

    merchant.status = atoi(ezxml_child(tran, "status")->txt);
    // printf("Status : %d\n", merchant.status);

    if(ezxml_child(tran, "message") != NULL)
       strncpy(itexPosMessage, ezxml_child(tran, "message")->txt, sizeof(itexPosMessage) - 1);

    int len = strlen(itexPosMessage);    
    if (len == 8 && isdigit(itexPosMessage[0])) {

        readMerchantData(&merchant);
        printf("Old tid : %s\nNew tid : %s\n", merchant.old_tid, itexPosMessage);
        if(strncmp(merchant.old_tid, itexPosMessage, 8)) {
            if(clearDb()) {
                ezxml_free(root);
                return ret;
            }
        }

        memset(merchant.tid, '\0', sizeof(merchant.tid));
        strncpy(merchant.tid, itexPosMessage, strlen(itexPosMessage));

    } else {
        ret = saveMerchantData(&merchant);
        gui_messagebox_show("VIOLATION", "Terminal is not mapped", "", "", 0);
        ezxml_free(root);
        return -1;
    }    
    
    printf("tid : %s\n", merchant.tid);

    address_name.append(ezxml_child(tran, "Address")->txt);
    // printf("Address and Name : %s\n", ezxml_child(tran, "Address")->txt);

    pos = 0;
    pos = address_name.find('|');
    strncpy(merchant.name, address_name.substr(0, pos).c_str(), sizeof(merchant.name) - 1);
    // printf("Merchant Name : %s\n", merchant.name);

    if(address_name.substr(pos + 1, std::string::npos).empty())
    {
        strncpy(merchant.address, "", sizeof(merchant.address) - 1);
    } else {
        strncpy(merchant.address, address_name.substr(pos + 1, std::string::npos).c_str(), sizeof(merchant.address) - 1);
    }
    
    // printf("Address : %s\n", merchant.address);

    strncpy(merchant.rrn, ezxml_child(tran, "RRN")->txt, sizeof(merchant.rrn) - 1);
    // printf("rrn : %s\n", merchant.rrn);

    strncpy(merchant.tid, ezxml_child(tran, "message")->txt, sizeof(merchant.tid) - 1);
    // printf("rrn : %s\n", merchant.tid);
    
    strncpy(merchant.stamp_label, ezxml_child(tran, "STAMP_LABEL")->txt, sizeof(merchant.stamp_label) - 1);
    // printf("Stamp Label : %s\n", merchant.stamp_label);
    
    merchant.stamp_duty_threshold = atoi(ezxml_child(tran, "STAMP_DUTY_THRESHOLD")->txt);
    // printf("Stamp threshold : %d\n", merchant.stamp_duty_threshold);

    merchant.stamp_duty = atoi(ezxml_child(tran, "STAMP_DUTY")->txt);
    // printf("Stamb duty : %d\n", merchant.stamp_duty);

    strncpy(merchant.port_type, ezxml_child(tran, "PORT_TYPE")->txt, sizeof(merchant.port_type) - 1);
    // printf("Port Type : %s\n", merchant.port_type);
    
    strncpy(merchant.platform_label, ezxml_child(tran, "PREFIX")->txt, sizeof(merchant.platform_label) - 1);
    // printf("Platform Label: %s\n", merchant.platform_label);

    if(ezxml_child(tran, "NOTIFICATION_ID")) {
        strncpy(merchant.notificationIdentifier, ezxml_child(tran, "NOTIFICATION_ID")->txt, sizeof(merchant.notificationIdentifier) - 1);
        // printf("Notification id: %s\n", merchant.notificationIdentifier);
    }
    


    if(strcmp(merchant.platform_label, "POSVAS") == 0)
    {
        merchant.nibss_platform = 2;
        // printf("Platform is : %d\n", merchant.nibss_platform);
        
        // get POSVAS IP and PORT SSL
        ip_and_port.append(ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);
        // printf("ip and port ssl : %s\n", ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);

        ip_and_port_plain.append(ezxml_child(tran, "POSVASPUBLIC")->txt);
        // printf("ip and port plain : %s\n", ezxml_child(tran, "POSVASPUBLIC")->txt);


    } else if(strcmp(merchant.platform_label, "EPMS") == 0)
    {
        merchant.nibss_platform = 1;
        // printf("Platform is : %d\n", merchant.nibss_platform);

        // EPMS IP and PORT SSL
        ip_and_port.append(ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);
        // printf("ip and port ssl : %s\n", ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);

        ip_and_port_plain.append(ezxml_child(tran, "EPMSPUBLIC")->txt);
        // printf("ip and port plain : %s\n", ezxml_child(tran, "EPMSPUBLIC")->txt);
    }

    pos = ip_and_port.find(';');
    strncpy(merchant.nibss_ip, ip_and_port.substr(0, pos).c_str(), sizeof(merchant.nibss_ip) - 1);
    // printf("ip and port : %s\n", merchant.nibss_ip);

    merchant.nibss_ssl_port = atoi(ip_and_port.substr(pos + 1, std::string::npos).c_str());
    // printf("ssl port is : %d\n", merchant.nibss_ssl_port);

    pos = 0;
    pos = ip_and_port_plain.find(';');
    // strncpy(merchant.nibss_ip, ip_and_port_plain.substr(0, pos).c_str(), sizeof(merchant.nibss_ip) - 1); same ip for all
    // printf("ip and port : %s\n", merchant.nibss_ip);

    merchant.nibss_plain_port = atoi(ip_and_port_plain.substr(pos + 1, std::string::npos).c_str());
    // printf("plain port is : %d\n", merchant.nibss_plain_port);

    merchant.account_selection = atoi(ezxml_child(tran, "accountSelection")->txt);
    // printf("Account Selection : %d\n", merchant.account_selection);

    strncpy(merchant.phone_no, ezxml_child(tran, "phone")->txt, sizeof(merchant.phone_no) - 1);
    // printf("Platform : %d\n", merchant.nibss_platform);
    // printf("Phone no : %s\n", merchant.phone_no);

    merchant.is_prepped = 0;

    ezxml_free(root);

    ret = saveMerchantData(&merchant);

    return ret;
    
}

int readMerchantData(MerchantData* merchant)
{

    cJSON *json, *nextTag;
    char buffer[1024] = {'\0'};
    int ret = -1;

    if(ret = getRecord((void *)buffer, MERCHANT_DETAIL_FILE, sizeof(buffer), 0))
    {
        printf("File read Error\n");
        return ret;
    }

    parseJsonFile(buffer, &json);
    
    nextTag = cJSON_GetObjectItemCaseSensitive(json, "address");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->address, nextTag->valuestring, sizeof(merchant->address) - 1);
        // printf("Address : %s\n", merchant->address);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "name");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->name, nextTag->valuestring, sizeof(merchant->name) - 1);
        // printf("Name : %s\n", merchant->name);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "rrn");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->rrn, nextTag->valuestring, sizeof(merchant->rrn) - 1);
        // printf("rrn : %s\n", merchant->rrn);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "notification_id"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->notificationIdentifier, nextTag->valuestring, sizeof(merchant->notificationIdentifier) - 1);
        // printf("notification id : %s\n", merchant->notificationIdentifier);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "tid");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->tid, nextTag->valuestring, sizeof(merchant->tid) - 1);
        // printf("TID : %s\n", merchant->tid);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "old_tid");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->old_tid, nextTag->valuestring, sizeof(merchant->old_tid) - 1);
        // printf("OLD TID : %s\n", merchant->old_tid);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "stamp_label"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        memcpy(merchant->stamp_label, nextTag->valuestring, sizeof(merchant->stamp_label) - 1);
        // printf("Stamp Label : %s\n", merchant->stamp_label);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "platform_label"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        memcpy(merchant->platform_label, nextTag->valuestring, sizeof(merchant->platform_label) - 1);
        // printf("Platform Label : %s\n", merchant->platform_label);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "stamp_duty");   // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->stamp_duty = nextTag->valueint;
        // printf("Stamp duty : %d\n", merchant->stamp_duty);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "is_prepped");   // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->is_prepped = nextTag->valueint;
        // printf("Is prepped : %d\n", merchant->is_prepped);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "stamp_threshold");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->stamp_duty_threshold = nextTag->valueint;
        // printf("Stamp duty threshold: %d\n", merchant->stamp_duty_threshold);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "platform");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->nibss_platform = nextTag->valueint;
        // printf("Nibss platform : %d\n", merchant->nibss_platform);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "ip");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->nibss_ip, nextTag->valuestring, sizeof(merchant->nibss_ip) - 1);
        // printf("Nibss ip : %s\n", merchant->nibss_ip);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "port");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->nibss_ssl_port = nextTag->valueint;
        // printf("Nibss port : %d\n", merchant->nibss_ssl_port);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "plain_port");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->nibss_plain_port = nextTag->valueint;
        // printf("Nibss plain port : %d\n", merchant->nibss_plain_port);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "phone_no"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->phone_no, nextTag->valuestring, sizeof(merchant->phone_no) - 1);
        // printf("Customer Phone : %s\n", merchant->phone_no);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "port_type"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->port_type, nextTag->valuestring, sizeof(merchant->port_type) - 1);
        // printf("Port Type : %s\n", merchant->port_type);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "account_selection"); // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->account_selection = nextTag->valueint;
        // printf("Account Selections : %d\n", merchant->account_selection);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "trans_type");
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->trans_type = nextTag->valueint;
        // printf("Trans Type : %d\n", merchant->trans_type);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "plain_key");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->pKey, nextTag->valuestring, sizeof(merchant->pKey) - 1);
    }

    cJSON_Delete(json);

    return 0;

}

int saveMerchantData(const MerchantData* merchant)
{
    // json equivalent of merchant

    cJSON *requestJson;
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
    cJSON_AddItemToObject(requestJson, "name", cJSON_CreateString(merchant->name));
    cJSON_AddItemToObject(requestJson, "rrn", cJSON_CreateString(merchant->rrn));
    cJSON_AddItemToObject(requestJson, "notification_id", cJSON_CreateString(merchant->notificationIdentifier));
    cJSON_AddItemToObject(requestJson, "tid", cJSON_CreateString(merchant->tid));
    cJSON_AddItemToObject(requestJson, "old_tid", cJSON_CreateString(merchant->old_tid));
    cJSON_AddItemToObject(requestJson, "status", cJSON_CreateNumber(merchant->status));
    cJSON_AddItemToObject(requestJson, "stamp_label", cJSON_CreateString(merchant->stamp_label));
    cJSON_AddItemToObject(requestJson, "platform_label", cJSON_CreateString(merchant->platform_label));
    cJSON_AddItemToObject(requestJson, "stamp_threshold", cJSON_CreateNumber(merchant->stamp_duty_threshold));
    cJSON_AddItemToObject(requestJson, "stamp_duty", cJSON_CreateNumber(merchant->stamp_duty));
    cJSON_AddItemToObject(requestJson, "port_type", cJSON_CreateString(merchant->port_type));
    cJSON_AddItemToObject(requestJson, "platform", cJSON_CreateNumber(merchant->nibss_platform));
    cJSON_AddItemToObject(requestJson, "ip", cJSON_CreateString(merchant->nibss_ip));
    cJSON_AddItemToObject(requestJson, "port", cJSON_CreateNumber(merchant->nibss_ssl_port));
    cJSON_AddItemToObject(requestJson, "plain_port", cJSON_CreateNumber(merchant->nibss_plain_port));
    cJSON_AddItemToObject(requestJson, "phone_no", cJSON_CreateString(merchant->phone_no));
    cJSON_AddItemToObject(requestJson, "is_prepped", cJSON_CreateNumber(merchant->is_prepped));

    cJSON_AddItemToObject(requestJson, "account_selection", cJSON_CreateNumber(merchant->account_selection));
    cJSON_AddItemToObject(requestJson, "trans_type", cJSON_CreateNumber(merchant->trans_type));

    cJSON_AddItemToObject(requestJson, "plain_key", cJSON_CreateString(merchant->pKey));




    requestJsonStr = cJSON_PrintUnformatted(requestJson);
    memcpy(jsonData, requestJsonStr, sizeof(jsonData));

    ret = saveRecord((void *)jsonData, MERCHANT_DETAIL_FILE, sizeof(jsonData), 0);

    return ret;
}

int getMerchantData()
{
    NetWorkParameters netParam;
    MerchantData mParam = {'\0'};
    int ret = -1;
    char responseXml[0x1000 * 2] = {'\0'};

    memset(&netParam, 0, sizeof(NetWorkParameters));
    initTamsParameters(&netParam);
    
    if(getMerchantDetails(&netParam, responseXml, sizeof(responseXml) - 1)) return ret;

    // Getting a copy of tid before prepping
    readMerchantData(&mParam);
    if(mParam.tid[0]) {
        strncpy(mParam.old_tid, mParam.tid, 8);
        saveMerchantData(&mParam);
    }

    if ((ret = saveMerchantDataXml(responseXml, sizeof(responseXml) - 1)) != 0) {
        printf("Error saving merchant data, ret : %d\n", ret);
        return ret;
    }

    resetVirtualConfiguration();

    return ret;
}




