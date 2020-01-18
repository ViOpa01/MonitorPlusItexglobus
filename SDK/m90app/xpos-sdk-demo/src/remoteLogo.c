
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "remoteLogo.h"
#include "itexFile.h"
#include "util.h"
#include "network.h"


static void setUpNetworkParam(NetWorkParameters *netParam)
{
    char domain[] = "merchant.payvice.com";

    
	strncpy(netParam->title, "LOGO", 10);
	strncpy(netParam->apn, "CMNET", 10);
    // strncpy(netParam->apn, "web.gprs.mtnnigeria.net", sizeof(netParam->apn));
    strncpy(netParam->host, domain, strlen(domain));
    netParam->port = 80;

    netParam->receiveTimeout = 10000;
	netParam->netLinkTimeout = 30000;
    netParam->isHttp = 1;
    netParam->isSsl = 0;
}

int downloadRemoteLogo(const char *tid)
{
    NetWorkParameters netParam = {0};
    char path[64]=  {'\0'};
    char response[1024 * 17] = {0};
    int statusCode = 0;
    int bIndex = 0;
    int contentLenght = 0;

    
    removeFile(BANKLOGO);

    if(!(*tid))
    {
        printf("Invalid tid\n");
        return -1;
    }

    printf("TID : %s\n", tid);

    setUpNetworkParam(&netParam);
    sprintf(path, "external-assets/logos/%.4s.bmp", tid);

    netParam.packetSize  = 0;
	netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "%s /%s HTTP/1.1\r\n", "GET", path);
 	netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n\r\n", netParam.host, netParam.port);

    printf("request : \n%s\n", netParam.packet);
    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return -1;
    }

    memcpy(response, netParam.response, (size_t)netParam.responseSize);
    printf("Response : %s\n", response);
    printf("Response lenght : %d\n", netParam.responseSize);

    statusCode = getHttpStatusCode(response);
	if (statusCode < 200 || statusCode > 299) {
		printf("Error Status");
		return -1;
	}

    printf("Status Code : %d\n", statusCode);

    if(statusCode == 200)
    {
        contentLenght = getContentLength(response);
        printf("Content Lenght : %d\n", contentLenght);
    }

    bIndex = bodyIndex(response);
    printf("Body index : %d\n", bIndex);

    
    return saveRecord((void *)bodyPointer(response), BANKLOGO, contentLenght, 0);
   
}

// int saveRecord(const void *parameters, const char *filename, const int recordSize, const int flag) flag = 0