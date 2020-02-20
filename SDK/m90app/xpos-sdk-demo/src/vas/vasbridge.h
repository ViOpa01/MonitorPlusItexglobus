#ifndef VASBRIDGE__H
#define VASBRIDGE__H


#ifdef __cplusplus
extern "C" {
#endif

#include "network.h"
#include "EmvEft.h"
#include "merchant.h"
#include "Nibss8583.h"

int vasTransactionTypesBridge();
int doVasCardTransaction(Eft* trxContext, unsigned long amount);
int requeryMiddleware(Eft* trxContext, const char* tid);
short setupBaseHugeNetwork(NetWorkParameters * netParam, const char *path);


#ifdef __cplusplus
}
#endif



#endif
