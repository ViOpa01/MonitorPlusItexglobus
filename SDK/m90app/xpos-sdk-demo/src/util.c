#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "util.h"
#include "network.h"
#include "atc_pub.h"

#include "libapi_xpos/inc/libapi_util.h"


static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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

int hex2bin(const char *pcInBuffer, char *pcOutBuffer, int iLen)
{
	int 	c;
	char	*p;
	int 	iBytes=0;
	char *hextable = "0123456789ABCDEF";
	int nibble = 0;
	int nibble_val;

	while(iBytes < iLen)	{
		c = *pcInBuffer++;
		if (c == 0)
			break;

		c = toupper(c);

		p = strchr(hextable, c);
		if (p)
		{
			if (nibble & 1)
			{
				iBytes++;
				*pcOutBuffer = (nibble_val << 4 | (p - hextable));
				pcOutBuffer++;
			}//if
			else
			{
				nibble_val = (p - hextable);
			}//else
			nibble++;
		}//if
		else
		{
			nibble = 0;
		}//else
	}//while
	
	return iBytes;
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


/**
 * Function: beautifyDateTime
 * Usage:
 * ---------------------------
 *
 */

short beautifyDateTime(char * dbDate, const int size, const char * yyyymmddhhmmss) 
{
    const char dateSeparator = '-';
    const char timeSeparator = ':';

    if (size < 24) {
        fprintf(stderr, "Not enough buffer to format date.\n");
        return -1;
    }

    //yyyy
    strncpy(dbDate, yyyymmddhhmmss, 4);
    dbDate[4] = dateSeparator;

    //mm
    strncpy(&dbDate[5], &yyyymmddhhmmss[4], 2);
    dbDate[7] = dateSeparator;

    //dd
    strncpy(&dbDate[8], &yyyymmddhhmmss[6], 2);
    dbDate[10] = ' ';

    //hh
    strncpy(&dbDate[11], &yyyymmddhhmmss[8], 2);
    dbDate[13] = timeSeparator;

    //mm
    strncpy(&dbDate[14], &yyyymmddhhmmss[10], 2);
    dbDate[16] = timeSeparator;

    //ss
    strncpy(&dbDate[17], &yyyymmddhhmmss[12], 2);
    dbDate[19] = '.';

    //hh
    strncpy(&dbDate[20], "000", 3);
    dbDate[23] = 0;

    return 0; 
}

void getFormattedDateTime(char* dateTime, size_t len)
{

    char date[15] = {'\0'};
    char time[15] = {'\0'};

    getDate(date);
    getTime(time);
    snprintf(dateTime, len, "%s %s", date, time);
}

void getImsi(char buff[20])
{
    ap_get_imsi(buff, 20);
}

void getMcc(char buff[4])
{
    char imsi[20] = {'\0'};
    ap_get_imsi(imsi, 20);

    strncpy(buff, imsi, 3);
}

void getMnc(char buff[3])
{
    char imsi[20] = {'\0'};
    ap_get_imsi(imsi, 20);

    strncpy(buff, &imsi[3], 2);
}

const char* getSimId()
{
    return atc_iccid();
}

int getSignalLevel()
{
    return atc_signal();
}

int getCellId()
{
    return atc_cell();
}

int getLocationAreaCode()
{
    return atc_lac();
}

unsigned char* base64_encode(const unsigned char* src, size_t len, size_t* out_len)
{
    unsigned char *out, *pos;
    const unsigned char *end, *in;
    size_t olen;
    int line_len;

    olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen += olen / 72; /* line feeds */
    olen++; /* nul termination */
    if (olen < len)
        return NULL; /* integer overflow */
    out = malloc(olen);
    if (out == NULL)
        return NULL;

    end = src + len;
    in = src;
    pos = out;
    line_len = 0;
    while (end - in >= 3) {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
        line_len += 4;
        if (line_len >= 72) {
            //*pos++ = '\n';
            line_len = 0;
        }
    }

    if (end - in) {
        *pos++ = base64_table[in[0] >> 2];
        if (end - in == 1) {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        } else {
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
        line_len += 4;
    }

    if (line_len) {
        //*pos++ = '\n';
	}

    *pos = '\0';
    if (out_len)
        *out_len = pos - out;
    return out;
}

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char* base64_decode(const unsigned char* src, size_t len, size_t* out_len)
{
    unsigned char dtable[256], *out, *pos, block[4], tmp;
    size_t i, count, olen;
    int pad = 0;

    memset(dtable, 0x80, 256);
    for (i = 0; i < sizeof(base64_table) - 1; i++)
        dtable[base64_table[i]] = (unsigned char)i;
    dtable['='] = 0;

    count = 0;
    for (i = 0; i < len; i++) {
        if (dtable[src[i]] != 0x80)
            count++;
    }

    if (count == 0 || count % 4)
        return NULL;

    olen = count / 4 * 3;
    pos = out = malloc(olen);
    if (out == NULL)
        return NULL;

    count = 0;
    for (i = 0; i < len; i++) {
        tmp = dtable[src[i]];
        if (tmp == 0x80)
            continue;

        if (src[i] == '=')
            pad++;
        block[count] = tmp;
        count++;
        if (count == 4) {
            *pos++ = (block[0] << 2) | (block[1] >> 4);
            *pos++ = (block[1] << 4) | (block[2] >> 2);
            *pos++ = (block[2] << 6) | block[3];
            count = 0;
            if (pad) {
                if (pad == 1)
                    pos--;
                else if (pad == 2)
                    pos -= 2;
                else {
                    /* Invalid padding */
                    free(out);
                    return NULL;
                }
                break;
            }
        }
    }

    *out_len = pos - out;
    return out;
}

void generateRandomString(char* output, const int ouputSize)
{
    int i = 0;
    char charset[] = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM0123456789";
	static int init = 0;
	
	if (!init) {
		unsigned long seed = time(NULL);
		srand(seed);
		init++;
	}

    for (i = 0; i < ouputSize - 1; ++i) {
        *(output + i) = charset[rand() % (sizeof(charset) - 1)];
    }
    *(output + i) = 0;
}


char* safe_url_encode(const char* s, char* enc, const size_t len, int encoding)
{
    static int isInitialized = 0;
    static char html5[256] = { 0 }; // char set 0
    static char rfc3986[256] = { 0 }; // char set 1

    int i;

	memset(enc, 0, len);

    if (!isInitialized) {
        for (i = 0; i < 256; i++) {
            rfc3986[i] = isalnum(i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
            html5[i] = isalnum(i) || i == '*' || i == '-' || i == '.' || i == '_' ? i : (i == ' ') ? '+' : 0;
        }
        isInitialized = 1;
    }

    for (i = 0; *s && i < len - 3; ++s) {
        char* table = (encoding) ? html5 : rfc3986;
        if (table[*s])
            i += sprintf(enc, "%c", table[*s]);
        else
            i += sprintf(enc, "%%%02X", *s);
        while (*++enc)
            ;
    }

	enc[len - 1] = 0;
    return (enc);
}

int sha512Hex(char* hexoutput, const char* data, const size_t datalen)
{
    SHA512_CTX ctx;
    char md[SHA512_DIGEST_LENGTH];

    SHA512_Init(&ctx);
    SHA512_Update(&ctx, data, datalen);
    SHA512_Final((unsigned char*)md, &ctx);

    bin2hex((unsigned char *)md, hexoutput, SHA512_DIGEST_LENGTH);

    return 0;
}

int sha512Bin(char* digest, const char* data, const size_t datalen)
{
    SHA512_CTX ctx;

    SHA512_Init(&ctx);
    SHA512_Update(&ctx, data, datalen);
    SHA512_Final((unsigned char*)digest, &ctx);

    return 0;
}

void hmac_sha256(
    const unsigned char* text, /* pointer to data stream        */
    int text_len, /* length of data stream         */
    const unsigned char* key, /* pointer to authentication key */
    int key_len, /* length of authentication key  */
    void* digest) /* caller digest to be filled in */
{
    unsigned char k_ipad[65]; /* inner padding -
                                 * key XORd with ipad
                                 */
    unsigned char k_opad[65]; /* outer padding -
                                 * key XORd with opad
                                 */
    unsigned char tk[SHA256_DIGEST_LENGTH];
    unsigned char tk2[SHA256_DIGEST_LENGTH];
    unsigned char bufferIn[1024];
    unsigned char bufferOut[1024];
    int i;

    /* if key is longer than 64 bytes reset it to key=sha256(key) */
    if (key_len > 64) {
        SHA256(key, key_len, tk);
        key = tk;
        key_len = SHA256_DIGEST_LENGTH;
    }

    /*
     * the HMAC_SHA256 transform looks like:
     *
     * SHA256(K XOR opad, SHA256(K XOR ipad, text))
     *
     * where K is an n byte key
     * ipad is the byte 0x36 repeated 64 times
     * opad is the byte 0x5c repeated 64 times
     * and text is the data being protected
     */

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof k_ipad);
    memset(k_opad, 0, sizeof k_opad);
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /*
     * perform inner SHA256
     */
    memset(bufferIn, 0x00, 1024);
    memcpy(bufferIn, k_ipad, 64);
    memcpy(bufferIn + 64, text, text_len);

    SHA256(bufferIn, 64 + text_len, tk2);

    /*
     * perform outer SHA256
     */
    memset(bufferOut, 0x00, 1024);
    memcpy(bufferOut, k_opad, 64);
    memcpy(bufferOut + 64, tk2, SHA256_DIGEST_LENGTH);

    SHA256(bufferOut, 64 + SHA256_DIGEST_LENGTH, digest);
}
