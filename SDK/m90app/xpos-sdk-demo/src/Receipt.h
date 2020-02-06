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

void printBankLogo();
void printFooter();
short printReceipts(Eft * eft, const short isReprint);
short printPaycodeReceipts(Eft * eft, const short isReprint);
void printHandshakeReceipt(MerchantData *mParam);
void reprintByRrn(void);
void reprintLastTrans(void);
void getPrinterStatus(const int status);


#ifdef __cplusplus
}
#endif

#endif
