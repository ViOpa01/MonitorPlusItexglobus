#ifndef SRC_VAS_BRIDGE_H
#define SRC_VAS_BRIDGE_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../network.h"
#include "../EmvEft.h"
#include "../merchant.h"
#include "../Nibss8583.h"

int vasTransactionTypesBridge();
int doVasCardTransaction(Eft* trxContext, unsigned long amount);
int requeryMiddleware(Eft* trxContext, const char* tid);
int setupBaseHugeNetwork(NetWorkParameters * netParam, const char *path);
int performCashIn();
int performCashOut();
int isAgent();


#ifdef __cplusplus
}
#endif

#endif
