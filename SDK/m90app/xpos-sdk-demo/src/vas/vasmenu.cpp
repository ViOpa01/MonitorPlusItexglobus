#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "platform/platform.h"

#include "airtime.h"
#include "cashin.h"
#include "cashout.h"
#include "data.h"
#include "electricity.h"
#include "paytv.h"
#include "smile.h"
#include "jamb.h"
#include "waec.h"
#include "airtimeussd.h"
#include "electricityussd.h"
#include "dataussd.h"
#include "paytvussd.h"

#include "vas.h"
#include "vasdb.h"
#include "vasadmin.h"

#include "jsonwrapper/jsobject.h"

#include "vasmenu.h"


std::vector<VAS_Menu_T> getVasTransactions(std::vector<std::string>& menu, const bool displayCashio)
{
    std::vector<VAS_Menu_T> menuOptions;

    menu.clear();

    const iisys::JSObject& isAgent = Payvice().object(Payvice::IS_AGENT);

    if (isAgent.isBool() && isAgent.getBool() == true && displayCashio == true) {
        menuOptions.push_back(CASHOUT);
        menuOptions.push_back(CASHIN);
    }

    menuOptions.push_back(ENERGY);
    menuOptions.push_back(AIRTIME);
    menuOptions.push_back(VAS_USSD);
    menuOptions.push_back(TV_SUBSCRIPTIONS);
    menuOptions.push_back(DATA);
    menuOptions.push_back(JAMB_EPIN);
    menuOptions.push_back(WAEC);
    menuOptions.push_back(SMILE);
    

    for (size_t i = 0; i < menuOptions.size(); ++i) {
        menu.push_back(vasMenuString(menuOptions[i]));
    }
    menu.push_back("Vas Admin");

    return menuOptions;
}

int vasTransactionTypes(const bool displayCashio)
{
    static int once_flag = initVasTables();
    std::vector<std::string> menu;
    std::vector<VAS_Menu_T> menuOptions = getVasTransactions(menu, displayCashio);

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

int doVasUssd(VasFlow& flow, const char* title, VasComProxy& comProxy)
{
    std::vector<std::string> menu;

    menu.push_back("Airtime");
    menu.push_back("Data");
    menu.push_back("Cable");
    menu.push_back("Electricity");

    const int selection = UI_ShowSelection(30000, title, menu, 0);
    if (selection == 0) {
        AirtimeUssd airtimeUssd(title, comProxy);
        return flow.start(&airtimeUssd);
    } else if (selection == 1) {
        DataUssd dataUssd(title, comProxy);
        return flow.start(&dataUssd);
    } else if (selection == 2) {
        PaytvUssd paytvUssd(title, comProxy);
        return flow.start(&paytvUssd);
    } else if (selection == 3) {
        ElectricityUssd electricityUssd(title, comProxy);
        return flow.start(&electricityUssd);
    }

    return -1;
}

int doVasTransaction(VAS_Menu_T menu)
{
    VasFlow flow;
    Postman postman = getVasPostMan();
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
    case CASHIN: {
        Cashin cashin(title, postman);
        return flow.start(&cashin);
    }
    case CASHOUT: {
        Cashout cashout(title, postman);
        return flow.start(&cashout);
    }
    case JAMB_EPIN: {
        return Jamb::menu(flow, title, postman);
    }
    case WAEC: {
        Waec waec(title, postman);
        return flow.start(&waec);
    }
    case VAS_USSD: {
        return doVasUssd(flow, title, postman);
    }
    default:
        break;
    }

    return 0;
}

int doCashIn()
{
    VasFlow flow;
    Postman postman = getVasPostMan();
    const char* title = vasMenuString(CASHIN);

    Cashin cashin(title, postman);
    return flow.start(&cashin);
}

int doCashOut()
{
    VasFlow flow;
    Postman postman = getVasPostMan();
    const char* title = vasMenuString(CASHOUT);

    Cashout cashout(title, postman);
    return flow.start(&cashout);
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
    case CASHIN:
        return "Transfer";
    case CASHOUT:
        return "Withdrawal";
    case JAMB_EPIN:
        return "JAMB PIN";
    case WAEC:
        return "WAEC";
    case VAS_USSD:
        return "PIN Generation";
    default:
        return "";
    }
}
