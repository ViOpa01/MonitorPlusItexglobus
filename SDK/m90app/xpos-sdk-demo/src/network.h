#ifndef _NETWORK_INCLUDED
#define _NETWORK_INCLUDED

//#include "Nibss8583.h"

typedef enum NetType {
    NET_EPMS_SSL = 1,
    NET_POSVAS_SSL = 2,
    NET_EPMS_PLAIN,
    NET_POSVAS_PLAIN,

    NET_EPMS_SSL_TEST,
    NET_EPMS_PLAIN_TEST,
    NET_POSVAS_SSL_TEST,
    NET_POSVAS_PLAIN_TEST,
    
    UPSL_DIRECT_TEST

}NetType;

enum CommsStatus {
   
    SEND_RECEIVE_SUCCESSFUL,
    CONNECTION_FAILED,
    SENDING_FAILED,
    RECEIVING_FAILED

};

typedef struct NetWorkParameters
{
    unsigned char packet[4096 * 3];
    unsigned char response[4096 * 6];
    int packetSize;  
    int responseSize;
    unsigned char host[64];
    int port;
    int isSsl;
    int isHttp;
    int async;
    int socketFd;
    int socketIndex;
    //char apn[50];
    char title[35];
    //int netLinkTimeout;
    int receiveTimeout;

    char serverCert[256];
	char clientCert[256];
	char clientKey[256];
	int verificationLevel; //0 no verification, 1: verify

    const char * endTag;
} NetWorkParameters;

typedef struct {
    char apn[64];
    char imsi[17];
    char username[16];
    char password[16];
    int mode;
    int slot;
    int timeout;
    char operatorName[35];
} Network;


/**
 * Function: netLink
 * usage: netLink(&netparam);
 * --------------------------
 *
 */

short netLink(Network * gprsSettings);

short getNetParams(NetWorkParameters * netParam, NetType netType, int isHttp);
enum CommsStatus sendAndRecvPacket(NetWorkParameters *netParam);
int imsiToNetProfile(Network* profile, const char* imsi);
void * preDial(void * netParams);
short gprsInit(void);
void platformAutoSwitch(NetType *netType);

#endif 


