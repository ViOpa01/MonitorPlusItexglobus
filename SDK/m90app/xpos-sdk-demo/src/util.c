#include "util.h"
#include "network.h"
#include "string.h"
#include "stdio.h"


int bin2hex(unsigned char* pcInBuffer, char* pcOutBuffer, int iLen)
{

    int iCount;
    char* pcBuffer;
    unsigned char* pcTemp;
    unsigned char ucCh;

    memset(pcOutBuffer, 0, sizeof(pcOutBuffer));
    pcTemp = pcInBuffer;
    pcBuffer = pcOutBuffer;
    for (iCount = 0; iCount < iLen; iCount++) {
        ucCh = *pcTemp;
        pcBuffer += sprintf(pcBuffer, "%02X", (int)ucCh);
        pcTemp++;
    }

    return 0;
}

void getTerminalSn(char *sn) 
{
    char buffer[40] = {0};
    if(!Sys_GetTermSn(buffer))
    {
        strncpy(sn, buffer, strlen(buffer));
    
    }

}

// Parse Http Head Tag
char *parseHttpHeadValue( char *heads ,const char *head )
{
	char *pret = (char *)strstr( heads ,head );

	if ( pret != 0 )	
    {
		pret += strlen(head);
		pret += strlen(": ");
	}
	return pret;

}

// get content lenght
int getContentLength(char *heads)
{
	char *pContentLength = parseHttpHeadValue( heads ,"Content-Length" );

	if ( pContentLength != 0)
    {
		return atoi( pContentLength );
	}

	return -1;
}

int getHttpStatusCode(char *heads)
{	
	char *rescode = (char *)strstr( heads ," " );
	if ( rescode != 0 )	{
		rescode+=1;
		return atoi( rescode);
	}
	else{
		
	}

	return -1;
}




