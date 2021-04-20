#ifndef USSD_MENU_H
#define USSD_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CGATE,
    MCASH,
    PAYATTITUDE,
    PAYCODE,
    NQR
} USSD_T;


int ussdTransactionsMenu();
const char* ussdTypeToString(USSD_T provider);

#ifdef __cplusplus
}
#endif

#endif
