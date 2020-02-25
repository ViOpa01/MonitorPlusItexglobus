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
void printReceiptLogo(const char filename[32]);
void printReceiptHeader(const char *transDate);
void printFooter();
void printLine(const char *head, const char *val);
short printReceipts(Eft * eft, const short isReprint);
short printPaycodeReceipts(Eft * eft, const short isReprint);
void printHandshakeReceipt(MerchantData *mParam);
void reprintByRrn(void);
void reprintLastTrans(void);
int getPrinterStatus(const int status);

const char *responseCodeToStr(const char responseCode[3]);



#ifdef __cplusplus
}
#endif

#endif
