#pragma once

enum CommsStatus {
   
    SEND_RECEIVE_SUCCESSFUL,
    CONNECTION_FAILED,
    SENDING_FAILED,
    RECEIVING_FAILED,

    //Extended Response, leave this part for me.
    MANUAL_REVERSAL_NEEDED,
    AUTO_REVERSAL_SUCCESSUL, 

};

typedef struct
{
    unsigned char packet[2048];
    unsigned char response[4096];
    int packetSize;     // just request body
    int responseSize;
    unsigned char host[64];
    int port;
    int isSsl;
} NetWorkParameters;


enum CommsStatus sendAndRecvDataSsl(NetWorkParameters *netParam);
