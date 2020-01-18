#include "util.h"
#include "network.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "libapi_xpos/inc/libapi_util.h"



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
/*
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
*/

void logHex(unsigned char * data, const int size, const char * title)
{
    const int ascLen = size * 2 + 1;
    char * asc = (char *) calloc(ascLen, sizeof(char));

    Util_Bcd2Asc((char *) data, asc, ascLen - 1);
    fprintf(stderr, "%s -> %s\r\n", title, asc);

    free(asc);
}

void getDate(char *buff)
{
	char d[32] = {0};
	Sys_GetDateTime(d);
	sprintf(buff, "%c%c%c%c-%c%c-%c%c", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
}

void getTime(char *buff)
{
	char d[32] = {0};
	Sys_GetDateTime(d);
	sprintf(buff, "%c%c:%c%c:%c%c", d[8], d[9], d[10], d[11], d[12], d[13]);
}

void getDateAndTime(char *yyyymmddhhmmss)
{
    Sys_GetDateTime(yyyymmddhhmmss);
    yyyymmddhhmmss[14] = '\0';
}

void MaskPan(char *pszInPan, char *pszOutPan)
{
	int  iPanLen, iCnt, index;
	char  szBuffer[40], szTemp[50];

	iPanLen = strlen((char *)pszInPan);
	iCnt = index = 0;

	memset(szTemp,0,sizeof(szTemp));
	memset(szBuffer,0,sizeof(szBuffer));
	while(iCnt < iPanLen)
	{
		if (iCnt<6 || iCnt+4>= iPanLen)
		{
			szBuffer[index] = pszInPan[iCnt];
		}
		else
		{
			szBuffer[index] = 'X';
		}
		iCnt++;
		index++;
	}
	iCnt = index = 0;

	if (pszOutPan != NULL)
	{
		sprintf((char *)pszOutPan,szBuffer);
	}

}


int getHttpStatusCode(const char* response)
{
    char* token;
    char delim[] = " ";

    char responseCopy[16] = { 0 };

    // Make a copy of the response but avoid copying the whole thing
    strncpy(responseCopy, response, sizeof(responseCopy) - 1);

    // The first token is the http version
    token = strtok(responseCopy, delim);

    // The second token is the status code
    token = strtok(NULL, delim);
    if (!token)
        return -1;

    return atoi(token);
}

// Returns pointer to terminating null character in response argument if body not found
const char* bodyPointer(const char* response)
{
    size_t i;
    const size_t len = strlen(response);

    for (i = 0; i < len - 2; i++, response++) {
        if (*response != '\n')
            continue;
        else if (*(response + 1) == '\r' && *(response + 2) == '\n')
            return (response + 3);
    }

    return NULL; // Index of null terminating character is returned
}

int bodyIndex(const char* response)
{
    size_t i;
    const size_t len = strlen(response);

    for (i = 0; i < len - 2; i++) {
        if (*(response + i) != '\n')
            continue;
        else if (*(response + i + 1) == '\r' && *(response + i + 2) == '\n')
            return i + 3;
    }

    return len;
}

/*
int getContentLength(void* userdata, const char* tag, const int tag_len, const char* value, const int val_len)
{
    int* i;
    int index;
    int space;
    const char* trimVal;
	char hKey[256 + 1] = {0};
	char hValue[1024 + 1] = {0};

    i = (int *)userdata;

    // Convert to lower case while trimming
    for (index = 0; index < tag_len; ++index) {
        hKey[index] = tolower(*(tag + index));
    }

    //trim leading whitespace before picking value
    space = 0;
    while (space < val_len && isspace(*(value + space)))
        ++space;

    //
    for (index = 0; space < val_len; ++space, ++index) {
        hValue[index] = tolower(*(value + space));
    }

	if (!strcmp("content-length", hKey)) {
		*i = atoi(hValue);
        return -1;	// done
    }

	return 0;
}
*/


