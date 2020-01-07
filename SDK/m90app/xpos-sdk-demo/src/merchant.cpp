#include "libapi_xpos/inc/libapi_file.h"

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
}

#define ITEX_TAMS_PUBLIC_IP "basehuge.itexapp.com"
#define ITEX_TASM_PUBLIC_PORT "80"
#define DEFAULT_TID "2070AS89"

#define MERCHANT_DETAIL_FILE    "merchant.json"

using namespace std;

void writeMyName()
{
    std::string name = "Jeremiah Olatunde";
    printf("%s\n", name.c_str());

}


static void getMerchantDetails(char *buffer)
{
    NetWorkParameters netParam = {0};
    char terminalSn[22] = {0};

    char path[0x500] = {'\0'};
    string add;


    strncpy((char *)netParam.host, ITEX_TAMS_PUBLIC_IP, strlen(ITEX_TAMS_PUBLIC_IP));
    netParam.port = atoi(ITEX_TASM_PUBLIC_PORT);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.packetSize = 0;
    getTerminalSn(terminalSn);

    sprintf(path, "tams/eftpos/devinterface/transactionadvice.php?action=TAMS_WEBAPI&termID=%s&posUID=%s&ver=%s%s&model=%s&control=TamsSecurity", DEFAULT_TID, "346231236" /*terminalSn*/, APP_NAME, APP_VER, APP_MODEL);
    
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "GET /%s\r\n", path);
 	netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d", netParam.host, netParam.port);
	netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "%s", "\r\n\r\n");

    printf("request : \n%s\n", netParam.packet);
    sendAndRecvDataSsl(&netParam);

    memcpy(buffer, netParam.response, strlen((char *)netParam.response));
    
}


class MerchantParameterData {

    std::string address;
    std::string rrn;
    std::string status;
    std::string stamp_label;
    std::string stamp_duty_threshold;
    std::string stamp_duty;
    std::string port_type;
    std::string prefix; // POSVAS, EPMS
    std::string tid;
    std::string ip_and_port;    // POSVASPUBLIC_SSL, EMPSPUBLIC_SSL
    std::string phone_no;

    char  rawResponse[0x1000];
    


  public:
    MerchantParameterData(char *buffer);

    short parseXmlResponse();
    short parseJsonFile(char *buff, cJSON **parsed);
    void readMerchantDetails(MerchantData *parameter);
    void saveMerchantDetails();
    void sayMsg (int);
    
};

void MerchantParameterData::sayMsg (int port) {
    printf("Port is : %d\n", port);
}

MerchantParameterData::MerchantParameterData(char *buffer) {

    memcpy(rawResponse, buffer, strlen(buffer));
    printf("%s\n", rawResponse);

}

short MerchantParameterData::parseJsonFile(char *buff, cJSON **parsed)
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

short MerchantParameterData::parseXmlResponse()
{
    // efttran->tran, then my data

    ezxml_t root;
    ezxml_t tran;
    root = ezxml_parse_str(rawResponse, strlen(rawResponse));

    printf("Raw response is>>>> %s\n", rawResponse);

    if (!root) {
        printf("\nError, Please Try Again\n");
        ezxml_free(root);
        return -1;
    }

    tran = ezxml_child(root, "tran");

    address.append(ezxml_child(tran, "Address")->txt);
    printf("Address : %s\n", ezxml_child(tran, "Address")->txt);

    rrn.append(ezxml_child(tran, "RRN")->txt);
    printf("rrn : %s\n", ezxml_child(tran, "RRN")->txt);

    tid.append(ezxml_child(tran, "message")->txt);
    printf("tid : %s\n", ezxml_child(tran, "message")->txt);

    status.append(ezxml_child(tran, "status")->txt);
    printf("Status : %s\n", ezxml_child(tran, "status")->txt);

    stamp_label.append(ezxml_child(tran, "STAMP_LABEL")->txt);
    printf("Stamp Label : %s\n", ezxml_child(tran, "STAMP_LABEL")->txt);

    stamp_duty_threshold.append(ezxml_child(tran, "STAMP_DUTY_THRESHOLD")->txt);
    printf("Stamp threshold : %s\n", ezxml_child(tran, "STAMP_DUTY_THRESHOLD")->txt);

    stamp_duty.append(ezxml_child(tran, "STAMP_DUTY")->txt);
    printf("Stamb duty : %s\n", ezxml_child(tran, "STAMP_DUTY")->txt);

    port_type.append(ezxml_child(tran, "PORT_TYPE")->txt);
    printf("Port Type : %s\n", ezxml_child(tran, "PORT_TYPE")->txt);

    prefix.append(ezxml_child(tran, "PREFIX")->txt);
    printf("Platform : %s\n", ezxml_child(tran, "PREFIX")->txt);

    if(strcmp(prefix.c_str(), "POSVAS") == 0)
    {
        // get POSVAS IP and PORT
        ip_and_port.append(ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);
        printf("ip and port : %s\n", ezxml_child(tran, "POSVASPUBLIC_SSL")->txt);

    } else if(strcmp(prefix.c_str(), "EPMS") == 0)
    {
        // EPMS IP and PORT
        ip_and_port.append(ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);
        printf("ip and port : %s\n", ezxml_child(tran, "EPMSPUBLIC_SSL")->txt);
    }

    phone_no.append(ezxml_child(tran, "phone")->txt);
    printf("Phone no : %s\n", ezxml_child(tran, "phone")->txt);

    ezxml_free(root);
    //ezxml_free(tran);
}

void MerchantParameterData::readMerchantDetails(MerchantData *parameter)
{
    // 1. Read .json file
    // 2. convert it to struct for user to use

	cJSON *json;
    cJSON *jsonAddress, *jsonRrn, *jsonStatus, *jsonTID, *jsonStampLabel, *jsonStampDuty, *jsonStampDutyThreshold;
    cJSON *jsonPlatform, *jsonNibssIp, *jsonNibssPort, *jsonPortType, *jsonPhone;
   
    char buffer[1024] = {'\0'};

    getRecord((void *)buffer, MERCHANT_DETAIL_FILE, sizeof(buffer), 0);

    printf("After read record :\n%s\n", (char *)buffer);

    if(parameter->status = parseJsonFile(buffer, &json) == -1) return;
    
    jsonAddress = cJSON_GetObjectItemCaseSensitive(json, "address");
    jsonRrn = cJSON_GetObjectItemCaseSensitive(json, "rrn");
    jsonTID = cJSON_GetObjectItemCaseSensitive(json, "tid");
    jsonStampLabel = cJSON_GetObjectItemCaseSensitive(json, "stamp_label"); // String
    jsonStampDuty = cJSON_GetObjectItemCaseSensitive(json, "stamp_duty");   // Int
    jsonStampDutyThreshold = cJSON_GetObjectItemCaseSensitive(json, "stamp_threshold");    // Int
    jsonPlatform = cJSON_GetObjectItemCaseSensitive(json, "prefix");    // String
    jsonPhone = cJSON_GetObjectItemCaseSensitive(json, "phone_no"); // String
    jsonNibssIp = cJSON_GetObjectItemCaseSensitive(json, "ip");    // String
    jsonNibssPort = cJSON_GetObjectItemCaseSensitive(json, "port");    // Int
    jsonPortType = cJSON_GetObjectItemCaseSensitive(json, "port_type"); // String

    if(cJSON_IsString(jsonAddress))
    {
        memcpy(parameter->address, jsonAddress->valuestring, sizeof(parameter->address));
        printf("Address : %s\n", parameter->address);
    }

    if(cJSON_IsString(jsonRrn))
    {
        memcpy(parameter->rrn, jsonRrn->valuestring, sizeof(parameter->rrn));
        printf("rrn : %s\n", parameter->rrn);
    }

    if(cJSON_IsString(jsonTID))
    {
        memcpy(parameter->tid, jsonTID->valuestring, sizeof(parameter->tid));
        printf("TID : %s\n", parameter->tid);
    }

    if(cJSON_IsString(jsonStampLabel))
    {
        memcpy(parameter->stamp_label, jsonStampLabel->valuestring, sizeof(parameter->stamp_label));
        printf("Stamp Label : %s\n", parameter->stamp_label);
    }

    if(cJSON_IsNumber(jsonStampDuty))
    {
        parameter->stamp_duty = jsonStampDuty->valueint;
        printf("Stamp duty : %d\n", parameter->stamp_duty);
    }

    if(cJSON_IsNumber(jsonStampDutyThreshold))
    {
        parameter->stamp_duty_threshold = jsonStampDutyThreshold->valueint;
        printf("Stamp duty threshold: %d\n", parameter->stamp_duty_threshold);
    }

    if(cJSON_IsString(jsonPlatform))
    {
        memcpy(parameter->nibss_platform, jsonPlatform->valuestring, sizeof(parameter->nibss_platform));
        printf("Nibss platform : %s\n", parameter->nibss_platform);
    }

    if(cJSON_IsString(jsonNibssIp))
    {
        memcpy(parameter->nibss_ip, jsonNibssIp->valuestring, sizeof(parameter->nibss_ip));
        printf("Nibss ip : %s\n", parameter->nibss_ip);
    }

    if(cJSON_IsNumber(jsonNibssPort))
    {
        parameter->nibss_port = jsonNibssPort->valueint;
        printf("Nibss port : %d\n", parameter->nibss_port);
    }

    if(cJSON_IsString(jsonPhone))
    {
        memcpy(parameter->phone_no, jsonPhone->valuestring, sizeof(parameter->phone_no));
        printf("Customer Phone : %s\n", parameter->phone_no);
    }

    if(cJSON_IsString(jsonPortType))
    {
        memcpy(parameter->port_type, jsonPortType->valuestring, sizeof(parameter->port_type));
        printf("Port Type : %s\n", parameter->port_type);
    }

    cJSON_Delete(json);

}

void MerchantParameterData::saveMerchantDetails()
{
    
    cJSON *json, *requestJson;
    string ip;
    int port;
    int pos = 0;
    char *requestJsonStr;
    char jsonData[1024] = {'\0'};


    // 1. get xml file coming and convert
    // 2. Parse the xml
    parseXmlResponse();

    // 3. Arrange them in json formats
    requestJson = cJSON_CreateObject();

    if (!requestJson) {
        
        printf("Can't create json object\n");
        return;
    }

    pos = ip_and_port.find(';');
    ip = ip_and_port.substr(0, pos);
    port = atoi(ip_and_port.substr(pos+1, ip_and_port.length()).c_str());

    cJSON_AddItemToObject(requestJson, "address", cJSON_CreateString(address.c_str()));
    cJSON_AddItemToObject(requestJson, "rrn", cJSON_CreateString(rrn.c_str()));
    cJSON_AddItemToObject(requestJson, "tid", cJSON_CreateString(tid.c_str()));
    cJSON_AddItemToObject(requestJson, "status", cJSON_CreateNumber(atoi(status.c_str())));
    cJSON_AddItemToObject(requestJson, "stamp_label", cJSON_CreateString(stamp_label.c_str()));
    cJSON_AddItemToObject(requestJson, "stamp_threshold", cJSON_CreateNumber(atoi(stamp_duty_threshold.c_str())));
    cJSON_AddItemToObject(requestJson, "stamp_duty", cJSON_CreateNumber(atoi(stamp_duty.c_str())));
    cJSON_AddItemToObject(requestJson, "port_type", cJSON_CreateString(port_type.c_str()));
    cJSON_AddItemToObject(requestJson, "prefix", cJSON_CreateString(prefix.c_str()));
    cJSON_AddItemToObject(requestJson, "ip", cJSON_CreateString(ip.c_str()));
    cJSON_AddItemToObject(requestJson, "port", cJSON_CreateNumber(port));
    cJSON_AddItemToObject(requestJson, "phone_no", cJSON_CreateString(phone_no.c_str()));

    requestJsonStr = cJSON_PrintUnformatted(requestJson);
    memcpy(jsonData, requestJsonStr, sizeof(jsonData));

    printf("Parameter in json format :\n%s\n", jsonData);

    // 4. Save the json file
    saveRecord((void *)jsonData, MERCHANT_DETAIL_FILE, sizeof(jsonData), 0);
    
}

void runGetMerchantParameter(MerchantData *param, int shouldRefresh)
{
    char responseXml[0x1000] = {'\0'};
    getMerchantDetails(responseXml);

    MerchantParameterData obj(responseXml);

    if(shouldRefresh)
    {
         obj.saveMerchantDetails();
    }
   
    obj.readMerchantDetails(param);
    obj.sayMsg(77);
  
}


