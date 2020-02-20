#ifndef UTIL__H
#define UTIL__H

#include <stdlib.h>
#include <openssl/sha.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct Userdata {
   int contentLength;
   int chunked;
} Userdata;

void getTerminalSn(char *sn);
char *parseHttpHeadValue( char *heads ,const char *head );
// int getContentLength(char *heads);
void logHex(unsigned char * data, const int size, const char * title);

void getTime(char *buff);
void getDate(char *buff);
void getDateAndTime(char *yyyymmddhhmmss);
void MaskPan(char *pszInPan, char *pszOutPan);

int bin2hex(unsigned char* pcInBuffer, char* pcOutBuffer, int iLen);
int hex2bin(const char *pcInBuffer, char *pcOutBuffer, int iLen);


int getHttpStatusCode(const char* response);
const char* bodyPointer(const char* response);
int bodyIndex(const char* response);
// int getContentLength(void* userdata, const char* tag, const int tag_len, const char* value, const int val_len);
short beautifyDateTime(char * dbDate, const int size, const char * yyyymmddhhmmss);
void getFormattedDateTime(char* dateTime, size_t len);
void getImsi(char buff[20]);
int getSignalLevel();
int getCellId();
int getLocationAreaCode();
const char* getSimId();

unsigned char* base64_encode(const unsigned char* src, size_t len, size_t* out_len);
unsigned char* base64_decode(const unsigned char* src, size_t len, size_t* out_len);

void generateRandomString(char* output, const int ouputSize);
char* safe_url_encode(const char* s, char* enc, const size_t len, int encoding);

int sha512Bin(char* digest, const char* data, const size_t datalen);
int sha512Hex(char* hexoutput, const char* data, const size_t datalen);

void hmac_sha256(
    const unsigned char* text, /* pointer to data stream        */
    int text_len, /* length of data stream         */
    const unsigned char* key, /* pointer to authentication key */
    int key_len, /* length of authentication key  */
    void* digest /* caller digest to be filled in */ );

#ifdef __cplusplus
}
#endif

#endif

