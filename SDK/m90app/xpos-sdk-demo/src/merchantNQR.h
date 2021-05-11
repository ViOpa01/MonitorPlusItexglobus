#ifndef MERCHANT_NQR
#define MERCHANT_NQR

typedef struct MerchantNQR
{
    char dateAndTime[22];
    char codeType[32];
    char tid[16];
    char clientReference[64];
    char qrCode[512];
    char version[16];
    char productCode[2048];
    char amount[13];
    char responseMessage[64];
    char responseCode[4];
    char merchantNo[16];
    char subMerchantNo[16];
    char orderSn[32];
    char orderNo[32];
    char rrn[32];
    char merchantName[64];
    char status[32];
} MerchantNQR;

typedef struct MenuItems
{
    char label[32];
} MenuItems;


void doMerchantNQRTransaction();
void fetchNQREOD();
void lastNQRTransaction();

#endif