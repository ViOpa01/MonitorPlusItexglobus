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
    int receiveTimeout;

    char serverCert[256];
	char clientCert[256];
	char clientKey[256];
	int verificationLevel; //0 no verification, 1: verify
} NetWorkParameters;






enum CommsStatus sendAndRecvPacket(NetWorkParameters *netParam);

#endif 


