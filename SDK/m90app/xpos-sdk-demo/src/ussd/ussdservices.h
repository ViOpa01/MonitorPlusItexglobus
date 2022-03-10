#ifndef USSD_SERVICES
#define USSD_SERVICES

typedef enum {
    CGATE_PURCHASE,
    CGATE_CASHOUT,
    MCASH_PURCHASE,
    PAYATTITUDE_PURCHASE,
    TRIBEASE_PURCHASE,
    TRIBEASE_TOKEN,
} USSDService;

const char* ussdServiceToString(USSDService service);

#endif
