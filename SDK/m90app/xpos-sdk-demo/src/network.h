#ifndef _NETWORK_INCLUDED
#define _NETWORK_INCLUDED

//#include "Nibss8583.h"

enum CommsStatus {
   
    SEND_RECEIVE_SUCCESSFUL,
    CONNECTION_FAILED,
    SENDING_FAILED,
    RECEIVING_FAILED

};

typedef struct
{
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
    int sendTimeout;
    int receiveTimeout;

} NetWorkParameters;

void logNetwork(const NetWorkParameters *param)
{
    printf("Host : %s\n", param->host);
    printf("Port : %d\n", param->port);
    printf("isSsl : %d\n", param->isSsl);
    printf("isHttp : %d\n", param->isHttp);
}


enum CommsStatus sendAndRecvDataSsl(NetWorkParameters *netParam);

#endif 


