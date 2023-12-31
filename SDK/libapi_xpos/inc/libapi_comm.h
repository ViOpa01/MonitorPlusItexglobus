#pragma once

#include "libapi_pub.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct __st_wifi_ap_list{
	int ecn;
	char ssid[64];//WiFi name
	char utf8ssid[64];
	int rssi;//Signal strength
	char mac[20];
	int channel;
	int freq;
}st_wifi_ap_list;

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Connect Network
Input : title��Tips for connecting to the network
		  apn��gprs apn
		  timeover : Connection timeout
		
Output : Nothing
return: 0,     success
		Other, failure	
*************************************************************************************/
LIB_EXPORT int comm_net_link(char * title, char * apn , int timeover);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Connect Network
Input : title��Tips for connecting to the network
		  apn��gprs apn
		  timeover : Connection timeout
		  user:	gprs apn user id
		  pwd: gprs apn user password
		  auth:Authentication parameter 
Output : Nothing
return: 0,     success
		Other, failure	
*************************************************************************************/
LIB_EXPORT int comm_net_link_ex(char * title, char * apn, int timeover, char *user, char *pwd, int auth);



/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Disconnect from the network
Input : Nothing
Output : Nothing
return: 0,     success
		Other, failure	
*************************************************************************************/
LIB_EXPORT int comm_net_unlink();


/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:create sock
Input : index(0/1)
Output : Nothing
return: 0,     success
		Other, failure	
*************************************************************************************/
LIB_EXPORT int comm_sock_create(int index);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:connect to the server
Input : index		sock index
		  ip		server ip
		  port		server port
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_sock_connect(int index, char * ip, int port);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Receive data
Input : index		sock index
		  buff		Receive buffer
		  len		Receiving length
		  timeover	overtime time
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_sock_recv(int index, unsigned char * buff, int len, unsigned int timeover);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:send data
Input : index		sock index
		  buff		Send buffer
		  len		Send length
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_sock_send(int index, unsigned char * buff , int size);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Disconnect the server
Input : index		sock index
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_sock_close(int index);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:ssl initialization
Input : index		sock index
	      cacert	Server certificate
		  clientcert Client certificate
		  clientkey	Client key
		  level		Verification level 0=Not verified 1=Verify server certificate
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_ssl_init(int index, char * cacert, char * clientcert, char * clientkey,int level);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:ssl connect to the server
Input : index		sock index
	      ip		server ip
		  port		server port

Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_ssl_connect(int index , char * ip , int port);
/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:ssl connect to the server
Input : index		sock index
	      ip		server ip
		  port		server port
		  func      callback
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/

LIB_EXPORT int comm_ssl_connect2(int index , char * ip , int port,void *func);
/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:ssl send data
Input : index		sock index
	      data		ssl data
		  size		Data size
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_ssl_send(int index, char * pdata, int size);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:ssl Receive data
Input : index		sock index
	      data		ssl data
		  size		Data size
Output : Nothing			
return: 0,     success
		Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_ssl_recv(int index, char * pdata, int size);

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:ssl Disconnect
Input : index		sock index
Output : Nothing			
return: 0,     success
		Other, failure	
*************************************************************************************/
LIB_EXPORT int comm_ssl_close(int index);




/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Get the router list
Input : Nothing	
Output : ap_list	Router list data,	The ap_list space is allocated by the caller with an array size of 10	
return: 	Number of routers
*************************************************************************************/
LIB_EXPORT int comm_wifi_list_ap(st_wifi_ap_list * ap_list);


/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Connecting router
Input : ap_list	Router data	
Input : pwd password	
Output : 
return: 0,     success
Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_wifi_link_ap(st_wifi_ap_list * ap_list , char * pwd);


/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:unlink router
Input : 
Output : 
return: 0,     success
Other, failure		
*************************************************************************************/
LIB_EXPORT int comm_wifi_unlink_ap();

/*************************************************************************************
Copyright: Fujian MoreFun Electronic Technology Co., Ltd.
Author:lx
Functions:Get connection status
Input : 
Output : 
return: 1,     connection
		0 ,    disconnect
*************************************************************************************/
LIB_EXPORT int comm_wifi_get_link_state();



#ifdef __cplusplus
}
#endif	