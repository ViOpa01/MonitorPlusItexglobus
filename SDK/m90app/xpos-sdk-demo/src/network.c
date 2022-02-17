#include <string.h>
#include <stdio.h>

#include "network.h"
#include "merchant.h"
#include "util.h"
#include "nibss.h"
#include "log.h"
#include "sdk_http.h"
#include "merchant.h"
#include "itexFile.h"
#include "cJSON.h"
#include "libapi_xpos/inc/libapi_comm.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_file.h"
#include "libapi_xpos/inc/libapi_util.h"


#define ITEX_TASM_PUBLIC_PORT "80"
#define ITEX_TASM_SSL_PORT "443"

#define APN_LABEL_SIZE  21

#define COMM_SOCK	m_comm_sock

static int m_connect_tick = 0;
static int m_connect_time = 0;
static int m_connect_exit = 0;
static int m_comm_sock = 1;

extern isIdleState;

void resetCallHomeIpAndPort(NetWorkParameters *netParam, MerchantData *mParam);
typedef int (*PrintSycnCb)(char *title, char* msg, char* leftButton, char* rightButton, int timeover);

static void networkErrorLogger(const int isAsync, const char* title, const char* msg, const int timeout, PrintSycnCb callback)
{
	if(isAsync)
	{
		LOG_PRINTF("%s : %s\n", title, msg);
		return;
	}

	callback(title, msg, "", "Ok", 3000);
}


static void logNetworkParameters(NetWorkParameters * netWorkParameters)
{
    LOG_PRINTF("Host -> %s:%d, packet size -> %d\n", netWorkParameters->host, netWorkParameters->port, netWorkParameters->packetSize);
    LOG_PRINTF("IsSsl -> %s, IsHttp -> %s\n", netWorkParameters->isSsl ? "YES" : "NO", netWorkParameters->isHttp ? "YES" : "NO");
    LOG_PRINTF("Recv Timeout -> %d, title -> %s", netWorkParameters->receiveTimeout, netWorkParameters->title);
}

void platformAutoSwitch(NetType *netType)
{
	
	if(!isDevMode(*netType))		// Platfrom Auto Switch is happening here
	{
		MerchantData mParam;
		memset(&mParam, 0x00, sizeof(MerchantData));
		readMerchantData(&mParam);

		*netType = mParam.nibss_platform;	
	}
}

static const char* networkProfiles[][5] = {
   	{ "62160", "etisalat", "", "", "ETISALAT" }, // etisalat
    // { "42402", "etisalat", "", ""},     // etisalat
    { "62130", "web.gprs.mtnnigeria.net", "web", "web", "MTN" },
    { "62001", "INTERNET", "", "", "" }, //EMP MTN PRIVATE SIM
    { "62150", "glosecure", "gprs", "gprs", "GLO" },
    { "62120", "internet.ng.airtel.com", "internet", "internet", "AIRTEL" },
    { "23450", "globasure", "web", "web", "GLO" },
    // { "20408", "FAST.M2M", "", "", "" },
    { "20404", "ESEYE.COM", "USER", "PASS", "" },
    { "20408", "ESEYE.COM", "USER", "PASS", "" },
    { "20601", "bicsapn", "", "", ""}
    //{"", "public_fast", ""},
    //{"", "public_wlapn", ""},
};

static const int netProfileList = (sizeof(networkProfiles) / sizeof(char*)) / 5;

int imsiToNetProfile(Network* profile, const char* imsi)
{
    int i;

    for (i = 0; i < netProfileList; i++) {
        if (!strncmp(imsi, networkProfiles[i][0], 5)) {
            strcpy(profile->apn, networkProfiles[i][1]);
            strcpy(profile->username, networkProfiles[i][2]);
            strcpy(profile->password, networkProfiles[i][3]);
			strcpy(profile->operatorName, networkProfiles[i][4]);
            return 0;
        }
    }

    return -1;
}

static void logNetworkProfile(Network* profile)
{
    printf("[INFO] :::%s::: APN -> %s, Username -> %s, Pass -> %s, operator -> "
           "%s, "
           "timeout -> %d\n",
        __FUNCTION__, profile->apn, profile->username, profile->password,
        profile->operatorName, profile->timeout);
}

static short netLink(Network * gprsSettings)
{
	logNetworkProfile(gprsSettings);
	return comm_net_link_ex(gprsSettings->operatorName, gprsSettings->apn, gprsSettings->timeout, gprsSettings->username, gprsSettings->password, 0);
}

static int linkSimProfile(Network *profile)
{
	int result = -1;
	int i = 0;
	char title[] = "Waiting...";
	const int MAX_TRIES = 3;

	gui_clear_dc();
	gui_text_out((gui_get_width() - gui_get_text_width("GPRS INIT")) / 2, 1, "GPRS INIT");
	gui_text_out((gui_get_width() - gui_get_text_width(title)) / 2, gui_get_height() / 2, title);
	profile->timeout = 30000;

	for (i = 0; i < MAX_TRIES; i++) {
		result = netLink(profile);
		if (result == 0) {
			break;
		}
		Sys_Delay(500);
	}

	return result;
}

int manualSimProfile(void) 
{
	Network profile = {'\0'};
	int result = 0;
NAME :
	gui_clear_dc();
    result = Util_InputTextEx(GUI_LINE_TOP(0), (char*)"Enter NAME", GUI_LINE_TOP(2), profile.operatorName, 1, sizeof(profile.operatorName) - 1, 1, 1, 30000, (char*)"Enter NAME");
	if (result < 0) return result;

APN :
    gui_clear_dc();
    result = Util_InputTextEx(GUI_LINE_TOP(0), (char*)"Enter APN", GUI_LINE_TOP(2), profile.apn, 1, sizeof(profile.apn) - 1, 1, 1, 30000, (char*)"Enter APN");
	if (result == -2) {
		goto NAME;
	} else if (result == -1 || result == -3) {
		return result;
	}

USERNAME :
    result = Util_InputTextEx(GUI_LINE_TOP(0), (char*)"Enter USER", GUI_LINE_TOP(2), profile.username, 0, sizeof(profile.username) - 1, 1, 1, 30000, (char*)"Enter USER");
	if (result == -2) {
		goto APN;
	} else if (result == -1 || result == -3) {
		return result;
	}

    result = Util_InputTextEx(GUI_LINE_TOP(0), (char*)"Enter PWD", GUI_LINE_TOP(2), profile.password, 0, sizeof(profile.password) - 1, 1, 1, 30000, (char*)"Enter PWD");

	if (result == -2) {
		goto USERNAME;
	} else if (result == -1 || result == -3) {
		return result;
	}

	printf("manual configuration\n");
	return linkSimProfile(&profile);
}

static short checkNetworkProfile(Network* profile)
{
	logNetworkProfile(profile);
    return profile->apn[0] || profile->username[0] || profile->password[0];
}

static int setSimProfileFromApnList(cJSON* apns)
{
	Network profile;
	int ret = -1;
	size_t i = 0;

	size_t size = cJSON_GetArraySize(apns);
	for (i = 0; i < size; i++) {
		cJSON *el, *tag;
		memset(&profile, '\0', sizeof(Network));

		el = cJSON_GetArrayItem(apns, i);

		tag = cJSON_GetObjectItemCaseSensitive(el, "apn");
		if (!tag || cJSON_IsNull(tag) || !cJSON_IsString(tag)) {
			continue;
		}
		strcpy(profile.apn, tag->valuestring);

		tag = cJSON_GetObjectItemCaseSensitive(el, "username");
		if (!tag || cJSON_IsNull(tag) || !cJSON_IsString(tag)) {
			continue;
		}
		strcpy(profile.username, tag->valuestring);

		tag = cJSON_GetObjectItemCaseSensitive(el, "password");
		if (!tag || cJSON_IsNull(tag) || !cJSON_IsString(tag)) {
			continue;
		}
		strcpy(profile.password, tag->valuestring);

		tag = cJSON_GetObjectItemCaseSensitive(el, "name");
		if (!tag || cJSON_IsNull(tag) || !cJSON_IsString(tag)) {
			continue;
		}
		strcpy(profile.operatorName, tag->valuestring);

		if (linkSimProfile(&profile) == 0) {
			ret = 0;
			break;
		}
	}
	return ret;
}

static short getSimStatus(char* mccMnc)
{
	if (!mccMnc[0]) {
		gui_messagebox_show("GPRS INIT" , "SIM not present", "" , "Exit" , 0);
		return -1;
	}
	return 0;
}

static short setSimProfileFromSavedApnForMccMnc(char* mccMnc)
{
	cJSON *json = NULL, *apns;
	char buffer[1024 * 2] = {'\0'};
	char filename[32] = {'\0'};
	short ret = -1;

	sprintf(filename, "../apn/%s.json", mccMnc);
	ret = UFile_Check(filename, FILE_PRIVATE);
    printf("ret : %d -> Ufile_Check %s\n", ret, filename);

    if(ret != UFILE_SUCCESS || getRecord((void *)buffer, filename, sizeof(buffer), 0)) {
		return ret;
	}

	if ((ret = parseJsonString(buffer, &json)) != 0) {
		gui_messagebox_show("ERROR" , "Parsing Error!", "", "", 3000);
		return ret;
	}

	apns = cJSON_GetObjectItemCaseSensitive(json, "apns");
    if (cJSON_IsNull(apns) || !cJSON_IsArray(apns)) {
        gui_messagebox_show("APN", "apns object not found!", "", "", 3000);
	    cJSON_Delete(json);
		return ret;
    }
	ret = setSimProfileFromApnList(apns);
	cJSON_Delete(json);
	return ret;
}

static short setSimProfileFromPreviousSettings(Network *profile)
{
	if (!checkNetworkProfile(profile)) {
		return -1;
	}
	linkSimProfile(profile);
	return 0;
}

static short setSimProfile(Network *profile, char* mccMnc)
{
	if (setSimProfileFromPreviousSettings(profile) == 0) {
		return 0;
	}

	if (setSimProfileFromSavedApnForMccMnc(mccMnc) == 0) {
		return 0;
	}

	return manualSimProfile();
}

int getSimConfigOption(const char **list, const int size)
{
	return gui_select_page_ex("SELECT APN TYPE", (char**)list, size, 30000, 0);// if exit : -1, timout : -2
}

int selectSimConfig()
{
	cJSON *json = NULL, *apns;
	cJSON *el, *tag;
	MerchantData merchantData;
	char apnList[][APN_LABEL_SIZE] = {0};
	char filename[32] = {'\0'};
	char imsi[20] = {'\0'};
	char mcc[6] = {'\0'};
	char buffer[1024 * 2] = {'\0'};
	int ret = -1;
	size_t size = 0;
	int apnSize = 0;
	int i = 0;

	memset(&merchantData, 0x00, sizeof(MerchantData));
	memset(apnList, 0x00, sizeof(apnList));
	readMerchantData(&merchantData);
	merchantData.gprsSettings.timeout = 30000;

	getImsi(imsi, sizeof(imsi));

	if (strlen(imsi) < 5) {
		gui_messagebox_show("GPRS INIT" , "Invalid imsi" , "", "Exit" , 0);
		return -1;
	}

	strncpy(mcc, imsi, 5);
	sprintf(filename, "../apn/%s.json", mcc);

	ret = UFile_Check(filename, FILE_PRIVATE);
    printf("ret : %d -> Ufile_Check %s\n", ret, filename);

    if(ret != 0 || getRecord((void *)buffer, filename, sizeof(buffer), 0)) {
		gui_messagebox_show("SIM CONFIG" , "APN not found!", "", "", 3000);
		return -1;
	}

	if (parseJsonString(buffer, &json)) {
		gui_messagebox_show("ERROR" , "Parsing Error!", "", "", 3000);
		goto clear_exit;
	}

	apns = cJSON_GetObjectItemCaseSensitive(json, "apns");
    if(!cJSON_IsArray(apns) || cJSON_IsNull(apns))
    {
        gui_messagebox_show("APN", "apns object not found!", "", "", 3000);
		ret = -1;
		goto clear_exit;
    }

	size = cJSON_GetArraySize(apns);
	for (i = 0; i < size; i++) {

		el = cJSON_GetArrayItem(apns, i);
		tag = cJSON_GetObjectItemCaseSensitive(el, "name");
		if (!tag || cJSON_IsNull(tag)) {
			ret = -1;
			continue;
		} else if (!cJSON_IsString(tag) || !tag->valuestring[0]) {
			strncpy(apnList[i], "UNKNOWN APN", APN_LABEL_SIZE -1);
		} else {
			strncpy(apnList[i], tag->valuestring, APN_LABEL_SIZE -1);
		}

		printf("APN[%d] => %s\n", i, apnList[i]);

	}

	{
		const char *menu[size];
		int index = 0;
		for (index = 0; index < size; index++) {
			menu[index] = apnList[index];
			printf("MENU[%d] => %s,  APN[%d] => %s\n", index, menu[index], index, apnList[index]);
		}

		apnSize = (sizeof(menu) / sizeof(char*));
		printf("APN SIZE : %d\n", apnSize);

		if (apnSize > 0) {
			int option = -1;

			option = getSimConfigOption(menu, apnSize);

			if (option < 0) {
				ret = -1;
				goto clear_exit;
			}

			el = cJSON_GetArrayItem(apns, option);
			if (!el || cJSON_IsNull(el)) {
				ret = 0;
				goto clear_exit;
			}

			tag = cJSON_GetObjectItemCaseSensitive(el, "apn");
			if (tag && !cJSON_IsNull(tag) &&cJSON_IsString(tag)) {
				strcpy(merchantData.gprsSettings.apn, tag->valuestring);;
			}

			tag = cJSON_GetObjectItemCaseSensitive(el, "username");
			if (tag && !cJSON_IsNull(tag) &&cJSON_IsString(tag)) {
				strcpy(merchantData.gprsSettings.username, tag->valuestring);;
			}

			tag = cJSON_GetObjectItemCaseSensitive(el, "password");
			if (tag && !cJSON_IsNull(tag) &&cJSON_IsString(tag)) {
				strcpy(merchantData.gprsSettings.password, tag->valuestring);;
			}

			tag = cJSON_GetObjectItemCaseSensitive(el, "name");
			if (tag && !cJSON_IsNull(tag) &&cJSON_IsString(tag)) {
				strcpy(merchantData.gprsSettings.operatorName, tag->valuestring);;
			}

			strncpy(merchantData.gprsSettings.imsi, imsi, sizeof(merchantData.gprsSettings.imsi) -1);
			saveMerchantData(&merchantData);

			if (linkSimProfile(&merchantData.gprsSettings) == 0) {
				ret = 0;
			}

		}
	}

clear_exit :
	if (!cJSON_IsNull(json)) cJSON_Delete(json);

	return ret;
}

short getNetParams(NetWorkParameters * netParam, NetType netType, int isHttp)
{
	MerchantData mParam;

	memset(&mParam, 0x00, sizeof(MerchantData));
	readMerchantData(&mParam);

	platformAutoSwitch(&netType);

	if(netType == NET_EPMS_SSL || netType == NET_POSVAS_SSL)
	{
		// 196.6.103.72 5042  nibss epms port and ip test environment
		// 196.6.103.18 5014  nibss posvas port and ip live environment
		// 196.6.103.73 5043  nibss epms port and ip live environment
		strncpy(netParam->title, "Nibss", 10);

		// strncpy(netParam->host, "197.253.19.78", strlen(mParam.nibss_ip));
		// netParam->port = 5008;
		// netParam->isSsl = 1;

		// strncpy(netParam->host, "196.6.103.73", strlen(mParam.nibss_ip));
		// netParam->port = 5043;
		// netParam->isSsl = 1;

		strncpy(netParam->host, mParam.nibss_ip, sizeof(netParam->host));
		netParam->port = mParam.nibss_ssl_port;
		netParam->isSsl = 1;

		printf("SSL: EPMS/POSVAS: ip -> %s, port -> %d\n", netParam->host, netParam->port);

	} else if(netType == NET_POSVAS_PLAIN || netType == NET_EPMS_PLAIN)
	{
		
		strncpy(netParam->host, mParam.nibss_ip, sizeof(netParam->host));
		if(!strcmp(mParam.platform_label, "POSVAS"))
		{
			netParam->port = 5004;		// To handle wrong IP coming for Plain POSVAS on TAMS;
		} else {
			
			netParam->port = mParam.nibss_plain_port;
		}
		

		strncpy(netParam->title, "Nibss", 10);
		netParam->isSsl = 0;

		printf("Plain: EMPS/POSVAS: ip -> %s, port -> %d\n", netParam->host, netParam->port);

	}else if(netType == NET_EPMS_SSL_TEST) {
		strcpy(netParam->host,  "196.6.103.72");
		netParam->port = 5043;
		netParam->isSsl = 1;
	}else if(netType == NET_EPMS_PLAIN_TEST) {

	}else if(netType == NET_POSVAS_SSL_TEST) {
		strcpy(netParam->host, "196.6.103.10");
        netParam->port = 55533;
		netParam->isSsl = 1;

		printf("SSL: TEST/POSVAS: ip -> %s, port -> %d\n", netParam->host, netParam->port);
	}else if(netType == NET_POSVAS_PLAIN_TEST) {
		netParam->isSsl = 0;
		strcpy(netParam->host, "196.6.103.10");
		netParam->port = 55531;
	
		printf("Plain: TEST/POSVAS: ip -> %s, port -> %d\n", netParam->host, netParam->port);
	}else if(netType == UPSL_DIRECT_TEST) {
		netParam->isSsl = 0;
		strcpy(netParam->host, "196.46.20.30");
		netParam->port = 5334;
		
		printf("Plain: UPSL: ip -> %s, port -> %d\n", netParam->host, netParam->port);

	}
	else 
	{
		printf("Uknown Host\n");
	}

	netParam->isHttp = isHttp;
	// netParam->receiveTimeout = 1000;
	netParam->receiveTimeout = 90000;

	return 0;

}


static void comm_page_set_page(char * title , char * msg ,int quit)
{
	gui_begin_batch_paint();
	
	gui_clear_dc();

	gui_text_out((gui_get_width() - gui_get_text_width(title)) / 2, GUI_LINE_TOP(0), title);
	gui_text_out(0 , GUI_LINE_TOP(1)  , msg);

	if (quit == 1){
		gui_page_op_paint("Cancel" , "");
	}

	gui_end_batch_paint();
}

static int comm_page_get_exit()
{
	st_gui_message pmsg;
	int ret = 0;
	int get_ret = 0;

	while(ret == 0){
		ret = gui_get_message(&pmsg, 0);

		if ( ret == 0 ){
			if (pmsg.message_id == GUI_KEYPRESS && pmsg.wparam == GUI_KEY_QUIT){
				get_ret = 1;
			}
			gui_proc_default_msg(&pmsg);
		}
	}

	return get_ret;
}

static int _connect_server_func_proc_async()
{
	char msg[32] = {0};
	int ret = 0;
	int num = 0;

	num =  Sys_TimerCheck(m_connect_tick) / 1000;
	
	if(num <= 0){
		ret = 1;
	}
	else {
		// sprintf(msg , "Connecting(%d)" , num);
		sprintf(msg , "Connecting...");
		LOG_PRINTF("%s \n", msg);
	}

	return ret ;
}

static int _connect_server_func_proc()
{
	char msg[32] = {0};
	int ret = 0;
	int num = 0;

	if(comm_page_get_exit() == 1){
		m_connect_exit = 1;
	}

	num =  Sys_TimerCheck(m_connect_tick) / 1000;
	
	if(num <= 0){
		ret = 1;
	}
	else if(m_connect_exit == 0){
		// sprintf(msg , "Connecting(%d)" , num);
		sprintf(msg , "Connecting...");
		comm_page_set_page("Http", msg , 0);
	}
	else{
		sprintf(msg , "Canceling...");
		comm_page_set_page("Http", msg , 0);
		ret = 2;
	}

	return ret ;
}

static int allDataReceived(unsigned char *packet, const int bytesRead, const char * endTag)
{
    
	int result = 0;

	if (bytesRead) {
		int index = 0;
		const int tpduSize = 2;
		
		if (*packet && endTag && strstr(packet, endTag)) {
			result = 1;
		} else if(bytesRead > 2 && *(&packet[tpduSize])) {
			unsigned char bcdLen[2];
			int msgLen;

			memcpy(bcdLen, packet, tpduSize);
			msgLen = ((bcdLen[0] << 8) + bcdLen[1]) + tpduSize;
		    index = tpduSize;
			result = (msgLen == bytesRead) ? 1 : 0;
			printf("%s:::msgLen -> %d, bytesRead -> %d\n", __FUNCTION__, msgLen, bytesRead);
		}

		packet[bytesRead] = 0;
		printf("Packet -> '%s'\n", &packet[index]);
	}

    return result;
}



static short tryConnection(NetWorkParameters *netParam, const int i)
{
	char tmp[32] = {0};
	int result;
	int sock = 0;

	m_connect_tick = Sys_TimerOpen(30000);
	m_connect_exit = 0;
	m_connect_time = i + 1;

	// sprintf(tmp, "Connecting...(%d)", i + 1);
	sprintf(tmp, "Connecting...");

	if(netParam->async) {
		LOG_PRINTF("%s : %s\n", netParam->title, tmp);
	} else {
		comm_page_set_page(netParam->title, tmp, 1);
	}

	if (netParam->isSsl)
	{

		//for(;;) 
		//{
			result = comm_ssl_init(netParam->socketFd, 0/*netParam->serverCert*/, 0 /*netParam->clientCert*/, 0/*netParam->clientKey*/, 0/*netParam->verificationLevel*/);
			LOG_PRINTF("Ignoring ssl init return -> %d, comm socket -> %d", result, netParam->socketFd);
			//if (result == 0) break;
			//Sys_Delay(5000);
		//}
		

		sprintf(tmp , "Connect server %d times" , i + 1);
		if(netParam->async)
		{
			LOG_PRINTF("%s : %s\n", "SSL", tmp);
		} else {
			comm_page_set_page("SSL", tmp , 0);
		}
		

		//TODO: Check callback later.
		if(netParam->async)
		{
			result = comm_ssl_connect2(netParam->socketFd, netParam->host, netParam->port, _connect_server_func_proc_async);
		} else {
			result = comm_ssl_connect2(netParam->socketFd, netParam->host, netParam->port, _connect_server_func_proc);
		}
		LOG_PRINTF("Ssl connect ret : %d\n", result);
		
		if (result)
		{
			LOG_PRINTF("Ssl connect failed... ret : %d\n", result);
			comm_ssl_close(netParam->socketFd);
			Sys_Delay(500);
			return -4;
		}

		if(!netParam->async) 
		{
			if (comm_page_get_exit() || m_connect_exit == 1)
			{
				LOG_PRINTF("comm_func_connect_server Cancel");
				comm_ssl_close(netParam->socketFd);
				Sys_Delay(500);
				return -5;
			}
		}
	}
	else
	{
		if (comm_sock_connect(netParam->socketFd, netParam->host, netParam->port)) //  Connect to http server
		{
			LOG_PRINTF("Socket connect failed...\n");
			comm_sock_close(netParam->socketFd);
			Sys_Delay(500);
			return -6;
		}
	}

	return 0;
}

void * preDial(void * netParams)
{
	int maxTry = 3;
	Network * gprsSettings = (Network *) netParams;
	int result = -1;
	int i;

	for (i = 0; i < maxTry; i++) {
		result = netLink(gprsSettings);
		if (!result) break;
	}

	result ? puts("Predialing failed...") : puts("Predialing Successful...\n");
}

static short connectToHost(NetWorkParameters *netParam)
{
    int i;
    int nret;	
	const int nTime = 3;
	MerchantData merchantData;
	netParam->socketFd = 1;

	memset(&merchantData, 0x00, sizeof(MerchantData));
	readMerchantData(&merchantData);

	nret = netLink(&merchantData.gprsSettings);
	
	if (nret != 0)
	{
		// Fail to link
		networkErrorLogger(netParam->async, "NET LINK ERROR", "SIM not present\nOR Out of Data", 10, gui_messagebox_show);	
		return -1;
	}
	
	if((netParam->socketFd = comm_sock_create(netParam->socketIndex)) != netParam->socketIndex)	// socketIndex (0/1), only callhome thread using 1
	{
		// Fail to create socket
		printf("****Net descriptor : %d***\n", netParam->socketFd);
		networkErrorLogger(netParam->async, "SOCKET", "Socket Failed!", 10, gui_messagebox_show);	
		return -2;
	}

	//Sys_Delay(1000);

	for (i = 0; i < nTime; i++)
	{
		if (!tryConnection(netParam, i)) break;
		Sys_Delay(500);
	}

	if (i == nTime) {
		// Fail to connect
		networkErrorLogger(netParam->async, netParam->isSsl ? "HTTPS" : "HTTP", "SIM out of Data\nOR Host is down.", 1000, gui_messagebox_show);	
		return -3;
	}

	return 0;
}


static short sendPacket(NetWorkParameters *netParam)
{
	int result = -1;

	printf("packet size to send -> %d\n", netParam->packetSize);
	printf("\npacket -> %s\n", netParam->packet);

	if (netParam->isSsl)
	{
		result = comm_ssl_send(netParam->socketFd, netParam->packet, netParam->packetSize);
		return result;
	}
	else
	{
		result = comm_sock_send(netParam->socketFd, (unsigned char *)netParam->packet, netParam->packetSize);
	}

	printf("Send Result : %d\n", result);


	return result > 0 ? 0 : -1;
}


/*
;
bytes = http_recv_buff(netParam->title, netParam->response, size, tick, timeover, netParam->isSsl);
*/

static int http_recv_buff(NetWorkParameters *netParam, unsigned int tick1, int timeover)
{
	int nret;
	int curRecvLen = 0;
	char msg[32];
	unsigned int tick2 = Sys_TimerOpen(1000);
	const int maxLen = sizeof(netParam->response);

	printf( "------http_recv_buff------\r\n" );		
	while(Sys_TimerCheck(tick1) > 0){
		unsigned char buffer[0x10000];
		memset(buffer, 0x00, sizeof(buffer));

		if(!netParam->async)
		{
			if(strlen(netParam->title)>0){
				int ret;
				int num;
				num = Sys_TimerCheck(tick1)/1000;
				num = num < 0 ? 0 : num;

				// sprintf(msg , "%s...(%d)" , "Receiving" , num);
				sprintf(msg , "Receiving...");

				comm_page_set_page(netParam->title , msg , 0);
				ret = comm_page_get_exit();

				if(ret == 1){ 
					return -2;
				}
			}
		}


		if(netParam->isSsl == 1){
			nret = comm_ssl_recv( netParam->socketFd, (unsigned char *)buffer/*(netParam->response + curRecvLen)*/, sizeof(buffer) - 1/*maxLen - curRecvLen*/);
		}else{
			nret = comm_sock_recv( netParam->socketFd, (unsigned char *)buffer /*(netParam->response + curRecvLen)*/, sizeof(buffer) -1/*maxLen - curRecvLen */, 5000);
		}

		if (nret >0){
			tick2 = Sys_TimerOpen(1000);

			memcpy(&netParam->response[curRecvLen], buffer, nret);

			curRecvLen += nret;

			if(curRecvLen >= maxLen){
				break;
			}else if(allDataReceived(netParam->response, curRecvLen, netParam->endTag)){
				break;
			}
		}else if(Sys_TimerCheck(tick2) == 0 && netParam->isSsl == 1){
			mf_ssl_recv3(netParam->socketFd);
			//printf("----------mf_ssl_recv3\r\n");
			tick2 = Sys_TimerOpen(1000);
		}
	}

	printf( "------curRecvLen: %d\r\n", curRecvLen );		

	return curRecvLen;
}




/*
  unsigned char packet[2048];
    unsigned char response[4096];
    int packetSize;  
    int responseSize;
    unsigned char host[64];
    int port;
    int isSsl;
    int isHttp;
    char apn[50];
    char title[35];
    int netLinkTimeout;
    int receiveTimeout;

    char serverCert[256];
	char clientCert[256];
	char clientKey[256];
	int verificationLevel; 
*/

static short receivePacket(NetWorkParameters *netParam)
{
	int bytes = 0;
	int timeover = netParam->receiveTimeout;
	unsigned int tick = Sys_TimerOpen(timeover);
	
	bytes = http_recv_buff(netParam, tick, timeover);

	if (bytes > 0) {
		netParam->responseSize = bytes;
	} else {
		return -1;
	}

	printf("\nrecv result -> %d, \n", netParam->responseSize);

	return bytes > 0 ? 0 : -1;
	
}

/*
short getNetParams(NetWorkParameters * netParam, const NetType netType)
{
	MerchantData mParam;

	memset(&mParam, 0x00, sizoef(MerchantData));
	readMerchantData(&mParam);


	strncpy(netParam->host, mParam.nibss_ip, strlen(mParam.nibss_ip));
	netParam->port = mParam.nibss_ssl_port;
	netParam->isSsl = 1;
	netParam->isHttp = 0;
	netParam->receiveTimeout = 1000;
	strncpy(netParam->title, "Nibss", 10);
	strncpy(netParam->apn, "CMNET", 10);
	netParam->netLinkTimeout = 30000;

	return 0;

}
*/



#if 0
void hostTest(void)
{
    char testData[200] = { '\0' };
    int len;
    char* response = 0;
    enum com_ErrorCodes com_errno;

    struct com_ConnectHandle* handle = beginNibssConnection(NULL, NULL, &com_errno);
    if (!handle) {
        UI_ShowOkCancelMessage(10000, "Nibss", com_GetErrorString(com_errno), UI_DIALOG_TYPE_NONE);
        return;
    }

    strcpy(&testData[2], "080022380000008000009A0000031611084511084511084503162044309N");

    len = strlen(&testData[2]);
    testData[0] = (unsigned char)(len >> 8);
    testData[1] = (unsigned char)len;

    len = sendAndReceiveNibss((void**)&response, handle, (const void*)testData, len + 2, nibssCallback, connection_callback, &com_errno);
    if (len > 0 && response) {
        char message[1024] = { 0 };
        memcpy(message, response + 2, len - 2);
        UI_ShowOkCancelMessage(2000, "RECEIVED", message, UI_DIALOG_TYPE_CONFIRMATION);
        free(response);
    }

    endNibssConnection(handle);
}
#endif


enum CommsStatus sendAndRecvPacket(NetWorkParameters *netParam)
{
	isIdleState = 0;
	logNetworkParameters(netParam);

	if (connectToHost(netParam))
	{
		isIdleState = 1;
		netParam->isSsl ? comm_ssl_close(netParam->socketFd) : comm_sock_close(netParam->socketFd);
		comm_net_unlink(); // Unlink network connection
		return CONNECTION_FAILED;
	}

	puts("\nConnection Successful!\n");

	if (sendPacket(netParam))
	{
		isIdleState = 1;
		netParam->isSsl ? comm_ssl_close(netParam->socketFd) : comm_sock_close(netParam->socketFd);
		comm_net_unlink(); // Unlink network connection
		return SENDING_FAILED;
	}

	//Sys_Delay(500);

	puts("Sending Successful!\n");

	if (receivePacket(netParam))
	{
		isIdleState = 1;
		networkErrorLogger(netParam->async, "Response", "No response received!", 3000, gui_messagebox_show);	
		netParam->isSsl ? comm_ssl_close(netParam->socketFd) : comm_sock_close(netParam->socketFd);
		comm_net_unlink(); // Unlink network connection
		return RECEIVING_FAILED;
	}

	// LOG_PRINTF("Response : %s", &netParam->response[4]);

	puts("Receive Successful!\n");
	isIdleState = 1;
	netParam->isSsl ? comm_ssl_close(netParam->socketFd) : comm_sock_close(netParam->socketFd);
	comm_net_unlink(); // Unlink network connection
	return SEND_RECEIVE_SUCCESSFUL;
}

static void getNetworkProfile(Network* profile)
{
	MerchantData merchantData = {'\0'};
	readMerchantData(&merchantData);

	memcpy(profile, &merchantData.gprsSettings, sizeof(Network));
}

short gprsInit(void)
{
	Network profile = {'\0'};
	char mccMnc[6] = {'\0'};

	getMccMnc(mccMnc, sizeof(mccMnc));
	if (getSimStatus(mccMnc) != 0) {
		return -1;
	}

	getNetworkProfile(&profile);
	return setSimProfile(&profile, mccMnc);
}

void resetCallHomeIpAndPort(NetWorkParameters *netParam, MerchantData *mParam) {
	strncpy(netParam->host, mParam->callhome_ip, strlen(mParam->callhome_ip));
	netParam->port = mParam->callhome_port;
}
