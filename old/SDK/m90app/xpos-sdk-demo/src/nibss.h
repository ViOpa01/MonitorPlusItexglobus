/**
 * File: nibss.h 
 * -------------
 * Defines a new interface for Nibss implementation.
 * @author Opeyemi Ajani, Itex integrated services.
 */


#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _ITEX_NIBSS_INCLUDED
#define _ITEX_NIBSS_INCLUDED

#include "Nibss8583.h"

short uiHandshake(void);


int getParameters(MerchantParameters * merchantParameters);
int saveParameters(const MerchantParameters * merchantParameters);
int getSessionKey(char sessionKey[33]);


#ifdef __cplusplus
}
#endif

#endif
