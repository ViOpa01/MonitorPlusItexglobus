#ifndef SRC_VASMENU_H
#define SRC_VASMENU_H

#include <stdlib.h>

#include <vector>
#include <string>

typedef enum {
    ENERGY,
    AIRTIME,
    TV_SUBSCRIPTIONS,
    DATA,
    SMILE,
    CASHIO
} VAS_Menu_T;

std::vector<VAS_Menu_T> getVasTransactions(std::vector<std::string>& menu);
int vasTransactionTypes();
int doVasTransaction(VAS_Menu_T menu);
int doCashIO();

const char* vasMenuString(VAS_Menu_T type);


#endif
