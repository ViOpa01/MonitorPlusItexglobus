#ifndef UTIL__H
#define UTIL__H


void getTerminalSn(char *sn);
char *parseHttpHeadValue( char *heads ,const char *head );
int getContentLength(char *heads);
void logHex(unsigned char * data, const int size, const char * title);

void getTime(char *buff);
void getDate(char *buff);
void getDateAndTime(char *yyyymmddhhmmss);
void MaskPan(char *pszInPan, char *pszOutPan);

#endif UTIL__H

