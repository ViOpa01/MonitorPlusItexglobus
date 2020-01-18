#ifndef UTIL__H
#define UTIL__H


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

int getHttpStatusCode(const char* response);
const char* bodyPointer(const char* response);
int bodyIndex(const char* response);
// int getContentLength(void* userdata, const char* tag, const int tag_len, const char* value, const int val_len);




#endif UTIL__H

