/**
 * File: EmvEft.h
 * --------------
 * Defines a new interface for Emv Eft transactions.
 * @author Opeyemi Ajani.
 */



#ifndef _EMV_EFT_INCLUDED
#define _EMV_EFT_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#include "nibss.h"



void eftTrans(const enum TransType transType);

#ifdef __cplusplus
}
#endif


#endif

