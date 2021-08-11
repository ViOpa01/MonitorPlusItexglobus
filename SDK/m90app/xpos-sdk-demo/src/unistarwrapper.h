#ifndef SRC_VAS_UNISTAR_WRAPPER_H
#define SRC_VAS_UNISTAR_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "basefun.h"
#include "SmartCardCb.h"
#include "CpuCard_Fun.h"


int bindSmartCardApi(SmartCardInFunc* smartCardInFunc);
int customerCardInserted(char* title, char* desc);
void customerCardInfo(void);
void writeUserCardTest(void);

#ifdef __cplusplus
}
#endif

#endif
