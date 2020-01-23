#ifndef RECEIPT__H
#define RECEIPT__H

#include "merchant.h"

#ifdef __cplusplus
extern "C" {
#endif

enum receiptCopy{
	CUSTOMER_COPY,
	MERCHANT_COPY,
	REPRINT_COPY
};

short printReceipts(Eft * eft, const short isReprint);
// int printEftReceipt(enum receiptCopy copy, Eft *eft);
void printHandshakeReceipt(MerchantData *mParam);
void reprintByRrn(void);
void reprintLastTrans(void);


#ifdef __cplusplus
}
#endif

#endif
