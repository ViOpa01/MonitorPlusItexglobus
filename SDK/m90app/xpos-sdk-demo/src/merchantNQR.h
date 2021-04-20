#ifndef MERCHANT_NQR
#define MERCHANT_NQR

typedef struct MerchantNQR
{
    char date[16];
    char time[16];
    char codeType[32];
    char tid[16];
    char clientReference[64];
    char qrCode[512];
    char version[16];

    long long amount;
} MerchantNQR;


void doMerchantNQRTransaction();

#endif