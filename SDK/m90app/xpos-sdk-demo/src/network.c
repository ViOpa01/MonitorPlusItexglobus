#include <string.h>
#include <stdio.h>

#include "network.h"
#include "merchant.h"
#include "util.h"
#include "nibss.h"
#include "log.h"
#include "sdk_http.h"
#include "merchant.h"
#include "libapi_xpos/inc/libapi_comm.h"
#include "libapi_xpos/inc/libapi_gui.h"


#define ITEX_TAMS_PUBLIC_IP "basehuge.itexapp.com"
#define ITEX_TASM_PUBLIC_PORT "80"
#define ITEX_TASM_SSL_PORT "443"

#define COMM_SOCK	m_comm_sock

static int m_connect_tick = 0;
static int m_connect_time = 0;
static int m_connect_exit = 0;
static int m_comm_sock = 1;

extern isIdleState;
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
    { "20408", "FAST.M2M", "", "", "" },
    { "20404", "ESEYE.COM", "USER", "PASS", "" },
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

		// strncpy(netParam->host, "196.6.103.73", strlen(mParam.nibss_ip));
		// netParam->port = 5043;

		strncpy(netParam->host, mParam.nibss_ip, strlen(mParam.nibss_ip));
		netParam->port = mParam.nibss_ssl_port;

		strncpy(netParam->title, "Nibss", 10);
		netParam->isSsl = 1;

		printf("SSL: EPMS/POSVAS: ip -> %s, port -> %d\n", netParam->host, netParam->port);

	} else if(netType == NET_POSVAS_PLAIN || netType == NET_EPMS_PLAIN)
	{
		
		strncpy(netParam->host, mParam.nibss_ip, strlen(mParam.nibss_ip));
		if(!strcmp(mParam.nibss_platform, "POSVAS"))
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
		sprintf(msg , "Connecting(%d)" , num);
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
		sprintf(msg , "Connecting(%d)" , num);
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

	sprintf(tmp, "Connecting...(%d)", i + 1);

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




short netLink(Network * gprsSettings)
{
	printf("Linking: APN -> %s, Username -> %s, Pass -> %s, operator -> %s, timeout -> %d\n", gprsSettings->apn, gprsSettings->username, gprsSettings->password, gprsSettings->operatorName, gprsSettings->timeout);
	return comm_net_link_ex(gprsSettings->operatorName, gprsSettings->apn, gprsSettings->timeout, gprsSettings->username, gprsSettings->password, 0);
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
		networkErrorLogger(netParam->async, "NET LINK", "Link Failed!", 10, gui_messagebox_show);	
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
		networkErrorLogger(netParam->async, netParam->isSsl ? "HTTPS" : "HTTP", "Connection Failed!", 1000, gui_messagebox_show);	
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
	char title[32];
	unsigned int tick2 = Sys_TimerOpen(1000);
	const int maxLen = sizeof(netParam->response);

	printf( "------http_recv_buff------\r\n" );		
	while(Sys_TimerCheck(tick1) > 0){
		int ret;
		int num;
		unsigned char buffer[2048 * 4] = { '\0' };

		if(!netParam->async)
		{
			if(strlen(netParam->title)>0){
				num = Sys_TimerCheck(tick1)/1000;
				num = num < 0 ? 0 : num;

				sprintf(msg , "%s...(%d)" , "Receiving" , num);

				comm_page_set_page(netParam->title , msg , 0);
				ret = comm_page_get_exit();

				if(ret == 1){ 
					return -2;
				}
			}
		}


		if(netParam->isSsl == 1){
			nret = comm_ssl_recv( netParam->socketFd, (unsigned char *)buffer/*(netParam->response + curRecvLen)*/, sizeof(buffer)/*maxLen - curRecvLen*/);
		}else{
			nret = comm_sock_recv( netParam->socketFd, (unsigned char *)buffer /*(netParam->response + curRecvLen)*/, sizeof(buffer)/*maxLen - curRecvLen */, 5000);
		}

		//printf("nret is : %d\n", nret);

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
	int result = -1;
	int bytes = 0;
	int count = 0;
	int timeover = netParam->receiveTimeout;
	unsigned int tick = Sys_TimerOpen(timeover);
	
	bytes = http_recv_buff(netParam, tick, timeover);

	if (bytes > 0) {
		netParam->responseSize = bytes;
	} else {
		return -1;
	}

	printf("\nrecv result -> %d\n", netParam->responseSize);

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

	puts("Receive Successful!\n");
	isIdleState = 1;
	netParam->isSsl ? comm_ssl_close(netParam->socketFd) : comm_sock_close(netParam->socketFd);
	comm_net_unlink(); // Unlink network connection
	return SEND_RECEIVE_SUCCESSFUL;
}

/**
 * Function: gprsInit
 * Usage: if (!gprsInit()) .... success.
 * ------------------------------------
 *
 */

short gprsInit(void)
{
    char imsi[20] = {'\0'};
    Network profile;
	MerchantData merchantData;
	int maxTry = 4;
	int result = -1;
	int i;
	char title[] = "GPRS INIT";

	gui_clear_dc();
	gui_text_out((gui_get_width() - gui_get_text_width(title)) / 2, gui_get_height() / 2, title);
	
    memset(&profile, 0x00, sizeof(Network));
    getImsi(imsi);
	
	memset(&merchantData, 0x00, sizeof(MerchantData));
	readMerchantData(&merchantData);

	if (imsiToNetProfile(&merchantData.gprsSettings, imsi)) {
		gui_messagebox_show("GPRS INIT" , "Can't Configure Sim", "" , "Exit" , 0);
		return -1;
	}

	merchantData.gprsSettings.timeout = 30000;
	saveMerchantData(&merchantData);

	for (i = 0; i < maxTry; i++) {
		result = netLink(&merchantData.gprsSettings);
		if (!result) break;
		Sys_Delay(500);
	}

	return result;
}