#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <algorithm>
#include <iterator>

#include "simpio.h"
#include "util.h"



#include <unistd.h>

#include "jsobject.h"

#include "electricity.h"
#include "cashio.h"
#include "airtime.h"
#include "data.h"
#include "paytv.h"
#include "smile.h"

#include "vas.h"
#include "vasdb.h"
#include "wallet.h"

extern "C" {
#include "itexFile.h"
}

extern int formatAmount(std::string& ulAmount);

#define VASADMIN "VASADMIN"

int vasEodMap(VasDB& db, const char* date, const std::string& service, std::map<std::string, std::string>& values)
{
    std::string amountApproved;
    std::string approvedCount;
    std::string amountDeclined;
    std::string declinedCount;
    std::string totalCount;
    std::string amountStr;

    std::string amountSymbol = getCurrencySymbol();

    int ret = -1;

    if (db.sumTransactionsOnDate(amountApproved, date, VasDB::APPROVED, service)) {
        return ret;
    } else if (db.countTransactionsOnDate(approvedCount, date, VasDB::APPROVED, service)) {
        return ret;
    } else if (db.sumTransactionsOnDate(amountDeclined, date, VasDB::DECLINED, service)) {
        return ret;
    } else if (db.countTransactionsOnDate(declinedCount, date, VasDB::DECLINED, service)) {
        return ret;
    } else if (db.countTransactionsOnDate(totalCount, date, VasDB::ALL, service)) {
        return ret;
    }

    std::vector<std::map<std::string, std::string> > transactions;
    if (db.selectTransactionsOnDate(transactions, date, service) < 0) {
        return -1;
    }

    iisys::JSObject jHelp;

    // getFormattedDateTime(dateTime, sizeof(dateTime));
    values[VASDB_DATE] = date;

    for (unsigned i = 0; i < transactions.size(); ++i) {
        std::map<std::string, std::string>& element = transactions[i];
        int approved = !strcmp(element[VASDB_STATUS].c_str(), VasDB::trxStatusString(VasDB::APPROVED));

        amountStr = element[VASDB_AMOUNT];
        formatAmount(amountStr);

        jHelp[i]("status") = (approved) ? " A " : " D ";
        if (service.empty()) {
            jHelp[i]("preferredField") = element[VASDB_SERVICE];
        } else {
            jHelp[i]("preferredField") = element[VASDB_TRANS_SEQ];
        }
        jHelp[i]("amount") = amountStr;
        jHelp[i]("tstamp") = element[VASDB_DATE].substr(11, 5);
    }
    values["menu"] = jHelp.dump();

    // printf("Approved -> %s, Declined -> %s", amountApproved.c_str(), amountDeclined.c_str());
    formatAmount(amountApproved);
    formatAmount(amountDeclined);
    // printf("Approved -> %s, Declined -> %s", amountApproved.c_str(), amountDeclined.c_str());

    values["approvedAmount"] = amountSymbol + " " + amountApproved;
    values["declinedAmount"] = amountSymbol + " " + amountDeclined;

    values["approvedCount"] = approvedCount;
    values["declinedCount"] = declinedCount;
    values["totalCount"] = totalCount;

    return 0;
}

int requestDateAndService(VasDB& database, std::string& date, std::string& service)
{
    std::vector<std::string> dates;
    std::vector<std::string> services;
    const char* allServices = "All Services";

    database.selectUniqueDates(dates);
    if (dates.empty()) {
        UI_ShowButtonMessage(3000, "Message", "No Records", "OK", UI_DIALOG_TYPE_CAUTION);
        return -1;
    }

    while (1) {
        int selection = UI_ShowSelection(30000, "Select Date", dates, 0);
        if (selection < 0) {
            date.clear();
            return -1;
        }

        date = dates[selection]; // set date

        services.clear();
        database.selectUniqueServices(services, date);

        if (services.size() == 0) {
            date.clear();
            UI_ShowButtonMessage(2000, date.c_str(), "No Records for Date", "OK", UI_DIALOG_TYPE_CAUTION);
            return -1;
        } else if (services.size() > 1) {
            services.push_back(allServices);
        }

        selection = UI_ShowSelection(30000, date.c_str(), services, 0);

        if (selection < 0) {
            continue;
        }

        service = (services[selection] == allServices) ? "" : services[selection]; // set service
        break;
    }

    return 0;
}

int showVasTransactions(int timeout, const char* title, std::vector<std::map<std::string, std::string> >& elements, int preselect)
{
    std::string amountStr;
    std::vector<std::string> transvector;
    std::string amountSymbol = getCurrencySymbol();

    for (size_t i = 0; i < elements.size(); ++i) {
        std::map<std::string, std::string>& element = elements[i];
        int approved = !strcmp(element[VASDB_STATUS].c_str(), VasDB::trxStatusString(VasDB::APPROVED));
        amountStr = element[VASDB_AMOUNT];
        formatAmount(amountStr);

        transvector.push_back(element[VASDB_SERVICE] + '\n'
            + ((approved) ? " Approved, " : " Declined, ") + amountSymbol + " " + amountStr
            + "\nTRANS SEQ: " + element[VASDB_TRANS_SEQ]);
    }

    return UI_ShowSelection(timeout, title, transvector, preselect);
}

void vasEndofDay()
{
    std::string date, service;
    VasDB database;

    while (1) {
        if (requestDateAndService(database, date, service) < 0) {
            return;
        }

        std::map<std::string, std::string> values;
        if (vasEodMap(database, date.c_str(), service, values) < 0) {
            continue;
        }

        values["trxType"] = service.empty() ? "Services" : service;
    
        printVasEod(values);

    }
}

bool listVasTransactions(VasDB& db, const std::string& dateTime, const std::string& service, const char* title = NULL)
{
    std::vector<std::map<std::string, std::string> > transactions;

    if (db.selectTransactionsOnDate(transactions, dateTime.c_str(), service) < 0) {
        return false;
    }

    if (!transactions.size()) {
        UI_ShowButtonMessage(2000,  dateTime.c_str(), "No Transactions For Specified Date","OK", UI_DIALOG_TYPE_CAUTION);
        return false;
    }

    int selection = showVasTransactions(60000, title ? title : service.c_str(), transactions, 0);

    if (selection < 0) {
        return false;
    }

    std::map<std::string, std::string>& record = transactions[selection];
  
    record["walletId"] = Payvice().object(Payvice::WALLETID).getString();
    record["receipt_copy"] += "- Reprint -";
    if (printVas(record) != UPRN_SUCCESS) {
        return false;
    }

    return true;
}

bool vasReprintToday()
{
    VasDB db;
    char dateTime[32] = { 0 };

    Demo_SplashScreen("Loading Transactions...", "www.payvice.com");
    getFormattedDateTime(dateTime, sizeof(dateTime));
    return listVasTransactions(db, dateTime, "", "Transactions Today");
}

bool vasReprintByDate()
{
    std::string date, service;
    VasDB database;

    while (1) {
        if (requestDateAndService(database, date, service) < 0) {
            return false;
        }
        break;
    }

    return listVasTransactions(database, date, service, service.empty() ? "All Services" : service.c_str());
}

int printRequery(iisys::JSObject& transaction)
{
    Postman postman;
    std::string productName;
    std::string statusMsg;
    VasStatus status;
    std::map<std::string, std::string> record;
    
    statusMsg = transaction("status").getString();
    productName = transaction("product").getString();

    // enum('status', ['initialized', 'debited', 'pending', 'successful', 'failed', 'declined'])->default('initialized');
    if (statusMsg != "successful" && statusMsg != "failed" && statusMsg != "declined") {
        return -1;
    }

    std::transform(productName.begin(), productName.end(), productName.begin(), ::tolower);

    iisys::JSObject response;
    if (!response.load(transaction("response").getString())) {
        return -1;
    }


    // VasStatus status = vasErrorCheck(response);
    // status.message = transaction("status").getString();


    if (productName == "ikedc") {
        Electricity electricity(vasMenuString(ENERGY), postman);
        status = electricity.processPaymentResponse(response, IKEJA);
        record = electricity.storageMap(status);
    } else if (productName == "ekedc") {
        Electricity electricity(vasMenuString(ENERGY), postman);
        status = electricity.processPaymentResponse(response, EKEDC);
        record = electricity.storageMap(status);
    } else if (productName == "eedc") {
        Electricity electricity(vasMenuString(ENERGY), postman);
        status = electricity.processPaymentResponse(response, EEDC);
        record = electricity.storageMap(status);
    } else if (productName == "ibedc") {
        Electricity electricity(vasMenuString(ENERGY), postman);
        status = electricity.processPaymentResponse(response, IBEDC);
        record = electricity.storageMap(status);
    } else if (productName == "phed") {
        Electricity electricity(vasMenuString(ENERGY), postman);
        status = electricity.processPaymentResponse(response, PHED);
        record = electricity.storageMap(status);
    } else if (productName == "aedc") {
        Electricity electricity(vasMenuString(ENERGY), postman);
        status = electricity.processPaymentResponse(response, AEDC);
        record = electricity.storageMap(status);
    } else if (productName == "kedco") {
        Electricity electricity(vasMenuString(ENERGY), postman);
        status = electricity.processPaymentResponse(response, KEDCO);
        record = electricity.storageMap(status);
    } else if (productName == "withdrawal") {
        ViceBanking viceBanking(vasMenuString(CASHIO), postman);
        status = viceBanking.processPaymentResponse(response, WITHDRAWAL);
        record = viceBanking.storageMap(status);
    } else if (productName == "transfer") {
        ViceBanking viceBanking(vasMenuString(CASHIO), postman);
        status = viceBanking.processPaymentResponse(response, TRANSFER);
        record = viceBanking.storageMap(status);
    } else if (productName == "gotv") {
        PayTV payTv(vasMenuString(TV_SUBSCRIPTIONS), postman);
        status = payTv.processPaymentResponse(response, GOTV);
        record = payTv.storageMap(status);
    } else if (productName == "dstv") {
        PayTV payTv(vasMenuString(TV_SUBSCRIPTIONS), postman);
        status = payTv.processPaymentResponse(response, DSTV);
        record = payTv.storageMap(status);
    } else if (productName == "startimes") {
        PayTV payTv(vasMenuString(TV_SUBSCRIPTIONS), postman);
        status = payTv.processPaymentResponse(response, STARTIMES);
        record = payTv.storageMap(status);
    } else if (productName == "mtnvtu") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, MTNVTU);
        record = airtime.storageMap(status);
    } else if (productName == "glovtu") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, GLOVTU);
        record = airtime.storageMap(status);
    } else if (productName == "airtelvtu") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, AIRTELVTU);
        record = airtime.storageMap(status);
    } else if (productName == "etisalatvtu") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, ETISALATVTU);
        record = airtime.storageMap(status);
    } else if (productName == "mtndata") {
        Data data(vasMenuString(DATA), postman);
        status = data.processPaymentResponse(response, MTNDATA);
        record = data.storageMap(status);
    } else if (productName == "glodata") {
        Data data(vasMenuString(DATA), postman);
        status = data.processPaymentResponse(response, GLODATA);
        record = data.storageMap(status);
    } else if (productName == "airteldata") {
        Data data(vasMenuString(DATA), postman);
        status = data.processPaymentResponse(response, AIRTELDATA);
        record = data.storageMap(status);
    } else if (productName == "etisalatdata") {
        Data data(vasMenuString(DATA), postman);
        status = data.processPaymentResponse(response, ETISALATDATA);
        record = data.storageMap(status);
    } else if (productName == "glovot") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, GLOVOT);
        record = airtime.storageMap(status);
    } else if (productName == "airtelvot") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, AIRTELVOT);
        record = airtime.storageMap(status);
    } else if (productName == "glovos") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, GLOVOS);
        record = airtime.storageMap(status);
    } else if (productName == "airtelvos") {
        Airtime airtime(vasMenuString(AIRTIME), postman);
        status = airtime.processPaymentResponse(response, AIRTELVOS);
        record = airtime.storageMap(status);
    }

    iisys::JSObject& account = transaction("VASCustomerAccount");
    iisys::JSObject& paymentMethod = transaction("paymentMethod");
    iisys::JSObject& address = transaction("VASCustomerAddress");
    iisys::JSObject& phone = transaction("VASCustomerPhone");
    iisys::JSObject& name = transaction("VASCustomerName");
    iisys::JSObject& seq = transaction("sequence");
    iisys::JSObject& product = transaction("product");
    iisys::JSObject& wallet = transaction("wallet");
    iisys::JSObject& tid = transaction("terminal");


    if (name.isString()) {
        record[VASDB_BENEFICIARY_NAME] = name.getString();
    }

    if (account.isString()) {
        record[VASDB_BENEFICIARY] = account.getString();
    }

    if (phone.isString()) {
        record[VASDB_BENEFICIARY_PHONE] = phone.getString();
    }

    if (product.isString()) {
        record[VASDB_PRODUCT] = product.getString();
    }

    if (paymentMethod.isString()) {
        record[VASDB_PAYMENT_METHOD] = paymentMethod.getString();
    }

    if (address.isString()) {
        record[VASDB_BENEFICIARY_ADDR] = address.getString();
    }

    if (seq.isString()) {
        record[VASDB_TRANS_SEQ] = seq.getString();
    }
    

    record[VASDB_AMOUNT] = transaction("amount").getString();
    record[VASDB_DATE] = transaction("date").getString();

    iisys::JSObject& cardName = transaction("cardName");
    if (!cardName.isNull()) {
        record[DB_NAME] = cardName.getString();
    }

    iisys::JSObject& cardPan = transaction("cardPAN");
    if (!cardPan.isNull()) {
        record[DB_PAN] = cardPan.getString();
    }

    iisys::JSObject& cardExpiry = transaction("cardExpiry");
    if (!cardExpiry.isNull()) {
        record[DB_EXPDATE] = cardExpiry.getString();
    }

    iisys::JSObject& cardStan = transaction("transactionSTAN");
    if (!cardStan.isNull()) {
        record[DB_STAN] = cardStan.getString();
    }

    iisys::JSObject& cardAuthID = transaction("transactionAuthCode");
    if (!cardAuthID.isNull()) {
        record[DB_AUTHID] = cardAuthID.getString();
    }

    iisys::JSObject& cardRef = transaction("transactionRRN");
    if (!cardRef.isNull()) {
        record[DB_RRN] = cardRef.getString();
    }

    iisys::JSObject& cardRespCode = transaction("transactionResponseCode");
    if (!cardRespCode.isNull()) {
        record[DB_RESP] = cardRespCode.getString();
    }

    iisys::JSObject& cardTid = transaction("virtualTID");
    if (!cardTid.isNull()) {
        record[VASDB_VIRTUAL_TID] = cardTid.getString();
    }

    record["walletId"] = wallet.getString();
    record["terminalId"] = tid.getString();
    record["receipt_copy"] += "- Reprint -";
    printVas(record);

    return 0;
}

void vasAdmin()
{
    Payvice payvice;
    const char* optionStr[] = { "Requery", "End of Day", "Reprint Today", "Reprint with Date", "Balance Enquiry", "Commission Transfer", "Log Out" };

    if(!loggedIn(payvice) && logIn(payvice) < 0) {
        return;
    }

    std::vector<std::string> optionMenu(optionStr, optionStr + sizeof(optionStr) / sizeof(char*));
    switch (UI_ShowSelection(30000, "Vas Admin", optionMenu, 0)) {
    case 0: {
        iisys::JSObject transaction;

        std::string seqNumber = getNumeric(0, 30000, "Sequence No", "", UI_DIALOG_TYPE_NONE);
      
        if (seqNumber.empty()) {
            return;
        }

        VasError requeryErr = requeryVas(transaction, NULL, seqNumber.c_str());
        if (requeryErr == TXN_NOT_FOUND) {
            UI_ShowButtonMessage(2000, "Transaction Not Found", "", "OK", UI_DIALOG_TYPE_CAUTION);
        } else if (requeryErr == TXN_PENDING) {
            UI_ShowButtonMessage(2000, "Transaction Pending", "", "OK", UI_DIALOG_TYPE_CAUTION);
        } else if (requeryErr != NO_ERRORS && transaction.isNull()) {
            UI_ShowButtonMessage(2000, "Requery Failed", "", "OK", UI_DIALOG_TYPE_CAUTION);
        }

        printRequery(transaction);

    }   break;
    case 1:
        vasEndofDay();
        break;
    case 2:
        vasReprintToday();
        break;
    case 3:
        vasReprintByDate();
        break;
    case 4:
        walletRequest(1);
        break;   
    case 5:
        walletRequest(2);
        break;

    case 6:
        if(UI_ShowOkCancelMessage(2000, "Notification", "Do you wish to log out?", UI_DIALOG_TYPE_CAUTION) == CONFIRM) {
            // log out 
            if (logOut(payvice) == 0) {
                // VASDB_FILE, but it contains the directory as well, itex/vas.db
                removeFile("vas.db");
                VasDB::init();
            }
  
        }

        break;

    default:
        break;
    }
}
