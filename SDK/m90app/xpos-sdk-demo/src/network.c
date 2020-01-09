#include <string.h>

#include "network.h"
#include "merchant.h"
#include "util.h"
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

static void logNetworkParameters(NetWorkParameters * netWorkParameters)
{
    LOG_PRINTF("Host -> %s:%d, packet size -> %d\n", netWorkParameters->host, netWorkParameters->port, netWorkParameters->packetSize);
    LOG_PRINTF("IsSsl -> %s, IsHttp -> %s\n", netWorkParameters->isSsl ? "YES" : "NO", netWorkParameters->isHttp ? "YES" : "NO");
    LOG_PRINTF("NetLink Timeout -> %d, Recv Timeout -> %d, title -> %s", netWorkParameters->netLinkTimeout,  netWorkParameters->receiveTimeout, netWorkParameters->title);

}

short getNetParams(NetWorkParameters * netParam, const NetType netType, int isHttp)
{
	MerchantData mParam;

	memset(&mParam, 0x00, sizeof(MerchantData));
	readMerchantData(&mParam);

	// NET_EPMS_SSL,
    // NET_EPMS_PLAIN,
    // NET_POSVAS_SSL,
    // NET_POSVAS_PLAIN


	if(netType == NET_EPMS_SSL || netType == NET_POSVAS_SSL)
	{
		// 196.6.103.72 5042  nibss epms port and ip test environment
		strncpy(netParam->host, mParam.nibss_ip, strlen(mParam.nibss_ip));
		netParam->port = mParam.nibss_port;
		strncpy(netParam->title, "Nibss", 10);
		netParam->isSsl = 1;

		printf("SSL: EPMS/POSVAS: ip -> %s, port -> %d\n", netParam->host, netParam->port);

	} else if(netType == NET_POSVAS_PLAIN || netType == NET_EPMS_PLAIN)
	{
		
		
		strncpy(netParam->host, mParam.nibss_ip, strlen(mParam.nibss_ip));
		//strncpy(netParam->host, "196.6.103.10", strlen(mParam.nibss_ip));

		netParam->port = 5004;
		//netParam->port = mParam.nibss_plain_port;

		// strncpy(netParam->host, "196.6.103.10", strlen(mParam.nibss_ip));
		// netParam->port = 55531;
		strncpy(netParam->title, "Nibss", 10);
		netParam->isSsl = 0;

		printf("Plain: EMPS/POSVAS: ip -> %s, port -> %d\n", netParam->host, netParam->port);

	}
	else 
	{
		printf("Uknown Host\n");
	}
	
	netParam->isHttp = isHttp;
	netParam->receiveTimeout = 4000;
	strncpy(netParam->apn, "CMNET", 10);
	netParam->netLinkTimeout = 30000;

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
		sprintf(msg , "Connect(%d)" , num);
		comm_page_set_page("Http", msg , 1);
	}
	else{
		sprintf(msg , "Canceling...");
		comm_page_set_page("Http", msg , 0);
		ret = 2;
	}

	return ret ;
}

static int allDataReceived(unsigned char *packet, const int bytesRead)
{
    const int tpduSize = 2;
    unsigned char bcdLen[2];
    int msgLen;

    memcpy(bcdLen, packet, tpduSize);
    msgLen = ((bcdLen[0] << 8) + bcdLen[1]) + tpduSize;

    return (msgLen == bytesRead) ? 1 : 0;
}


// Http receiving processing, the step is to receive the byte stream from the sock to parse out the header status code and length and other fields, and then receive the http body according to the length
static int socket_recv(int sock, char *buff, int len, int timeover, int isSsl, int isHttp)
{
	int index = 0;
	unsigned int tick1;
	char msg[32];
	char *phttpbody;
	int nHttpBodyLen;
	int nHttpHeadLen;

	printf("https_recv\n");

	tick1 = Sys_TimerOpen(timeover);

	// Receive http packets cyclically until timeout or reception is complete
	while (Sys_TimerCheck(tick1) > 0 && index < len ){	
		int ret;
		int num;

		if(isSsl)
			num = (timeover - Sys_TimerCheck(tick1)) /1000;
		else
			num = Sys_TimerCheck(tick1) /1000;
			
		num = num < 0 ? 0 : num;
		sprintf(msg , "%s(%d)" , "Recving" , num);

		comm_page_set_page( "Http", msg , 1);
		ret = comm_page_get_exit();

		if(ret == 1){ 
			return -2;
		}

        if(isSsl) 
        {
            ret = comm_ssl_recv( sock, (unsigned char *)(buff + index), len - index );
        } else 
        {
            ret = comm_sock_recv( sock, (unsigned char *)(buff + index), len - index , 1000);
        }

		// printf("ret from comm_sock_recv : %d\n", ret);

		// if(ret) return ret;		// Temporarily

	
		if(isHttp){

			if(ret >= 0){
				index += ret;

				buff[index] = '\0';
				phttpbody = (char *)strstr( buff ,"\r\n\r\n" ); // Http headend
				
				if(phttpbody!=0){
					//char *p;				
					int nrescode = getHttpStatusCode(buff);
					printf( "Status Code: %d",nrescode );

					if ( nrescode == 200){	

						nHttpHeadLen = phttpbody + 4 - buff;	

						nHttpBodyLen = getContentLength(buff);

						printf("HeadLen:%d  Content-Length: %d\n", nHttpHeadLen, nHttpBodyLen);

						//if(nHttpBodyLen<=0) return -1;

						if (index >= nHttpHeadLen + nHttpBodyLen){		
							//The receiving length is equal to the length of the head plus
							memcpy(buff , phttpbody + 4 , nHttpBodyLen);
							printf( "http recv body: %s", buff );
							return nHttpBodyLen;	// Receive completed, return to receive length
						}
						return ret;
					}
					else{  //not 200

						return -1;
					}
				}
							

			}else {
				return -1;
			}
		} else if(ret >= 0) {

			// More work need to be done here in case all data don't come at once 
			
			if(ret > 0) {
				index += ret;
			}

			if (allDataReceived((unsigned char*)buff, index)){
				return index;
			} 
			
		}

			
	}

	return -1;
}


/*
enum CommsStatus {
   
    SEND_RECEIVE_SUCCESSFUL,
    CONNECTION_FAILED,
    SENDING_FAILED,
    RECEIVING_FAILED,

    //Extended Response, leave this part for me.
    MANUAL_REVERSAL_NEEDED,
    AUTO_REVERSAL_SUCCESSUL, 

};
*/


static short tryConnection(NetWorkParameters *netParam, const int i)
{
	char tmp[32] = {0};
	int result;

	m_connect_tick = Sys_TimerOpen(30000);
	m_connect_exit = 0;
	m_connect_time = i + 1;

	sprintf(tmp, "Connecting (%d) ...", i + 1);
	comm_page_set_page("Http", tmp, 1);

	//comm_page_set_page("COM", tmp , 1);

	if (netParam->isSsl)
	{

		//for(;;) 
		//{
			result = comm_ssl_init(COMM_SOCK, netParam->serverCert, netParam->clientCert, netParam->clientKey, netParam->verificationLevel);
			//LOG_PRINTF("Ignoring ssl init return -> %d, comm socket -> %d", result, COMM_SOCK);
			//if (result == 0) break;
			//Sys_Delay(5000);
		//}
		

		sprintf(tmp , "Connect server %d times" , i + 1);
		comm_page_set_page("SSL", tmp , 1);
		

		//TODO: Check callback later.

		if (comm_ssl_connect2(COMM_SOCK, netParam->host, netParam->port, _connect_server_func_proc))
		{
			LOG_PRINTF("Ssl connect failed...\n");
			comm_ssl_close(COMM_SOCK);
			return -4;
		}

		if (comm_page_get_exit() || m_connect_exit == 1)
		{
			LOG_PRINTF("comm_func_connect_server Cancel");
			comm_ssl_close(COMM_SOCK);
			return -5;
		}
	}
	else
	{
		if (comm_sock_connect(COMM_SOCK, netParam->host, netParam->port)) //  Connect to http server
		{
			LOG_PRINTF("Socket connect failed...\n");
			comm_sock_close(COMM_SOCK);
			return -6;
		}
	}

	return 0;
}


static short connectToHost(NetWorkParameters *netParam)
{
    int i;
    int nret;	
	const int nTime = 3;

			
	nret = comm_net_link(netParam->title, netParam->apn, netParam->netLinkTimeout);		// Network link with 30s timeout
	
	if (nret != 0)
	{
		gui_messagebox_show("NET LINK" , "Link Fail", "" , "Exit" , 0);
		return -1;
	}

	if(COMM_SOCK = comm_sock_create(0))
	{
		// Failed to create socket
        gui_messagebox_show("SOCKET" , "Socket Failed", "" , "Exit" , 0);
        printf("Failed to create socket\n");
		return -2;
	}

	Sys_Delay(1000);

	for (i = 0; i < nTime; i++)
	{
		if (!tryConnection(netParam, i)) break;
		Sys_Delay(500);
	}

	if (i == nTime) {
		gui_messagebox_show(netParam->isSsl ? "HTTPS" : "HTTP" , "Connection Fail", "" , "Exit" , 0);
		return -3;
	}

	return 0;
}


static short sendPacket(NetWorkParameters *netParam)
{
	int result = -1;

	printf("packet size to send -> %d\n", netParam->packetSize);
	if (netParam->isSsl)
	{
		result = comm_ssl_send(COMM_SOCK, netParam->packet, netParam->packetSize);
	}
	else
	{
		result = comm_sock_send(COMM_SOCK, (unsigned char *)netParam->packet, netParam->packetSize);
	}

	printf("Send Result : %d\n", result);

	return result > 0 ? 0 : -1;
}


static int http_recv_buff(char *s_title,char *recvBuff,int maxLen,unsigned int tick1,int timeover, int ssl_flag)
{
	int nret;
	int curRecvLen = 0;
	char msg[32];
	char title[32];
	unsigned int tick2 = Sys_TimerOpen(1000);

	printf( "------http_recv_buff------\r\n" );		
	while(Sys_TimerCheck(tick1) > 0){
		int ret;
		int num;

		if(strlen(s_title)>0){
			num = Sys_TimerCheck(tick1)/1000;
			num = num < 0 ? 0 : num;

			sprintf(msg , "%s(%d)" , "Recving" , num);

			comm_page_set_page(s_title , msg , 1);
			ret = comm_page_get_exit();

			if(ret == 1){ 
				return -2;
			}
		}


		if(ssl_flag==1){
			nret = comm_ssl_recv( COMM_SOCK, (unsigned char *)(recvBuff + curRecvLen), maxLen-curRecvLen);
		}else{
			nret = comm_sock_recv( COMM_SOCK, (unsigned char *)(recvBuff + curRecvLen), maxLen-curRecvLen , 700);
		}

		if (nret >0){
			tick2 = Sys_TimerOpen(1000);
			curRecvLen+=nret;
			if(curRecvLen == maxLen){
				break;
			}else if(allDataReceived(recvBuff, curRecvLen)){
				break;
			}
		}else if(Sys_TimerCheck(tick2) == 0 && ssl_flag==1){
			mf_ssl_recv3(COMM_SOCK);
			printf("----------mf_ssl_recv3\r\n");
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
	const int size = sizeof(netParam->response);
	int timeover = 60000;
	unsigned int tick = Sys_TimerOpen(timeover);
	


	bytes = http_recv_buff(netParam->title, netParam->response, size, tick, timeover, netParam->isSsl);

	if (bytes > 0) {
		netParam->responseSize = bytes;
	} else {
		return -1;
	}


	/*
	if (allDataReceived((unsigned char*)buff, index)){
				return index;
	}
	*/
	
	while (1) 
	{
		if (netParam->isSsl)
		{
			result = comm_ssl_recv(COMM_SOCK, (unsigned char *) &netParam->response[bytes], size - bytes);
		}
		else
		{
			result = comm_sock_recv(COMM_SOCK, (unsigned char *)&netParam->response[bytes], size - bytes, netParam->receiveTimeout);
			
		}

		if (result == 0) {
			netParam->responseSize = bytes;
			logHex(netParam->response, netParam->responseSize, "recv");
			break;
		} else if (result > 0) {
			printf("recv: %d bytes received\n", result);
			bytes += result;
		} else if (result < 0) {
			printf("Negative receive result -> %d\n", result);
		}

		Sys_Delay(100);
		
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
	netParam->port = mParam.nibss_port;
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
	logNetworkParameters(netParam);

	if (connectToHost(netParam))
	{
		return CONNECTION_FAILED;
	}

	puts("\nConnection Successful!\n");

	if (sendPacket(netParam))
	{
		return SENDING_FAILED;
	}

	puts("Sending Successful!\n");

	if (receivePacket(netParam))
	{
		return RECEIVING_FAILED;
	}

	puts("Receive Successful!\n");

	netParam->isSsl ? comm_ssl_close(COMM_SOCK) : comm_sock_close(COMM_SOCK);
	comm_net_unlink(); // Unlink network connection
	return SEND_RECEIVE_SUCCESSFUL;
}
