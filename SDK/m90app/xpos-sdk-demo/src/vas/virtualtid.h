#ifndef VAS_VIRTUALTID_H
#define VAS_VIRTUALTID_H

#include "EmvEft.h"


int copyVirtualPinBlock(Eft *pTrxContext, const char* pinBlock);

bool virtualConfigurationIsSet();
int  resetVirtualConfiguration();
void resetVirtualConfigurationAsync();

// int swithMerchantToVas(Merchant& merchant);

#endif
