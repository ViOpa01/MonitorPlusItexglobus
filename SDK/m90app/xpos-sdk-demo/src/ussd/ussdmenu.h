#ifndef USSD_MENU_H
#define USSD_MENU_H

typedef enum {
    CGATE,
    MCASH,
    PAYATTITUDE
} USSD_T;


int ussdTransactionsMenu();
const char* ussdTypeToString(USSD_T provider);

#endif
