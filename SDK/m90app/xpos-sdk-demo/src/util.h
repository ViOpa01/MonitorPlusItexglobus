#pragma once


void getTerminalSn(char *sn);
char *parseHttpHeadValue( char *heads ,const char *head );
int getContentLength(char *heads);
void logHex(unsigned char * data, const int size, const char * title);


