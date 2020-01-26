/**
 * File: nibss.h 
 * -------------
 * Defines a new interface for Nibss implementation.
 * @author Opeyemi Ajani, Itex integrated services.
 */





#ifndef _ITEX_NIBSS_INCLUDED
#define _ITEX_NIBSS_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#include "Nibss8583.h"
#include "network.h"


// #define CURRENT_PLATFORM NET_POSVAS_SSL
#define CURRENT_PLATFORM NET_EPMS_SSL

//#define CURRENT_PLATFORM NET_POSVAS_SSL_TEST
//

short uiHandshake(void);
short autoHandshake(void);
short uiGetParameters(void);
short uiCallHome(void);

int getParameters(MerchantParameters * merchantParameters);
int saveParameters(const MerchantParameters * merchantParameters);
int getSessionKey(char sessionKey[33]);


short isDevMode(const enum NetType netType);

short handleDe39(char * responseCode, char * responseDesc);



#ifdef __cplusplus
}
#endif

#endif
