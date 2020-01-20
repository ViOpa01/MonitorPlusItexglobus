/**
 * File: EftDbImpl.h 
 * -----------------
 * Implements EmvDB interface.
 */

#ifndef _EFT_DB_IMPLEMENTATION_INCLUDED
#define _EFT_DB_IMPLEMENTATION_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "Nibss8583.h"

short getEft(Eft * eft);
short saveEft(Eft *eft);
short updateEft(const Eft * eft);
short getLastTransaction(Eft * eft);

#ifdef __cplusplus
}
#endif

#endif 
