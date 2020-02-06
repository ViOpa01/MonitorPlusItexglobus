#ifndef VASBRIDGE__H
#define VASBRIDGE__H


#ifdef __cplusplus
extern "C" {
#endif

#include "network.h"
#include "EmvEft.h"
#include "merchant.h"

int vasTransactionTypesBridge();
int doVasCardTransaction(Eft* trxContext, unsigned long amount);
short setupBaseHugeNetwork(NetWorkParameters * netParam, const char *path);


#ifdef __cplusplus
}
#endif



#endif
