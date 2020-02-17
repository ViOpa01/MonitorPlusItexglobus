#ifndef VAS_VIRTUALTID_H
#define VAS_VIRTUALTID_H

#include "EmvEft.h"


#ifdef __cplusplus
extern "C" {
#endif

int copyVirtualPinBlock(Eft *pTrxContext, const char* pinBlock);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
typedef int bool;
#endif

bool virtualConfigurationIsSet();
int  resetVirtualConfiguration();
void resetVirtualConfigurationAsync();
int swithMerchantToVas(Eft* trxContext);

#endif
