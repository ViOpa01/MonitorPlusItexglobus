#ifndef SRC_VAS_UNISTAR_WRAPPER_H
#define SRC_VAS_UNISTAR_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "unistar-smartcard/basefun.h"
#include "unistar-smartcard/SmartCardCb.h"
#include "unistar-smartcard/CpuCard_Fun.h"


int bindSmartCardApi(SmartCardInFunc* smartCardInFunc);
int customerCardInserted(char* title, char* desc);
void customerCardInfo(void);
void writeUserCardTest(void);

#ifdef __cplusplus
}
#endif

#endif
