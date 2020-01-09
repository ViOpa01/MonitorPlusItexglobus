#pragma once

// #include "network.h"

#include "../../../libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_util.h"


void getTerminalSn(char *sn);
char *parseHttpHeadValue( char *heads ,const char *head );
int getContentLength(char *heads);


