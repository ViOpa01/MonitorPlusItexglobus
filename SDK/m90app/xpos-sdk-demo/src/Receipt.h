#ifndef RECEIPT__H
#define RECEIPT__H

#include "merchant.h"
#include "merchantNQR.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DOTTEDLINE		"--------------------------------"
#define DOUBLELINE		"================================"

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
int printNQRCode(char * data);
short printNQRReceipts(MerchantNQR * nqr, const short isReprint);

int formatAmountEx(char* ulAmount);
const char *responseCodeToStr(const char responseCode[3]);

void printPtspMerchantEodReceiptHead(char *date);
void printPtspMerchantEodBody(MerchantNQR *merchant);
void printPtspMerchantEodReceiptFooter(char* sum, int count);
short printLastNQRReceipts(MerchantNQR * nqr, const short isReprint);


#ifdef __cplusplus
}
#endif

#endif
