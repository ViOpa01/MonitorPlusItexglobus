#include "network.h"
#include "util.h"
#include "sdk_http.h"
#include "libapi_xpos/inc/libapi_comm.h"
#include "libapi_xpos/inc/libapi_gui.h"

#define COMM_SOCK	m_comm_sock

static int m_connect_tick = 0;
static int m_connect_time = 0;
static int m_connect_exit = 0;
static int m_comm_sock = 1;


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


// Http receiving processing, the step is to receive the byte stream from the sock to parse out the header status code and length and other fields, and then receive the http body according to the length
static int socket_recv(int sock, char *buff, int len, int timeover, int isSsl)
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
		// 

        if(isSsl) 
        {
            ret = comm_ssl_recv( sock, (unsigned char *)(buff + index), len - index );
        } else 
        {
            ret = comm_sock_recv( sock, (unsigned char *)(buff + index), len - index , 700);
        }
		
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
                        

		}
		else {
			return -1;
		}	
	}

	return -1;
}

enum CommsStatus sendAndRecvDataSsl(NetWorkParameters *netParam)
{
	char buff[0x1000]={0};
	// char recv[2048]={0};
	int ret = -1;
	int sock = 0;
	char apn[32] = "CMNET";
    int i;
    int nret;	
	int nTime = 3;

			
	nret = comm_net_link("", apn, 30000);		// Network link with 30s timeout
	if (nret != 0)
	{
		gui_messagebox_show("NET LINK" , "Link Fail", "" , "Exit" , 0);
		return;
	}

	if(COMM_SOCK = comm_sock_create(0))
	{
		// Failed to create socket
        gui_messagebox_show("SOCKET" , "Socket Failed", "" , "Exit" , 0);
        printf("Failed to create socket\n");
		return;
	}

	for ( i = 0 ; i < nTime ; i++)
	{		
		char tmp[32] = {0};

		m_connect_tick = Sys_TimerOpen(30000);
		m_connect_exit = 0;
		m_connect_time = i + 1;

		printf("connect server... ip = %s, port = %d\r\n", netParam->host,  netParam->port);
		
		sprintf(tmp , "Connection (%d) ..." , i + 1);
		comm_page_set_page("Http", tmp , 1);

		if(netParam->isSsl) 
		{
			printf("--------------------comm_ssl_init---------------------------\r\n" );
			comm_ssl_init(COMM_SOCK ,0,0,0,0);

			ret = comm_ssl_connect2(COMM_SOCK, netParam->host, netParam->port, _connect_server_func_proc);
			printf( "--------------------comm_ssl_connect: %d ---------------------------\r\n", ret );

			if (comm_page_get_exit() || m_connect_exit == 1)	{
				printf("comm_func_connect_server Cancel" );
				comm_ssl_close(COMM_SOCK);
				ret = -1;
				break;
			}

		} else {
			ret = comm_sock_connect(COMM_SOCK, netParam->host, netParam->port);	//  Connect to http server

		}

		
		printf( "--------------------comm_sock_connect: %d ---------------------------\r\n", ret );
		if (ret == 0)			
			break;
		
		if(netParam->isSsl)
		{
			comm_ssl_close(COMM_SOCK);
		} else {
			comm_sock_close(COMM_SOCK);
		}
		Sys_Delay(500);

	}	


	if(nTime <= 3 && ret)
	{
		printf("Failed to connect to the host\n");
		gui_messagebox_show(netParam->isSsl ? "HTTPS" : "HTTP" , "Connection Fail", "" , "Exit" , 0);
		return CONNECTION_FAILED;
	}

    printf("connect server ret : %d, %s:%d\r\n" , ret , netParam->host , netParam->port);

	printf("------------------------Sending request to the host-----------------------\n");
	if(ret == 0) 
	{

		if(netParam->isSsl) 
		{
			ret = comm_ssl_send(COMM_SOCK , netParam->packet ,  netParam->packetSize);
		} else {
			ret = comm_sock_send(COMM_SOCK , (unsigned char *)netParam->packet  ,  netParam->packetSize);	
		}

    /*
		if(ret)
		{
			printf("Failed to send the request, ret = %d\n", ret);
			return SENDING_FAILED;
		}
    */
	}

	printf("-------------------------------Receiving from the server----------------------------\n");

	ret = socket_recv(COMM_SOCK, netParam->response,  sizeof(netParam->response), 30000, netParam->isSsl);  // check for len very well
    netParam->responseSize = ret;   // getting the response length

	if(ret > 0) 
	{
		printf("Receive legth : %d\n", ret);
		printf("recv buff: %s\n", netParam->response);	

	} else {
		printf("No response received\n");
		gui_messagebox_show("RESPONSE" , "No Response received", "" , "Exit" , 0);
		return RECEIVING_FAILED;
	}

	if(netParam->isSsl)
	{
		comm_ssl_close(COMM_SOCK);
	} else {
		comm_sock_close(sock);
	}
		   
	comm_net_unlink();		// Unlink network connection
    
    return SEND_RECEIVE_SUCCESSFUL;

}



