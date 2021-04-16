#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "platform/platform.h"

#include "airtime.h"
#include "cashio.h"
#include "data.h"
#include "electricity.h"
#include "paytv.h"
#include "smile.h"

#include "vas.h"
#include "vasdb.h"
#include "vasadmin.h"

#include "jsonwrapper/jsobject.h"

#include "vasmenu.h"


std::vector<VAS_Menu_T> getVasTransactions(std::vector<std::string>& menu)
{
    std::vector<VAS_Menu_T> menuOptions;

    menu.clear();

    const iisys::JSObject& isAgent = Payvice().object(Payvice::IS_AGENT);
    if (isAgent.isBool() && isAgent.getBool() == true) {
        menuOptions.push_back(CASHIO);
    }

    menuOptions.push_back(ENERGY);
    menuOptions.push_back(TV_SUBSCRIPTIONS);
    menuOptions.push_back(AIRTIME);
    menuOptions.push_back(DATA);
    menuOptions.push_back(SMILE);

    for (size_t i = 0; i < menuOptions.size(); ++i) {
        menu.push_back(vasMenuString(menuOptions[i]));
    }
    menu.push_back("Vas Admin");

    return menuOptions;
}

int vasTransactionTypes()
{
    static int once_flag = initVasTables();
    std::vector<std::string> menu;
    std::vector<VAS_Menu_T> menuOptions = getVasTransactions(menu);

    (void)once_flag;

    while (1) {
        int selection = UI_ShowSelection(30000, "Services", menu, 0);

        if (selection < 0) {
            return -1;
        } else if (menu.size() - 1 == (size_t)selection) {
            if (vasAdmin() != 0) {
                return -2;
            }
        } else {
            if (doVasTransaction(menuOptions[selection]) == -2) {
                return -2;
            }
        }
    }

    return 0;
}

int doVasTransaction(VAS_Menu_T menu)
{
    VasFlow flow;
    Postman postman;
    const char* title = vasMenuString(menu);

    switch (menu) {
    case ENERGY: {
        Electricity electricity(title, postman);
        return flow.start(&electricity);
    }
    case AIRTIME: {
        Airtime airtime(title, postman);
        return flow.start(&airtime);
    }
    case TV_SUBSCRIPTIONS: {
        PayTV televisionSub(title, postman);
        return flow.start(&televisionSub);
    }
    case DATA: {
        Data data(title, postman);
        return flow.start(&data);
    }
    case SMILE: {
        Smile smile(title, postman);
        return flow.start(&smile);
    }
    case CASHIO: {
        ViceBanking cashIO(title, postman);
        return flow.start(&cashIO);
    }
    default:
        break;
    }

    return 0;
}

int doCashIO()
{
    VasFlow flow;
    Postman postman;
    const char* title = vasMenuString(CASHIO);

    ViceBanking cashIO(title, postman);
    return flow.start(&cashIO);
}

const char* vasMenuString(VAS_Menu_T type)
{
    switch (type) {
    case ENERGY:
        return "Electricity";
    case AIRTIME:
        return "Airtime";
    case TV_SUBSCRIPTIONS:
        return "TV Subscriptions";
    case DATA:
        return "Data";
    case SMILE:
        return "Smile";
    case CASHIO:
        return "Cash In / Cash Out";
    default:
        return "";
    }
}
