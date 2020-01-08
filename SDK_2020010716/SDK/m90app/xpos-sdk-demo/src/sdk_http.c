//#include "sdk_log.h"
#include "sdk_http.h"
#include "libapi_xpos/inc/libapi_comm.h"
#include "libapi_xpos/inc/libapi_gui.h"

#define COMM_SOCK	m_comm_sock

static int m_connect_tick = 0;
static int m_connect_time = 0;
static int m_connect_exit = 0;
static int m_comm_sock = 1;

// Pack the http header, the following code is the demo code, please refer to the http specification for the specific format.
static void http_pack(char * buff, char * msg)
{
	int index = 0;
	int msgl = strlen(msg);

	index += sprintf(buff + index , "POST %s HTTP/1.1\r\n", "/mpos/handshake");
	index += sprintf(buff + index , "HOST: %s\r\n" , "test.mosambee.in" );//www.baidu.com
	index += sprintf(buff + index , "Accept: */*\r\n");
	index += sprintf(buff + index , "Connection: Keep-Alive\r\n");
	index += sprintf(buff + index , "Content-Length: %d\r\n", msgl);
	//index += sprintf(buff + index , "Content-Type: text/plain;charset=UTF-8\r\n");	
	index += sprintf(buff + index , "Content-Type: application/x-www.form-urlencoded\r\n");	
	index += sprintf(buff + index , "cache-control: no-cache\r\n");	
	index += sprintf(buff + index , "\r\n");

	index += sprintf(buff + index , "%s", msg);
}

// Parse the field of http
static char *http_headvalue( char *heads ,const char *head )
{
	char *pret = (char *)strstr( heads ,head );
	if ( pret != 0 )	{
		pret += strlen(head);
		pret += strlen(": ");
	}
	return pret;

}

// Parsing the status code of http
static int http_StatusCode(char *heads)
{	
	char *rescode = (char *)strstr( heads ," " );
	
	printf( "---------http_StatusCode %s\r\n", heads );		

	if ( rescode != 0 )	{
		rescode+=1;
		return atoi( rescode);
	}
	else{
		printf( "-----------http_StatusCode error %s\r\n", heads );		
	}

	return -1;
}

// Parse the length of http
static int http_ContentLength(char *heads)
{
	char *pContentLength = http_headvalue( heads ,"Content-Length" );
	if ( pContentLength != 0)	{
		return atoi( pContentLength );
	}
	return -1;
}

void comm_page_set_page(char * title , char * msg ,int quit)
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

int comm_page_get_exit()
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
		comm_page_set_page("Https", msg , 1);
	}
	else{
		sprintf(msg , "Canceling...");
		comm_page_set_page("Https", msg , 0);
		ret = 2;
	}
	

	return ret ;
}

static int my_pow(int x,int y)
{
	int i;
	int iRet = 0;

	if(y==0) return 1;

	while(y>0){
		if(iRet==0){
			iRet = x;
		}else{
			iRet = iRet*x;
		}
		y--;
	}

	return iRet;
}

int str_hex_to_int(char *data)
{
	int len = strlen(data);
	int i;
	int iRet = 0;

	for(i=0;i<len;i++){
		char hexdata;

		if(data[i] >= '0' && data[i] <= '9'){
			hexdata = data[i]-'0';
		}else if(data[i] >= 'A' && data[i] <= 'F'){
			hexdata = data[i]-'A'+10;
		}else if(data[i] >= 'a' && data[i] <= 'f'){
			hexdata = data[i]-'a'+10;
		}else{
			return -1;
		}

		iRet += hexdata*my_pow(16,len-i-1);
	}

	return iRet;
}

/*
// Http receiving processing, the step is to receive the byte stream from the sock to parse out the header status code and length and other fields, and then receive the http body according to the length
static int http_recv(int sock, char *buff, int len, int timeover)
{
	int index = 0;
	unsigned int tick1;
	char msg[32];
	char *phttpbody;
	int nHttpBodyLen;
	int nHttpHeadLen;

	tick1 = Sys_TimerOpen(timeover);

	// Receive http packets cyclically until timeout or reception is complete
	while (Sys_TimerCheck(tick1) > 0 && index < len ){	
		int ret;
		int num;

		num = Sys_TimerCheck(tick1) /1000;
		num = num < 0 ? 0 : num;
		sprintf(msg , "%s(%d)" , "Recving" , num);

		comm_page_set_page( "Http", msg , 1);
		ret = comm_page_get_exit();

		if(ret == 1){ 
			return -2;
		}
		// 
		ret = comm_sock_recv( sock, (unsigned char *)(buff + index), len - index , 700);

		if(ret >= 0){
			index += ret;

			buff[index] = '\0';
			phttpbody = (char *)strstr( buff ,"\r\n\r\n" ); // Http headend
			if(phttpbody!=0){
				//char *p;				
				int nrescode = http_StatusCode(buff);
				printf( "http rescode: %d",nrescode );
				if ( nrescode == 200){	

					nHttpHeadLen = phttpbody + 4 - buff;	

					nHttpBodyLen = http_ContentLength(buff);

					printf("HeadLen:%d  Content-Length: %d",nHttpHeadLen,nHttpBodyLen);

					//if(nHttpBodyLen<=0) return -1;

					if (index >= nHttpHeadLen+nHttpBodyLen){		
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
                        

		}
		else {
			return -1;
		}	
	}

	return -1;
}

static int https_recv(int sock, char *buff, int len, int timeover)
{
	int index = 0;
	unsigned int tick1;
	char msg[32];
	char *phttpbody;
	int nHttpBodyLen;
	int nHttpHeadLen;

	printf( "https_recv");

	tick1 = Sys_TimerOpen(timeover);

	// Receive http packets cyclically until timeout or reception is complete
	while (Sys_TimerCheck(tick1) > 0 && index < len ){	
		int ret;
		int num;

		num = (timeover - Sys_TimerCheck(tick1)) /1000;
		num = num < 0 ? 0 : num;
		sprintf(msg , "%s(%d)" , "Recving" , num);

		comm_page_set_page( "Https", msg , 1);
		ret = comm_page_get_exit();

		if(ret == 1){ 
			return -2;
		}
		// 
		ret = comm_ssl_recv( sock, (unsigned char *)(buff + index), len - index );

		if(ret >= 0){
			index += ret;

			buff[index] = '\0';
			phttpbody = (char *)strstr( buff ,"\r\n\r\n" ); // Http headend
			if(phttpbody!=0){
				//char *p;				
				int nrescode = http_StatusCode(buff);
				printf( "https rescode: %d",nrescode );
				if ( nrescode == 200){	

					nHttpHeadLen = phttpbody + 4 - buff;	

					nHttpBodyLen = http_ContentLength(buff);

					printf("HeadLen:%d  Content-Length: %d",nHttpHeadLen,nHttpBodyLen);

					//if(nHttpBodyLen<=0) return -1;

					if (index >= nHttpHeadLen+nHttpBodyLen){		
						//The receiving length is equal to the length of the head plus
						memcpy(buff , phttpbody + 4 , nHttpBodyLen);
						printf( "https recv body: %s", buff );
						return nHttpBodyLen;	// Receive completed, return to receive length
					}
					return ret;
				}
				else{  //not 200

					return -1;
				}
			}


		}
		else {
			return -1;
		}	
	}

	return -1;
}*/

int http_recv_buff(char *s_title,char *recvBuff,int maxLen,unsigned int tick1,int timeover, int ssl_flag)
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
			}else if(strncmp(recvBuff,"HTTP/",5) == 0 && strstr(recvBuff,"\r\n\r\n") > 0){
				//recv head over, return
				char *p = strstr(recvBuff,"\r\n\r\n");
				if(strlen(p+4)>=10){
					break;//Receive 10 more bytes to ensure parsing length
				}
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

int http_recv(char *title, char *recv , int maxLen , int timeover, int ssl_flag)
{
	int ret;
	char *sHead = malloc(512+1);
	unsigned int tick = Sys_TimerOpen(timeover);
	int ContentLength = -1;
	char *p1;
	char *p2;
	char tmp[10] = {0};
	int headLen = 0; 
	int bodylen = 0;

	printf( "-------------http_recv-------------\r\n" );		

	memset(sHead,0,512+1);
	//recv head
	ret = http_recv_buff(title, sHead, 512, tick, timeover, ssl_flag);
	if(ret <= 0){
		free(sHead);
		return -1;
	}

	printf( "-----------sHead: %s\r\n", sHead );		

	if(http_StatusCode(sHead)!=200){
		free(sHead);
		return -2;
	}

	p1 = strstr(sHead,"Content-Length: ");
	if(p1 > 0){
		p2 = strstr(p1,"\r\n");
		if(p2 > 0){
			int ilen = p2-p1-strlen("Content-Length: ");
			if(ilen < 10){
				memcpy(tmp,p1+strlen("Content-Length: "),ilen);
				ContentLength = atoi(tmp);
			}
		}
	}

	p1 = strstr(sHead,"\r\n\r\n");
	if(p1 <= 0){
		free(sHead);
		return -1;
	}

	if(ContentLength > 0){
		bodylen = strlen(p1+4);
		memcpy(recv,p1+4,bodylen);
	}else{
		//not return length in the head
		char *p2 = strstr(p1+4,"\r\n");
		if(p2>0 && p2-p1-4  < 10){
			char tmplen[20] = {0};
			memcpy(tmplen,p1+4,p2-p1-4);
			ContentLength = str_hex_to_int(tmplen);
			bodylen = strlen(p2+2);
			memcpy(recv,p2+2,bodylen);
		}else{
			free(sHead);
			return -1;
		}
	}

	printf( "--------ContentLength: %d, bodylen: %d\r\n", ContentLength, bodylen );		

	if(bodylen >= ContentLength){
		free(sHead);
		return 0;
	}

	//recv body
	ret = http_recv_buff(title, recv+bodylen, ContentLength-bodylen, tick, timeover, ssl_flag);
	if(ret >= ContentLength-bodylen){
		free(sHead);
		return 0;
	}else{
		free(sHead);
		return -1;
	}

	free(sHead);
	return ret;
}


static void http_test()
{
	char *msg = "hello world!";
	char buff[2048]={0};
	char recv[2048]={0};
	char *ip = "www.baidu.com";//119.75.217.109
	int port = 80;
	int ret = -1;
	int sock = 0;
	char apn[32]="CMNET";
    int i;
    int nret;	
	int nTime = 3;

	http_pack(buff, msg);		//  Package http data
			
	nret = comm_net_link("Http", apn ,30000);		// 30s
	if (nret != 0)
	{
		gui_messagebox_show("Http" , "Link Fail", "" , "confirm" , 0);
		return;
	}

	COMM_SOCK = comm_sock_create(0);	//  Create sock


	for ( i = 0 ; i < nTime ; i ++)
	{		
		char tmp[32] = {0};

		m_connect_tick = Sys_TimerOpen(30000);
		m_connect_exit = 0;
		m_connect_time = i+1;

		printf("connect server ip=%s, port=%d\r\n", ip,  port);
		
		sprintf(tmp , "Connect server%dtimes" , i + 1);
		comm_page_set_page("Http", tmp , 1);

		ret = comm_sock_connect(sock, ip, port);	//  Connect to http server
		printf( "--------------------comm_sock_connect: %d ---------------------------\r\n", ret );
		if (ret == 0)			
			break;
		
		comm_sock_close(COMM_SOCK);
		Sys_Delay(500);
	}

	

	printf("connect server ret= %d,%s:%d\r\n" , ret , ip , port);

	if(ret == 0){
		comm_sock_send(sock , (unsigned char *)buff ,  strlen(buff));	// 		Send http request
		ret = http_recv("Http", recv, 1024, 30000, 0);		// Receive http response
		if (ret == 0)
		{
			sprintf(buff, "recv buff:%s", recv);
			printf("recv buff:%s", recv);
			gui_messagebox_show("Http" , buff, "" , "confirm" , 0);		
		}
		else
		{
			gui_messagebox_show("Http" , "Recv Fail", "" , "confirm" , 0);
		}
	}
	else {
		gui_messagebox_show("Http" , "Connect Fail", "" , "confirm" , 0);
	}

	comm_sock_close(sock);	   

	comm_net_unlink();
}


static void https_test()
{
	char *msg = "requestMessage=imeiNumber=865789024229520|osId=80dad2df0e73f8dc|applicationId=31|imsiNumber=8991921802931780305|qposSerialNo=171257333221017301489555|make=INGENICO|model=Move2500|appVersion=1.0.0|androidVersion=11.16.05 Patch(b54)|versionCode=0&deviceSerialNumber=0000000000";//080022380000008000009A00000530081329000001081329053020390013
	char buff[2048]={0};
	char recv[2048]={0};
	char *ip = "test.mosambee.in";
	int port = 443;
	int ret = - 1;
	char apn[32]="CMNET";
	int i;
	int nret;	
	int nTime = 3;

	//mbed_ssl_set_log(10); 
	http_pack(buff, msg);		//  Package http data
	
	nret = comm_net_link( "Https" , apn,  30000);
	if (nret != 0)
	{
		gui_messagebox_show("Https" , "Link Fail", "" , "confirm" , 0);
		return;
	}

	COMM_SOCK = comm_sock_create(0);	//  Create sock
	

	for ( i = 0 ; i < nTime ; i ++)
	{
		
		char tmp[32] = {0};

		m_connect_tick = Sys_TimerOpen(30000);
		m_connect_exit = 0;
		m_connect_time = i+1;

		printf("connect server ip=%s, port=%d\r\n", ip,  port);
		printf("--------------------comm_ssl_init---------------------------\r\n" );

		comm_ssl_init(COMM_SOCK ,0,0,0,0);	
		sprintf(tmp , "Connect server%dtimes" , i + 1);
		comm_page_set_page("Https", tmp , 1);

		//c_ret = comm_ssl_connect(COMM_SOCK, ip, port);	//  Connect to http server
		ret = comm_ssl_connect2(COMM_SOCK, ip, port, _connect_server_func_proc);	//  Connect to http server
		printf( "--------------------comm_ssl_connect: %d ---------------------------\r\n", ret );

		if (comm_page_get_exit() || m_connect_exit == 1)	{
			printf("comm_func_connect_server Cancel" );
			comm_ssl_close(COMM_SOCK);
			ret = -1;
			break;
		}

		if (ret == 0){		
			break;
		}

		comm_ssl_close(COMM_SOCK);
		Sys_Delay(500);
	}	


	printf("connect server ret= %d,%s:%d\r\n" , ret , ip , port);

	if(ret == 0){
		comm_ssl_send(COMM_SOCK , buff ,  strlen(buff));	// Send http request		
		printf("send buff:%s\r\n", buff);
		ret = http_recv("Https", recv, 2048, 30000, 1);		// Receive http response
		if (ret == 0)
		{
			sprintf(buff, "recv buff:%s\r\n", recv);
			printf("recv buff:%s\r\n", recv);
			gui_messagebox_show("Https", buff, "", "confirm", 0);
		}
		else
		{
			gui_messagebox_show("Https", "Recv Fail", "", "confirm", 0);
		}
	}
	else
	{
		gui_messagebox_show("Https", "Connect Fail", "", "confirm", 0);
	}		

	comm_ssl_close(COMM_SOCK);

	comm_net_unlink();
}


void sdk_http_test()
{
	http_test();	
}

void sdk_https_test()
{
	https_test();
}