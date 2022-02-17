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

static int getIntegerFrom_EzXml(ezxml_t xml, const char* tag)
{
    ezxml_t element = ezxml_child(xml, tag);
    if (element) {
        printf("Ezxml:::%s::::: %s :::%s\n", __FUNCTION__, tag, element->txt);
        return atoi(element->txt);
    }
    return 0;
}

static void copyStringFrom_EzXml(
    char* buff, size_t bufLen, ezxml_t xml, const char* tag)
{
    ezxml_t element = ezxml_child(xml, tag);
    if (element) {
        printf("Ezxml:::%s::::: %s :::%s\n", __FUNCTION__, tag, element->txt);
        strncpy(buff, element->txt, bufLen - 1);
    }
}

static short isValidTid(const char* tid)
{
    size_t len = strlen(tid);
    return len == 8 && isdigit(tid[0]);
}

static int isNewTid(const char* oldTid, const char* newTid)
{
    return strncmp(oldTid, newTid, 8);
}

static short handleItexPosMerchantMessage(
    const char* itexPosMessage, MerchantData* merchant)
{
    if (!itexPosMessage[0] || !isValidTid(itexPosMessage)) {
        saveMerchantData(merchant);
        gui_messagebox_show("VIOLATION", "Terminal is not mapped", "", "", 0);
        return -1;
    }
    readMerchantData(merchant);
    LOG_PRINTF("Old tid : %s\nNew tid : %s", merchant->old_tid, itexPosMessage);
    if (isNewTid(merchant->old_tid, itexPosMessage)) {
        if (clearDb()) {
            return -1;
        }
    }
    memset(merchant, 0x00, sizeof(MerchantData));
    strncpy(merchant->tid, itexPosMessage, strlen(itexPosMessage));

    LOG_PRINTF("tid : %s", merchant->tid);
    return 0;
}

static void handleMerchantAddress(ezxml_t xml, MerchantData* merchant)
{
    ezxml_t address = ezxml_child(xml, "Address");
    if (!address) {
        return;
    }
    std::string address_name;
    address_name.append(address->txt);

    size_t pos = address_name.find('|');
    strncpy(merchant->name, address_name.substr(0, pos).c_str(),
        sizeof(merchant->name) - 1);

    if (address_name.substr(pos + 1, std::string::npos).empty()) {
        strncpy(merchant->address, "", sizeof(merchant->address) - 1);
    } else {
        strncpy(merchant->address,
            address_name.substr(pos + 1, std::string::npos).c_str(),
            sizeof(merchant->address) - 1);
    }
}

static void handlePosvasIpAndPort(std::string& ip_and_port,
    std::string& ip_and_port_plain, ezxml_t xml, MerchantData* merchant)
{
    merchant->nibss_platform = 2;

    if (ezxml_child(xml, "POSVASPUBLIC_SSL")) {
        ip_and_port.append(ezxml_child(xml, "POSVASPUBLIC_SSL")->txt);
    }

    if (ezxml_child(xml, "POSVASPUBLIC")) {
        ip_and_port_plain.append(ezxml_child(xml, "POSVASPUBLIC")->txt);
    }

    copyStringFrom_EzXml(merchant->callhome_ip, sizeof(merchant->callhome_ip),
        xml, "CallhomePosvasIp");
    merchant->callhome_port = getIntegerFrom_EzXml(xml, "CallhomePosvasPort");
}

static void handleEpmsIpAndPort(std::string& ip_and_port,
    std::string& ip_and_port_plain, ezxml_t xml, MerchantData* merchant)
{
    merchant->nibss_platform = 1;

    if (ezxml_child(xml, "EPMSPUBLIC_SSL")) {
        ip_and_port.append(ezxml_child(xml, "EPMSPUBLIC_SSL")->txt);
    }

    if (ezxml_child(xml, "EPMSPUBLIC")) {
        ip_and_port_plain.append(ezxml_child(xml, "EPMSPUBLIC")->txt);
    }

    copyStringFrom_EzXml(merchant->callhome_ip, sizeof(merchant->callhome_ip),
        xml, "CallhomeIp");
    merchant->callhome_port = getIntegerFrom_EzXml(xml, "CallhomePort");
}

static void handlePlatformIpAndPort(ezxml_t xml, MerchantData* merchant)
{
    std::string ip_and_port; // POSVASPUBLIC_SSL, EMPSPUBLIC_SSL
    std::string ip_and_port_plain; // POSVASPUBLIC, EMPSPUBLIC

    if (strcmp(merchant->platform_label, "POSVAS") == 0) {
        handlePosvasIpAndPort(ip_and_port, ip_and_port_plain, xml, merchant);
    } else if (strcmp(merchant->platform_label, "EPMS") == 0) {
        handleEpmsIpAndPort(ip_and_port, ip_and_port_plain, xml, merchant);
    }
    size_t pos = ip_and_port.find(';');
    strncpy(merchant->nibss_ip, ip_and_port.substr(0, pos).c_str(),
        sizeof(merchant->nibss_ip) - 1);

    merchant->nibss_ssl_port
        = atoi(ip_and_port.substr(pos + 1, std::string::npos).c_str());

    pos = 0;
    pos = ip_and_port_plain.find(';');

    merchant->nibss_plain_port
        = atoi(ip_and_port_plain.substr(pos + 1, std::string::npos).c_str());
}

static void copyMerchantDataFromXml(ezxml_t xml, MerchantData* merchant)
{
    char itexPosMessage[14] = { 0 };
    copyStringFrom_EzXml(
        itexPosMessage, sizeof(itexPosMessage), xml, "message");
    if (handleItexPosMerchantMessage(itexPosMessage, merchant) != 0) {
        return;
    }

    copyStringFrom_EzXml(merchant->rrn, sizeof(merchant->rrn), xml, "RRN");
    copyStringFrom_EzXml(
        merchant->vasUrl, sizeof(merchant->vasUrl), xml, "VASURL");
    copyStringFrom_EzXml(
        merchant->phone_no, sizeof(merchant->phone_no), xml, "phone");
    copyStringFrom_EzXml(
        merchant->port_type, sizeof(merchant->port_type), xml, "PORT_TYPE");
    copyStringFrom_EzXml(
        merchant->app_type, sizeof(merchant->app_type), xml, "TERMAPPTYPE");
    copyStringFrom_EzXml(merchant->stamp_label, sizeof(merchant->stamp_label),
        xml, "STAMP_LABEL");
    copyStringFrom_EzXml(merchant->platform_label,
        sizeof(merchant->platform_label), xml, "PREFIX");
    copyStringFrom_EzXml(merchant->notificationIdentifier,
        sizeof(merchant->notificationIdentifier), xml, "NOTIFICATION_ID");

    merchant->status = getIntegerFrom_EzXml(xml, "status");
    merchant->stamp_duty = getIntegerFrom_EzXml(xml, "STAMP_DUTY");
    merchant->callhome_time = getIntegerFrom_EzXml(xml, "CallhomeTime");
    merchant->account_selection = getIntegerFrom_EzXml(xml, "accountSelection");
    merchant->stamp_duty_threshold
        = getIntegerFrom_EzXml(xml, "STAMP_DUTY_THRESHOLD");

    handlePlatformIpAndPort(xml, merchant);
    handleMerchantAddress(xml, merchant);
}


static int saveMerchantDataXml(const char* merchantXml, const int size)
{

    ezxml_t root;
    ezxml_t tran;

    MerchantData merchant = { 0 };
    int ret = -1;

    char rawResponse[0x1000] = { '\0' };

    merchant.status = 0;

    memcpy(rawResponse, merchantXml, size);
    root = ezxml_parse_str(rawResponse, strlen(rawResponse));

    if (!root) {
        gui_messagebox_show(
            "ERROR", "Invalid response from Tams", "", "", 3000);
        ezxml_free(root);
        LOG_PRINTF("\nError, Please Try Again");
        return ret;
    }

    tran = ezxml_child(root, "tran");

    if (!tran) {
        gui_messagebox_show("ERROR", "Error parsing response", "", "", 3000);
        ezxml_free(root);
        LOG_PRINTF("\nError parsing tran, Please Try Again");
        return ret;
    }

    copyMerchantDataFromXml(tran, &merchant);

    merchant.is_prepped = 0;

    ezxml_free(root);

    ret = saveMerchantData(&merchant);

    return ret;
}

static void copyStringFrom_cJSON(
    char* buff, size_t buffLen, const char* key, const cJSON* json)
{
    cJSON* nextTag = cJSON_GetObjectItemCaseSensitive(json, key);
    if (cJSON_IsString(nextTag) && !cJSON_IsNull(nextTag)) {
        printf("JSON:::%s::::: %s :::%s\n", __FUNCTION__, key, nextTag->valuestring);
        strncpy(buff, nextTag->valuestring, buffLen - 1);
    }
}

static int getIntegerFrom_cJSON(const cJSON* json, const char* key)
{
    cJSON* nextTag = cJSON_GetObjectItemCaseSensitive(json, key);
    if (cJSON_IsNumber(nextTag) && !cJSON_IsNull(nextTag)) {
        printf("JSON:::%s::::: %s :::%d\n", __FUNCTION__, key, nextTag->valueint);
        return nextTag->valueint;
    }
    return 0;
}

int readMerchantData(MerchantData* merchant)
{

    cJSON* json = NULL;
    char buffer[1024 * 2] = { '\0' };
    int ret = -1;

    if ((ret = getRecord(
             (void*)buffer, MERCHANT_DETAIL_FILE, sizeof(buffer), 0))) {
        LOG_PRINTF("File read Error");
        return ret;
    }

    parseJsonString(buffer, &json);

    copyStringFrom_cJSON(merchant->rrn, sizeof(merchant->rrn), "rrn", json);
    copyStringFrom_cJSON(merchant->tid, sizeof(merchant->tid), "tid", json);
    copyStringFrom_cJSON(merchant->name, sizeof(merchant->name), "name", json);
    copyStringFrom_cJSON(
        merchant->address, sizeof(merchant->address), "address", json);
    copyStringFrom_cJSON(merchant->notificationIdentifier,
        sizeof(merchant->notificationIdentifier), "notification_id", json);
    copyStringFrom_cJSON(
        merchant->vasUrl, sizeof(merchant->vasUrl), "vas_url", json);
    copyStringFrom_cJSON(
        merchant->old_tid, sizeof(merchant->old_tid), "old_tid", json);
    copyStringFrom_cJSON(merchant->stamp_label, sizeof(merchant->stamp_label),
        "stamp_label", json);
    copyStringFrom_cJSON(merchant->platform_label,
        sizeof(merchant->platform_label), "platform_label", json);
    copyStringFrom_cJSON(
        merchant->nibss_ip, sizeof(merchant->nibss_ip), "ip", json);
    copyStringFrom_cJSON(
        merchant->phone_no, sizeof(merchant->phone_no), "phone_no", json);
    copyStringFrom_cJSON(
        merchant->port_type, sizeof(merchant->port_type), "port_type", json);
    copyStringFrom_cJSON(
        merchant->pKey, sizeof(merchant->pKey), "plain_key", json);
    copyStringFrom_cJSON(merchant->callhome_ip, sizeof(merchant->callhome_ip),
        "callhome_ip", json);
    copyStringFrom_cJSON(
        merchant->app_type, sizeof(merchant->app_type), "app_type", json);
    copyStringFrom_cJSON(merchant->gprsSettings.operatorName,
        sizeof(merchant->gprsSettings.operatorName), "sim_operator_name", json);
    copyStringFrom_cJSON(merchant->gprsSettings.apn,
        sizeof(merchant->gprsSettings.apn), "sim_apn", json);
    copyStringFrom_cJSON(merchant->gprsSettings.username,
        sizeof(merchant->gprsSettings.username), "sim_user", json);
    copyStringFrom_cJSON(merchant->gprsSettings.password,
        sizeof(merchant->gprsSettings.password), "sim_pwd", json);

    merchant->nibss_ssl_port = getIntegerFrom_cJSON(json, "port");
    merchant->stamp_duty = getIntegerFrom_cJSON(json, "stamp_duty");
    merchant->trans_type = getIntegerFrom_cJSON(json, "trans_type");
    merchant->is_prepped = getIntegerFrom_cJSON(json, "is_prepped");
    merchant->nibss_platform = getIntegerFrom_cJSON(json, "platform");
    merchant->nibss_plain_port = getIntegerFrom_cJSON(json, "plain_port");
    merchant->callhome_port = getIntegerFrom_cJSON(json, "callhome_port");
    merchant->callhome_time = getIntegerFrom_cJSON(json, "callhome_time");
    merchant->gprsSettings.timeout = getIntegerFrom_cJSON(json, "sim_timeout");
    merchant->stamp_duty_threshold
        = getIntegerFrom_cJSON(json, "stamp_threshold");
    merchant->account_selection
        = getIntegerFrom_cJSON(json, "account_selection");

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
    cJSON_AddItemToObject(requestJson, "vas_url", cJSON_CreateString(merchant->vasUrl));

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
