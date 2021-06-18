#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "payvice.h"
#include "vas.h"
#include "../EmvDB.h"
#include "../merchant.h"
#include "./platform/simpio.h"

#include "platform/platform.h"
#include "vascomproxy.h"
#include "vasdb.h"
#include "virtualtid.h"
#include "viewmodels/cashioViewModel.h"


int fundPayviceWallet()
{
    VasResult result;
    Postman postman;
    ViceBankingViewModel viewModel("", postman);

    Payvice payvice;

    if (!loggedIn(payvice)) {
        logIn(payvice);
        return -2;
    }

    if (!itexIsMerchant() && !virtualConfigurationIsSet()) {
        Demo_SplashScreen("Configuring...", "www.payvice.com");
        if (resetVirtualConfiguration() < 0) {
            return -1;
        }
    }

    const unsigned long amount = getVasAmount(serviceToString(WALLET_FUNDING));
    if (amount == 0 || viewModel.setAmount(amount).error != NO_ERRORS) {
        return -1;
    } else if (viewModel.setService(WALLET_FUNDING).error != NO_ERRORS) {
        return -1;
    }

    const std::string phoneNumber = getPhoneNumber("Phone Number", "");
    if (phoneNumber.empty()) {
        return -1;
    }

    result = viewModel.setPhoneNumber(phoneNumber);
    if (result.error != NO_ERRORS) {
        UI_ShowButtonMessage(30000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    Demo_SplashScreen("Wallet Balance", "Please wait...");
    result = viewModel.lookup();
    if (result.error != NO_ERRORS) {
        UI_ShowButtonMessage(30000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }
    
    result = viewModel.complete("");

    std::map<std::string, std::string> record = viewModel.storageMap(result);

    if (record.find(VASDB_DATE) == record.end() || record[VASDB_DATE].empty()) {
        record[VASDB_DATE] = vasimpl::formattedDateTime() + ".000";
    }

    if (result.error) {
         UI_ShowButtonMessage(3000, record[VASDB_STATUS].c_str(), result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
    } else {
         UI_ShowButtonMessage(3000, "Approved", result.message.c_str(), "OK", UI_DIALOG_TYPE_CONFIRMATION);
    }
    
    // print
    record["walletId"] = payvice.object(Payvice::WALLETID).getString();
    // printStatus = 
    printVas(record);

    return 0;
}

int walletBalance()
{
    VasResult result;
    Postman postman;
    iisys::JSObject json;

    Payvice payvice;
    

    const iisys::JSObject& walletId = payvice.object(Payvice::WALLETID);
    const iisys::JSObject& username = payvice.object(Payvice::USER);

    json("wallet") = walletId;
    json("username") = username;

    Demo_SplashScreen("Wallet Balance", "Please wait...");
    result = postman.sendVasRequest("/api/vas/wallet/balance", &json);

    if(result.error != NO_ERRORS) {
        UI_ShowButtonMessage(2000, "Message", result.message.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        return -1;
    }

    if (!json.load(result.message)) {
        return -1;
    }

    result = vasResponseCheck(json);

    if(result.error != NO_ERRORS) {
        UI_ShowButtonMessage(2000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        return -1;
    }

    const iisys::JSObject& data = json("data");
    if (!data.isObject()) {
        return -1;
    }

    const iisys::JSObject& balance = data("balance");
    const iisys::JSObject& commission = data("commission");
    const iisys::JSObject& ledgerBalance = data("ledgerbalance");


    if (!balance.isString() || !commission.isString() || !ledgerBalance.isString()) {
        UI_ShowButtonMessage(2000, "Error", "Unexpected Error", "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    const std::string msg = std::string("Wallet Balance:\n") + balance.getString()
                            + "\nLedger Balance:\n" + ledgerBalance.getString()
                            + "\nCommission:\n" + commission.getString();

    UI_ShowButtonMessage(30000, "Wallet Info", msg.c_str(), "OK", UI_DIALOG_TYPE_NONE);

    return 0;
}

int walletTransfer()
{
    VasResult result;
    Postman postman;
    iisys::JSObject json;

    std::string pin;

    const unsigned long amount = getVasAmount("Transfer Amount");
    if (amount == 0) {
        return -1;
    }

    if (getVasPin(pin) != NO_ERRORS) {
        return -1;
    }

    json("amount") = majorDenomination(amount);
    json("pin") = encryptedPin(Payvice(), pin.c_str());

    Demo_SplashScreen("Commission Transfer", "Please wait...");
    result = postman.sendVasRequest("/api/v1/vas/commission/transfer", &json);

    if(result.error != NO_ERRORS) {
        UI_ShowButtonMessage(2000, "Message", result.message.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        return -1;
    }

    if (!json.load(result.message)) {
        return -1;
    }

    result = vasResponseCheck(json);

    if(result.error != NO_ERRORS) {
        UI_ShowButtonMessage(2000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        return -1;
    }

    const iisys::JSObject& data = json("data");
    if (!data.isObject()) {
        return -1;
    }

    const iisys::JSObject& balance = data("newBalance");
    const iisys::JSObject& commission = data("newCommission");

    if (!balance.isString() || !commission.isString()) {
        UI_ShowButtonMessage(2000, "Error", "Unexpected Error", "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    const std::string msg = std::string("Balance:\n") + balance.getString() + "\nCommission:\n" + commission.getString();

    UI_ShowButtonMessage(30000, result.message.c_str(), msg.c_str(), "OK", UI_DIALOG_TYPE_NONE);

    return 0;
}
