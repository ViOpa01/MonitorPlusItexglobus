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




enum SubTransType
{
    SUB_PAYCODE_CASHOUT,
    SUB_PAYCODE_CASHIN,
    SUB_NONE,
};

void eftTrans(const enum TransType transType, const enum SubTransType subTransType);

#ifdef __cplusplus
}
#endif


#endif

