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

void printDottedLine();
void printBankLogo();
void printReceiptHeader();
void printFooter();
void printLine(const char *head, const char *val);
short printReceipts(Eft * eft, const short isReprint);
void printHandshakeReceipt(MerchantData *mParam);
void reprintByRrn(void);
void reprintLastTrans(void);
void getPrinterStatus(const int status);


void printVasHeader(const char *transDate);


#ifdef __cplusplus
}
#endif

#endif
