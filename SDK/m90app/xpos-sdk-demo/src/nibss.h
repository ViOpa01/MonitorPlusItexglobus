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

short handshake(void);
int getParameters(MerchantParameters * merchantParameters);
int saveParameters(const MerchantParameters * merchantParameters);


#ifdef __cplusplus
}
#endif

#endif
