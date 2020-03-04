#ifndef USSD_SERVICES
#define USSD_SERVICES

typedef enum {
    CGATE_PURCHASE,
    CGATE_CASHOUT,
    MCASH_PURCHASE,
    PAYATTITUDE_PURCHASE
} USSDService;

const char* ussdServiceToString(USSDService service);

#endif
