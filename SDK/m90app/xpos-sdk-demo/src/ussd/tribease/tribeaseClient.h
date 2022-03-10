#ifndef TRIBEASE_CLIENT_H
#define TRIBEASE_CLIENT_H

typedef struct TribeaseClient {
    char name[64];
    unsigned long amount;
    char paymentRef[32];
    char customerPhoneNo[16];
    char merchantPhoneNo[16];
    char authToken[0x300];
    char pin[7];
    char ref[64];
    int token;
    char transactionRef[32];
    char responseMessage[64];
    char transactionStatus;
} TribeaseClient;

#endif // TRIBEASE_CLIENT_H
