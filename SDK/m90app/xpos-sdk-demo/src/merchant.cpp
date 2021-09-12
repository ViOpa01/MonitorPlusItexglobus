#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include "appInfo.h"
#include "merchant.h"
#include "EftDbImpl.h"
#include "vas/virtualtid.h"
#include "vas/payvice.h"
#include "vas/vasdb.h"

extern "C" {
#include "log.h"
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

    LOG_PRINTF("request : \n%s", netParam->packet);
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
		LOG_PRINTF("Error Passing json content");
		return ret;
	}

    *parsed = cJSON_Parse(json);

    if (!(*parsed)) {
        LOG_PRINTF("Invalid content");
        return ret;
    }

	status = cJSON_GetObjectItemCaseSensitive(*parsed, "status");

    if(cJSON_IsNumber(status))
    {
        ret = status->valueint;
        // LOG_PRINTF("Parameter status : %d", status->valueint);

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

    if (!root) {
        gui_messagebox_show("ERROR", "Invalid response from Tams", "", "", 3000);
        ezxml_free(root);
        LOG_PRINTF("\nError, Please Try Again");
        return ret;
    }

    tran = ezxml_child(root, "tran");

    if (!tran) {
        gui_messagebox_show("ERROR", "Error parsing response", "", "", 3000);
        ezxml_free(tran);
        LOG_PRINTF("\nError parsing tran, Please Try Again");
        return ret;
    }

    if(ezxml_child(tran, "status")) {
        merchant.status = atoi(ezxml_child(tran, "status")->txt);
        // LOG_PRINTF("Status : %d", merchant.status); 
    }

    if(ezxml_child(tran, "message") != NULL) {
       strncpy(itexPosMessage, ezxml_child(tran, "message")->txt, sizeof(itexPosMessage) - 1);

        int len = strlen(itexPosMessage);    
        if (len == 8 && isdigit(itexPosMessage[0])) {

            readMerchantData(&merchant);
            LOG_PRINTF("Old tid : %s\nNew tid : %s", merchant.old_tid, itexPosMessage);
            if(strncmp(merchant.old_tid, itexPosMessage, 8)) {
                if(clearDb()) {
                    ezxml_free(root);
                    return ret;
                }
            }

            // memset(merchant.tid, '\0', sizeof(merchant.tid));
            memset(&merchant, 0x00, sizeof(MerchantData));
            strncpy(merchant.tid, itexPosMessage, strlen(itexPosMessage));

        } else {
            ret = saveMerchantData(&merchant);
            gui_messagebox_show("VIOLATION", "Invalid TID", "", "", 0);
            ezxml_free(root);
            return -1;
        }    
        
        LOG_PRINTF("tid : %s", merchant.tid);
    } else {
        ret = saveMerchantData(&merchant);
        gui_messagebox_show("VIOLATION", "Terminal is not mapped", "", "", 0);
        ezxml_free(root);
        return -1;
    }

    if(ezxml_child(tran, "Address")) {
        address_name.append(ezxml_child(tran, "Address")->txt);
        // LOG_PRINTF("Address and Name : %s", ezxml_child(tran, "Address")->txt);

        pos = 0;
        pos = address_name.find('|');
        strncpy(merchant.name, address_name.substr(0, pos).c_str(), sizeof(merchant.name) - 1);
        // LOG_PRINTF("Merchant Name : %s", merchant.name);

        if(address_name.substr(pos + 1, std::string::npos).empty())
        {
            strncpy(merchant.address, "", sizeof(merchant.address) - 1);
        } else {
            strncpy(merchant.address, address_name.substr(pos + 1, std::string::npos).c_str(), sizeof(merchant.address) - 1);
        }
        
        // LOG_PRINTF("Address : %s", merchant.address);
    }

    if(ezxml_child(tran, "RRN")) {
        strncpy(merchant.rrn, ezxml_child(tran, "RRN")->txt, sizeof(merchant.rrn) - 1);
        // LOG_PRINTF("rrn : %s", merchant.rrn);
    }

    if(ezxml_child(tran, "message")) {
        strncpy(merchant.tid, ezxml_child(tran, "message")->txt, sizeof(merchant.tid) - 1);
        // LOG_PRINTF("rrn : %s", merchant.tid);
    }
    
    if(ezxml_child(tran, "STAMP_LABEL")) {
        strncpy(merchant.stamp_label, ezxml_child(tran, "STAMP_LABEL")->txt, sizeof(merchant.stamp_label) - 1);
        // LOG_PRINTF("Stamp Label : %s", merchant.stamp_label);
    }
    
    if(ezxml_child(tran, "STAMP_DUTY_THRESHOLD")) {
        merchant.stamp_duty_threshold = atoi(ezxml_child(tran, "STAMP_DUTY_THRESHOLD")->txt);
        // LOG_PRINTF("Stamp threshold : %d", merchant.stamp_duty_threshold);
    }

    if(ezxml_child(tran, "STAMP_DUTY")) {
        merchant.stamp_duty = atoi(ezxml_child(tran, "STAMP_DUTY")->txt);
        // LOG_PRINTF("Stamb duty : %d", merchant.stamp_duty);
    }

    if(ezxml_child(tran, "PORT_TYPE")) {
        strncpy(merchant.port_type, ezxml_child(tran, "PORT_TYPE")->txt, sizeof(merchant.port_type) - 1);
        // LOG_PRINTF("Port Type : %s", merchant.port_type);
    }
    
    if(ezxml_child(tran, "PREFIX")) {
        strncpy(merchant.platform_label, ezxml_child(tran, "PREFIX")->txt, sizeof(merchant.platform_label) - 1);
        // LOG_PRINTF("Platform Label: %s", merchant.platform_label);
    }

    if(ezxml_child(tran, "NOTIFICATION_ID")) {
        strncpy(merchant.notificationIdentifier, ezxml_child(tran, "NOTIFICATION_ID")->txt, sizeof(merchant.notificationIdentifier) - 1);
        // LOG_PRINTF("Notification id: %s", merchant.notificationIdentifier);
    }
    


    if(strcmp(merchant.platform_label, "POSVAS") == 0)
    {
        merchant.nibss_platform = 2;
        // LOG_PRINTF("Platform is : %d", merchant.nibss_platform);
        
        // get POSVAS IP and PORT SSL
        if(ezxml_child(tran, "POSVASPUBLIC_SSL")) {
            ip_and_port.append(ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);
            // LOG_PRINTF("ip and port ssl : %s", ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);
        }

        if(ezxml_child(tran, "POSVASPUBLIC")) {
            ip_and_port_plain.append(ezxml_child(tran, "POSVASPUBLIC")->txt);
            // LOG_PRINTF("ip and port plain : %s", ezxml_child(tran, "POSVASPUBLIC")->txt);
        }

        if(ezxml_child(tran, "CallhomePosvasIp")) {
            strncpy(merchant.callhome_ip, ezxml_child(tran, "CallhomePosvasIp")->txt, sizeof(merchant.callhome_ip) - 1);
            // LOG_PRINTF("Callhome ip : %s", merchant.callhome_ip);
        }

        if(ezxml_child(tran, "CallhomePosvasPort")) {
            merchant.callhome_port = atoi(ezxml_child(tran, "CallhomePosvasPort")->txt);
            // LOG_PRINTF("Callhome port : %d", merchant.callhome_port);
        }

    } else if(strcmp(merchant.platform_label, "EPMS") == 0)
    {
        merchant.nibss_platform = 1;
        // LOG_PRINTF("Platform is : %d", merchant.nibss_platform);

        // EPMS IP and PORT SSL
        if(ezxml_child(tran, "EPMSPUBLIC_SSL")) {
            ip_and_port.append(ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);
            // LOG_PRINTF("ip and port ssl : %s", ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);
        }

        if(ezxml_child(tran, "EPMSPUBLIC")) {
            ip_and_port_plain.append(ezxml_child(tran, "EPMSPUBLIC")->txt);
            // LOG_PRINTF("ip and port plain : %s", ezxml_child(tran, "EPMSPUBLIC")->txt);
        }

        if(ezxml_child(tran, "CallhomeIp")) {
            strncpy(merchant.callhome_ip, ezxml_child(tran, "CallhomeIp")->txt, sizeof(merchant.callhome_ip) - 1);
            // LOG_PRINTF("Callhome ip : %s", merchant.callhome_ip);
        }

        if(ezxml_child(tran, "CallhomePort")) {
            merchant.callhome_port = atoi(ezxml_child(tran, "CallhomePort")->txt);
            // LOG_PRINTF("Callhome port : %d", merchant.callhome_port);
        }

    }

    if(ezxml_child(tran, "CallhomeTime")) {
        merchant.callhome_time = atoi(ezxml_child(tran, "CallhomeTime")->txt);
        // LOG_PRINTF("Callhome time : %d", merchant.callhome_time);
    }

    pos = ip_and_port.find(';');
    strncpy(merchant.nibss_ip, ip_and_port.substr(0, pos).c_str(), sizeof(merchant.nibss_ip) - 1);
    // LOG_PRINTF("ip and port : %s", merchant.nibss_ip);

    merchant.nibss_ssl_port = atoi(ip_and_port.substr(pos + 1, std::string::npos).c_str());
    // LOG_PRINTF("ssl port is : %d", merchant.nibss_ssl_port);

    pos = 0;
    pos = ip_and_port_plain.find(';');
    // strncpy(merchant.nibss_ip, ip_and_port_plain.substr(0, pos).c_str(), sizeof(merchant.nibss_ip) - 1); same ip for all
    // LOG_PRINTF("ip and port : %s", merchant.nibss_ip);

    merchant.nibss_plain_port = atoi(ip_and_port_plain.substr(pos + 1, std::string::npos).c_str());
    // LOG_PRINTF("plain port is : %d", merchant.nibss_plain_port);

    if(ezxml_child(tran, "accountSelection")) {
        merchant.account_selection = atoi(ezxml_child(tran, "accountSelection")->txt);
        // LOG_PRINTF("Account Selection : %d", merchant.account_selection);
    }

    if(ezxml_child(tran, "phone")) {
        strncpy(merchant.phone_no, ezxml_child(tran, "phone")->txt, sizeof(merchant.phone_no) - 1);
        // LOG_PRINTF("Phone no : %s", merchant.phone_no);
    }

    if(ezxml_child(tran, "TERMAPPTYPE")) {
        strncpy(merchant.app_type, ezxml_child(tran, "TERMAPPTYPE")->txt, sizeof(merchant.app_type) - 1);
    }

    merchant.is_prepped = 0;

    ezxml_free(root);

    ret = saveMerchantData(&merchant);

    return ret;
    
}

int readMerchantData(MerchantData* merchant)
{

    cJSON *json, *nextTag;
    char buffer[1024 * 2] = {'\0'};
    int ret = -1;

    if(ret = getRecord((void *)buffer, MERCHANT_DETAIL_FILE, sizeof(buffer), 0))
    {
        LOG_PRINTF("File read Error");
        return ret;
    }

    parseJsonFile(buffer, &json);
    
    nextTag = cJSON_GetObjectItemCaseSensitive(json, "address");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->address, nextTag->valuestring, sizeof(merchant->address) - 1);
        // LOG_PRINTF("Address : %s", merchant->address);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "name");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->name, nextTag->valuestring, sizeof(merchant->name) - 1);
        // LOG_PRINTF("Name : %s", merchant->name);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "rrn");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->rrn, nextTag->valuestring, sizeof(merchant->rrn) - 1);
        // LOG_PRINTF("rrn : %s", merchant->rrn);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "notification_id"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->notificationIdentifier, nextTag->valuestring, sizeof(merchant->notificationIdentifier) - 1);
        // LOG_PRINTF("notification id : %s", merchant->notificationIdentifier);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "tid");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->tid, nextTag->valuestring, sizeof(merchant->tid) - 1);
        // LOG_PRINTF("TID : %s", merchant->tid);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "old_tid");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->old_tid, nextTag->valuestring, sizeof(merchant->old_tid) - 1);
        // LOG_PRINTF("OLD TID : %s", merchant->old_tid);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "stamp_label"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        memcpy(merchant->stamp_label, nextTag->valuestring, sizeof(merchant->stamp_label) - 1);
        // LOG_PRINTF("Stamp Label : %s", merchant->stamp_label);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "platform_label"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        memcpy(merchant->platform_label, nextTag->valuestring, sizeof(merchant->platform_label) - 1);
        // LOG_PRINTF("Platform Label : %s", merchant->platform_label);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "stamp_duty");   // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->stamp_duty = nextTag->valueint;
        // LOG_PRINTF("Stamp duty : %d", merchant->stamp_duty);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "is_prepped");   // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->is_prepped = nextTag->valueint;
        // LOG_PRINTF("Is prepped : %d", merchant->is_prepped);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "stamp_threshold");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->stamp_duty_threshold = nextTag->valueint;
        // LOG_PRINTF("Stamp duty threshold: %d", merchant->stamp_duty_threshold);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "platform");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->nibss_platform = nextTag->valueint;
        // LOG_PRINTF("Nibss platform : %d", merchant->nibss_platform);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "ip");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->nibss_ip, nextTag->valuestring, sizeof(merchant->nibss_ip) - 1);
        // LOG_PRINTF("Nibss ip : %s", merchant->nibss_ip);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "port");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->nibss_ssl_port = nextTag->valueint;
        // LOG_PRINTF("Nibss port : %d", merchant->nibss_ssl_port);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "plain_port");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->nibss_plain_port = nextTag->valueint;
        // LOG_PRINTF("Nibss plain port : %d", merchant->nibss_plain_port);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "phone_no"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->phone_no, nextTag->valuestring, sizeof(merchant->phone_no) - 1);
        // LOG_PRINTF("Customer Phone : %s", merchant->phone_no);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "port_type"); // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->port_type, nextTag->valuestring, sizeof(merchant->port_type) - 1);
        // LOG_PRINTF("Port Type : %s", merchant->port_type);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "account_selection"); // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->account_selection = nextTag->valueint;
        // LOG_PRINTF("Account Selections : %d", merchant->account_selection);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "trans_type");
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->trans_type = nextTag->valueint;
        // LOG_PRINTF("Trans Type : %d", merchant->trans_type);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "plain_key");
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->pKey, nextTag->valuestring, sizeof(merchant->pKey) - 1);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "callhome_ip");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->callhome_ip, nextTag->valuestring, sizeof(merchant->callhome_ip) - 1);
        // LOG_PRINTF("Callhome ip : %s", merchant->callhome_ip);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "callhome_port");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->callhome_port = nextTag->valueint;
        // LOG_PRINTF("Callhome port : %d", merchant->callhome_port);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "callhome_time");    // Int
    if(cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->callhome_time = nextTag->valueint;
        // LOG_PRINTF("Callhome time : %d", merchant->callhome_time);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "app_type");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->app_type, nextTag->valuestring, sizeof(merchant->app_type) - 1);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "sim_operator_name");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->gprsSettings.operatorName, nextTag->valuestring, sizeof(merchant->gprsSettings.operatorName) - 1);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "sim_apn");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->gprsSettings.apn, nextTag->valuestring, sizeof(merchant->gprsSettings.apn) - 1);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "sim_user");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->gprsSettings.username, nextTag->valuestring, sizeof(merchant->gprsSettings.username) - 1);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "sim_pwd");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        strncpy(merchant->gprsSettings.password, nextTag->valuestring, sizeof(merchant->gprsSettings.password) - 1);
    }

    nextTag = cJSON_GetObjectItemCaseSensitive(json, "sim_timeout");    // String
    if(cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag))
    {
        merchant->gprsSettings.timeout = nextTag->valueint;
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
        
        LOG_PRINTF("Can't create json object");
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

    cJSON_AddItemToObject(requestJson, "callhome_ip", cJSON_CreateString(merchant->callhome_ip));
    cJSON_AddItemToObject(requestJson, "callhome_port", cJSON_CreateNumber(merchant->callhome_port));
    cJSON_AddItemToObject(requestJson, "callhome_time", cJSON_CreateNumber(merchant->callhome_time));
    cJSON_AddItemToObject(requestJson, "app_type", cJSON_CreateString(merchant->app_type));

    cJSON_AddItemToObject(requestJson, "sim_operator_name", cJSON_CreateString(merchant->gprsSettings.operatorName));
    cJSON_AddItemToObject(requestJson, "sim_apn", cJSON_CreateString(merchant->gprsSettings.apn));
    cJSON_AddItemToObject(requestJson, "sim_user", cJSON_CreateString(merchant->gprsSettings.username));
    cJSON_AddItemToObject(requestJson, "sim_pwd", cJSON_CreateString(merchant->gprsSettings.password));
    cJSON_AddItemToObject(requestJson, "sim_timeout", cJSON_CreateNumber(merchant->gprsSettings.timeout));

    requestJsonStr = cJSON_PrintUnformatted(requestJson);
    memcpy(jsonData, requestJsonStr, sizeof(jsonData));
    free(requestJsonStr);
    cJSON_Delete(requestJson);

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
        LOG_PRINTF("Error saving merchant data, ret : %d", ret);
        return ret;
    }

    Payvice::resetApiToken();
    resetVirtualConfiguration();

    return ret;
}

int logoutAndResetVasDb()
{
    Payvice payvice;
    if (logOut(payvice) == 0) {
        remove(VASDB_FILE);
        initVasTables();
    }

    return 0;
}




