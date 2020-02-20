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

void copyMerchantParams(Eft *eft, const MerchantParameters *merchantParameters);
short autoReversalInPlace(Eft *eft, NetWorkParameters *netParam);
int performEft(Eft *eft, NetWorkParameters *netParam, const char *title);

void eftTrans(const enum TransType transType);
enum TransType cardPaymentHandler();

#ifdef __cplusplus
}
#endif


#endif

