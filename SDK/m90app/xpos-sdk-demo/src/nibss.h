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


/*
    NET_POSVAS_SSL
    NET_POSVAS_PLAIN
    NET_EPMS_SSL
    NET_EPMS_PLAIN

    ***DevMode Platform***

    NET_EPMS_SSL_TEST,
    NET_EPMS_PLAIN_TEST,
    NET_POSVAS_SSL_TEST,
    NET_POSVAS_PLAIN_TEST,
    
    UPSL_DIRECT_TEST

*/

// Set CURRENT_PLATFORM only for DevMode
#define CURRENT_PLATFORM NET_POSVAS_SSL

short uiHandshake(void);
short autoHandshake(void);
short uiGetParameters(void);
short uiCallHome(void);

int getParameters(MerchantParameters * merchantParameters);
int saveParameters(const MerchantParameters * merchantParameters);
int getSessionKey(char sessionKey[33]);


short isDevMode(const enum NetType netType);

short handleDe39(char * responseCode, char * responseDesc);

void addCallHomeData(NetworkManagement *networkMangement);
int sCallHomeAsync(NetworkManagement *networkMangement, NetWorkParameters *netParam);



#ifdef __cplusplus
}
#endif

#endif
