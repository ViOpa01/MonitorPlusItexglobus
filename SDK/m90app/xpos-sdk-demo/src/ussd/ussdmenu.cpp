#include <stdlib.h>

#include <string>
#include <vector>

#include "vas/simpio.h"
#include "vas/jsobject.h"

#include "ussdb.h"
#include "cgate.h"
#include "mcash.h"
#include "payWithPhone.h"

#include "ussdadmin.h"
#include "ussdservices.h"

#include "ussdmenu.h"

extern "C" {
#include "EmvEft.h"
}

const char* ussdTypeToString(USSD_T provider)
{
    switch (provider) {
    case CGATE:
        return "C'Gate";
    case MCASH:
        return "mCash";
    case PAYATTITUDE:
        return "PayAttitude";
    case PAYCODE:
        return "PayCode";
    default:
        return "USSD Type Unknown";
    }
}

int ussdTransactionsMenu()
{
    static int once_flag = USSDB::init();
    std::vector<std::string> menu;
    USSD_T menuOptions[] = { CGATE, MCASH, PAYATTITUDE, PAYCODE };

    (void)once_flag;

    for (size_t i = 0; i < sizeof(menuOptions) / sizeof(USSD_T); ++i) {
        menu.push_back(ussdTypeToString(menuOptions[i]));
    }
    menu.push_back("Admin");

    while (1) {
        int selection = UI_ShowSelection(30000, "Cardless Payment", menu, 0);

        if (selection < 0) {
            return -1;
        } else if (menu.size() - 1 == (size_t)selection) {
            ussdAdmin();
        }

        switch (menuOptions[selection]) {
        case CGATE:
            coralPayMenu();
            break;
        case MCASH:
            mCashMenu();
            break;
        case PAYATTITUDE:
            payWithPhone();
            break;
        case PAYCODE:
            paycodeHandler();
            break;
        default:
            break;
        }
    }
    
    return 0;
}







