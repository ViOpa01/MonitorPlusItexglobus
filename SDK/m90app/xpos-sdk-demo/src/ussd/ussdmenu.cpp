#include <stdlib.h>

#include <string>
#include <vector>

#include "vas/platform/simpio.h"
#include "vas/jsonwrapper/jsobject.h"

#include "ussdb.h"
#include "cgate.h"
#include "mcash.h"
#include "payWithPhone.h"
#include "tribease.h"

#include "ussdadmin.h"
#include "ussdservices.h"

#include "ussdmenu.h"

extern "C" {
#include "EmvEft.h"
#include "merchantNQR.h"
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
    case NQR:
        return "NQR";
    case TRIBEASE:
        return "Tribease";
    default:
        return "USSD Type Unknown";
    }
}

int ussdTransactionsMenu()
{
    static int once_flag = USSDB::init();
    std::vector<std::string> menu;
    USSD_T menuOptions[] = { CGATE, MCASH, PAYATTITUDE, PAYCODE, NQR, TRIBEASE };

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
        case NQR:
            //do some NQR stuffs
            NQRMenu();
            break;
        case TRIBEASE:
            tribease();
            break;
        default:
            break;
        }
    }
    
    return 0;
}

const char* NQRString(NQR_T handler)
{
    switch (handler) {
    case NQR_TRANSACTION:
        return "Purchase";
    // case NQR_CHECK_RECENT_TRANSACTION:
    //     return "Check Last Transaction";
    case NQR_REPRINT:
        return "Reprint Last Transaction";
    case NQR_PRINT_EOD:
        return "Print EOD";
    default:
        return "USSD Type Unknown";
    }
}

int NQRMenu()
{
    std::vector<std::string> menu;
    NQR_T menuOptions[] = { NQR_TRANSACTION, NQR_REPRINT, NQR_PRINT_EOD };


    for (size_t i = 0; i < sizeof(menuOptions) / sizeof(NQR_T); ++i) {
        menu.push_back(NQRString(menuOptions[i]));
    }

    while (1) {
        int selection = UI_ShowSelection(30000, "NQR Menu", menu, 0);

        if (selection < 0) {
            return -1;
        } 

        switch (menuOptions[selection]) {
        case NQR_TRANSACTION:
            doMerchantNQRTransaction();
            break;
        case NQR_REPRINT:
            lastNQRTransaction();
            break;
        case NQR_PRINT_EOD:
            fetchNQREOD();
            break;
        default:
            break;
        }
    }
    
    return 0;
}








