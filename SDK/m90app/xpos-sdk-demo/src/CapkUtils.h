/**
 * File: CapkUtils.h
 * -----------------
 * Defines a new interface for Capk.
 */

#ifndef _CAPK_UTILS_INCLUDED
#define _CAPK_UTILS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "libapi_xpos/inc/libapi_emv.h"

short addCapk(const CAPUBLICKEY * capk, const char * capkName);

#ifdef __cplusplus
}
#endif


#endif
