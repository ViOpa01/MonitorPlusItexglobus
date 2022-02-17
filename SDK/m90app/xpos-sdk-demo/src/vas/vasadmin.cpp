#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <vector>
#include <algorithm>

#include "jsonwrapper/jsobject.h"

#include "platform/platform.h"

#include "viewmodels/cashioViewModel.h"
#include "viewmodels/electricityViewModel.h"
#include "viewmodels/airtimeViewModel.h"
#include "viewmodels/dataViewModel.h"
#include "viewmodels/smileViewModel.h"
#include "viewmodels/paytvViewModel.h"
#include "viewmodels/jambviewmodel.h"
#include "viewmodels/waecviewmodel.h"
#include "viewmodels/airtimeUssdViewModel.h"
#include "viewmodels/dataUssdViewModel.h"
#include "viewmodels/electricityUssdViewModel.h"
#include "viewmodels/paytvUssdViewModel.h"

#include "payvice.h"
#include "vas.h"
#include "vasdb.h"
#include "vasmenu.h"
#include "wallet.h"
#include "vasadmin.h"

extern "C" {
#include "../itexFile.h"
}

#define VASADMIN "VASADMIN"

int vasEodMap(VasDB& db, const char* date, const std::string& service, std::map<std::string, std::string>& values)
{
    // LogManager log(VASADMIN);
    std::string amountApproved;
    std::string approvedCount;
    std::string amountDeclined;
    std::string declinedCount;
    std::string totalCount;
    std::string amountStr;

    std::string amountSymbol = getVasCurrencySymbol();

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

    // fillReceiptHeader(values);
    // fillReceiptFooter(values);

    values[VASDB_DATE] = date;

    for (size_t i = 0; i < transactions.size(); ++i) {
        std::map<std::string, std::string>& element = transactions[i];
        
        const std::string& status = element[VASDB_STATUS];
        if (status == VasDB::trxStatusString(VasDB::APPROVED)) {
            jHelp[i]("status") = " A ";
        } else if (status == VasDB::trxStatusString(VasDB::DECLINED)) {
            jHelp[i]("status") = " D ";
        } else if (status == VasDB::trxStatusString(VasDB::CARDAPPROVED)) {
            jHelp[i]("status") = " CA ";
        } else if (status == VasDB::trxStatusString(VasDB::PENDING)) {
            jHelp[i]("status") = " P ";
        } else if (status == VasDB::trxStatusString(VasDB::STATUS_UNKNOWN)) {
            jHelp[i]("status") = " U ";
        } else {
            jHelp[i]("status") = " UU ";
        }

        amountStr = element[VASDB_AMOUNT];
        vasimpl::formatAmount(amountStr);

        if (service.empty()) {
            jHelp[i]("preferredField") = element[VASDB_SERVICE];
        } else {
            jHelp[i]("preferredField") = element[VASDB_TRANS_SEQ];
        }
        jHelp[i]("amount") = amountStr;
        jHelp[i]("tstamp") = element[VASDB_DATE].substr(11, 5);
    }
    values["menu"] = jHelp.dump();

    // LOGF_INFO(log.handle, "Approved -> %s, Declined -> %s", amountApproved.c_str(), amountDeclined.c_str());
    vasimpl::formatAmount(amountApproved);
    vasimpl::formatAmount(amountDeclined);
    // LOGF_INFO(log.handle, "Approved -> %s, Declined -> %s", amountApproved.c_str(), amountDeclined.c_str());

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
        UI_ShowButtonMessage(2000, "No Records", "", "OK", UI_DIALOG_TYPE_CAUTION);
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
    std::string amountSymbol = getVasCurrencySymbol();

    for (size_t i = 0; i < elements.size(); ++i) {
        std::map<std::string, std::string>& element = elements[i];
        // int approved = !strcmp(element[VASDB_STATUS].c_str(), VasDB::trxStatusString(VasDB::APPROVED));
        const std::string& status = element[VASDB_STATUS];
        amountStr = element[VASDB_AMOUNT];
        vasimpl::formatAmount(amountStr);

        transvector.push_back(element[VASDB_SERVICE] + vasimpl::menuendl()
            + status + " " + amountSymbol + " " + amountStr
            + vasimpl::menuendl() + "TRANS SEQ: " + element[VASDB_TRANS_SEQ]);
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
        UI_ShowButtonMessage(2000, "No Transactions For Specified Date", dateTime.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        return false;
    }

    int selection = showVasTransactions(60000, title ? title : service.c_str(), transactions, 0);

    if (selection < 0) {
        return false;
    }

    std::map<std::string, std::string>& record = transactions[selection];
    // fillReceiptHeader(record);
    // fillReceiptFooter(record);
    record["walletId"] = Payvice().object(Payvice::WALLETID).getString();
    record["receipt_copy"] += "- Reprint -";
    if (printVas(record) != 0) {
        return false;
    }

    return true;
}

bool vasReprintToday()
{
    VasDB db;
    char dateTime[32] = { 0 };

    Demo_SplashScreen("Loading Transactions...", "www.payvice.com");
    vasimpl::formattedDateTime(dateTime, sizeof(dateTime));
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

static iisys::JSObject reconstructOriginalResponse(const iisys::JSObject& requeryData)
{
    // Had to reconstruct this to avoid server response inconsistencies
    iisys::JSObject response;

    const iisys::JSObject& tempResponse = requeryData("response");

    if (!tempResponse.isObject()) {
        return response;
    }

    response("responseCode") = tempResponse("responseCode");
    response("message") = tempResponse("message");
    response("data") = tempResponse;

    return response;
}

int printRequery(const iisys::JSObject& transaction)
{
    Postman postman;
    std::string productName;
    std::string statusMsg;
    VasResult status;
    std::map<std::string, std::string> record;
    
    statusMsg = transaction("status").getString();
    productName = transaction("product").getString();

    // enum('status', ['initialized', 'debited', 'pending', 'successful', 'failed', 'declined'])->default('initialized');
    if (statusMsg != "successful" && statusMsg != "failed" && statusMsg != "declined") {
        return -1;
    }

    std::transform(productName.begin(), productName.end(), productName.begin(), ::tolower);

    const iisys::JSObject response = reconstructOriginalResponse(transaction);

    if (productName == "ikedc") {
        ElectricityViewModel electricity(vasMenuString(ENERGY), postman);
        if (electricity.setService(IKEJA).error != NO_ERRORS) {
            return -1;
        }
        status = electricity.processPaymentResponse(response);
        record = electricity.storageMap(status);
    } else if (productName == "ekedc") {
        ElectricityViewModel electricity(vasMenuString(ENERGY), postman);
        if (electricity.setService(EKEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricity.processPaymentResponse(response);
        record = electricity.storageMap(status);
    } else if (productName == "eedc") {
        ElectricityViewModel electricity(vasMenuString(ENERGY), postman);
        if (electricity.setService(EEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricity.processPaymentResponse(response);
        record = electricity.storageMap(status);
    } else if (productName == "ibedc") {
        ElectricityViewModel electricity(vasMenuString(ENERGY), postman);
        if (electricity.setService(IBEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricity.processPaymentResponse(response);
        record = electricity.storageMap(status);
    } else if (productName == "phed") {
        ElectricityViewModel electricity(vasMenuString(ENERGY), postman);
        if (electricity.setService(PHED).error != NO_ERRORS) {
            return -1;
        }
        status = electricity.processPaymentResponse(response);
        record = electricity.storageMap(status);
    } else if (productName == "aedc") {
        ElectricityViewModel electricity(vasMenuString(ENERGY), postman);
        if (electricity.setService(AEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricity.processPaymentResponse(response);
        record = electricity.storageMap(status);
    } else if (productName == "kedco") {
        ElectricityViewModel electricity(vasMenuString(ENERGY), postman);
        if (electricity.setService(KEDCO).error != NO_ERRORS) {
            return -1;
        }
        status = electricity.processPaymentResponse(response);
        record = electricity.storageMap(status);
    } else if (productName == "withdrawal") {
        ViceBankingViewModel viceBanking(vasMenuString(CASHOUT), postman);
        viceBanking.setService(WITHDRAWAL);
        status = viceBanking.processPaymentResponse(response);
        record = viceBanking.storageMap(status);
    } else if (productName == "transfer") {
        ViceBankingViewModel viceBanking(vasMenuString(CASHIN), postman);
        viceBanking.setService(TRANSFER);
        status = viceBanking.processPaymentResponse(response);
        record = viceBanking.storageMap(status);
        const iisys::JSObject& bankName = transaction("bankName");
        if (bankName.isString()) {
            record[VASDB_SERVICE_DATA] = std::string("{\"recBank\": \"") + bankName.getString() + "\"}";
        }
    } else if (productName == "gotv") {
        PayTVViewModel payTv(vasMenuString(TV_SUBSCRIPTIONS), postman);
        if (payTv.setService(GOTV).error != NO_ERRORS){
            return -1;
        }
        status = payTv.processPaymentResponse(response, GOTV);
        record = payTv.storageMap(status);
    } else if (productName == "dstv") {
        PayTVViewModel payTv(vasMenuString(TV_SUBSCRIPTIONS), postman);
        if (payTv.setService(DSTV).error != NO_ERRORS){
            return -1;
        }
        status = payTv.processPaymentResponse(response, DSTV);
        record = payTv.storageMap(status);
    } else if (productName == "startimes") {
        PayTVViewModel payTv(vasMenuString(TV_SUBSCRIPTIONS), postman);
        if (payTv.setService(STARTIMES).error != NO_ERRORS){
            return -1;
        }
        status = payTv.processPaymentResponse(response, STARTIMES);
        record = payTv.storageMap(status);
    } else if (productName == "mtnvtu" || strncmp(productName.c_str(), "mtnvtu", 6) == 0) {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(MTNVTU).error != NO_ERRORS) {
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "glovtu") {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(GLOVTU).error != NO_ERRORS) {
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "airtelvtu") {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(AIRTELVTU).error != NO_ERRORS) {
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "etisalatvtu") {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(ETISALATVTU).error != NO_ERRORS) {
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "mtndata") {
        DataViewModel data(vasMenuString(DATA), postman);
        if (data.setService(MTNDATA). error != NO_ERRORS) {
            return -1;
        }
        status = data.processPaymentResponse(response);
        record = data.storageMap(status);
    } else if (productName == "glodata") {
        DataViewModel data(vasMenuString(DATA), postman);
        if (data.setService(GLODATA).error != NO_ERRORS) {
            return -1;
        }
        status = data.processPaymentResponse(response);
        record = data.storageMap(status);
    } else if (productName == "airteldata") {
        DataViewModel data(vasMenuString(DATA), postman);
        if (data.setService(AIRTELDATA).error != NO_ERRORS) {
            return -1;
        }
        status = data.processPaymentResponse(response);
        record = data.storageMap(status);
    } else if (productName == "etisalatdata") {
        DataViewModel data(vasMenuString(DATA), postman);
        if (data.setService(ETISALATDATA).error != NO_ERRORS) {
            return -1;
        }
        status = data.processPaymentResponse(response);
        record = data.storageMap(status);
    } else if (productName == "glovot") {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(GLOVOT).error != NO_ERRORS){
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "airtelvot") {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(AIRTELVOT).error != NO_ERRORS) {
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "glovos") {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(GLOVOS).error != NO_ERRORS) {
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "airtelvos") {
        AirtimeViewModel airtime(vasMenuString(AIRTIME), postman);
        if (airtime.setService(AIRTELVOS).error != NO_ERRORS) {
            return -1;
        }
        status = airtime.processPaymentResponse(response);
        record = airtime.storageMap(status);
    } else if (productName == "smile") {
        SmileViewModel smile(vasMenuString(SMILE), postman);
        const iisys::JSObject& nameObj = transaction("name");
        std::string name;

        if (nameObj.isString()) {
            name = nameObj.getString();
        }

        if (name == "Smile Bundle" && smile.setService(SMILEBUNDLE).error != NO_ERRORS) {
            return -1;
        } else if (name == "Smile Top Up" && smile.setService(SMILETOPUP).error != NO_ERRORS) {
            return -1;
        }
        status = smile.processPaymentResponse(response);
        record = smile.storageMap(status);
    } else if (productName == "jambutme") {
        JambViewModel jamb(vasMenuString(JAMB_EPIN), postman);
        if (jamb.setService(JAMB_UTME_PIN).error != NO_ERRORS) {
            return -1;
        }
        status = jamb.processPaymentResponse(response);
        record = jamb.storageMap(status);
    } else if (productName == "jambde") {
        JambViewModel jamb(vasMenuString(JAMB_EPIN), postman);
        if (jamb.setService(JAMB_DE_PIN).error != NO_ERRORS) {
            return -1;
        }
        status = jamb.processPaymentResponse(response);
        record = jamb.storageMap(status);
    } else if (productName == "waecreg") {
        WaecViewModel waec(vasMenuString(WAEC), postman);
        if (waec.setService(WAEC_REGISTRATION).error != NO_ERRORS) {
            return -1;
        }
        status = waec.processPaymentResponse(response);
        record = waec.storageMap(status);
    } else if (productName == "waecresult") {
        WaecViewModel waec(vasMenuString(WAEC), postman);
        if (waec.setService(WAEC_RESULT_CHECKER).error != NO_ERRORS) {
            return -1;
        }
        status = waec.processPaymentResponse(response);
        record = waec.storageMap(status);
    } else if (productName == "airtimepin") {
        AirtimeUssdViewModel airtimeUssd(vasMenuString(VAS_USSD), postman);
        if (airtimeUssd.getService() != AIRTIME_USSD) {
            return -1;
        }
        status = airtimeUssd.processPaymentResponse(response);
        record = airtimeUssd.storageMap(status);
    } else if (productName == "9mobiledatapin") {
        DataUssdViewModel dataUssd(vasMenuString(VAS_USSD), postman);
        if (dataUssd.setService(ETISALATDATA).error != NO_ERRORS) {
            return -1;
        }
        status = dataUssd.processPaymentResponse(response);
        record = dataUssd.storageMap(status);
    } else if (productName == "airteldatapin") {
        DataUssdViewModel dataUssd(vasMenuString(VAS_USSD), postman);
        if (dataUssd.setService(AIRTELDATA).error != NO_ERRORS) {
            return -1;
        }
        status = dataUssd.processPaymentResponse(response);
        record = dataUssd.storageMap(status);
    } else if (productName == "glodatapin") {
        DataUssdViewModel dataUssd(vasMenuString(VAS_USSD), postman);
        if (dataUssd.setService(GLODATA).error != NO_ERRORS) {
            return -1;
        }
        status = dataUssd.processPaymentResponse(response);
        record = dataUssd.storageMap(status);
    } else if (productName == "mtndatapin") {
        DataUssdViewModel dataUssd(vasMenuString(VAS_USSD), postman);
        if (dataUssd.setService(MTNDATA).error != NO_ERRORS) {
            return -1;
        }
        status = dataUssd.processPaymentResponse(response);
        record = dataUssd.storageMap(status);
    } else if (productName == "multichoicepin") {
        PaytvUssdViewModel paytvUssd(vasMenuString(VAS_USSD), postman);
        if (paytvUssd.setService(DSTV).error != NO_ERRORS) {
            return -1;
        }
        status = paytvUssd.processPaymentResponse(response);
        record = paytvUssd.storageMap(status);
    } else if (productName == "startimespin") {
        PaytvUssdViewModel paytvUssd(vasMenuString(VAS_USSD), postman);
        if (paytvUssd.setService(STARTIMES).error != NO_ERRORS) {
            return -1;
        }
        status = paytvUssd.processPaymentResponse(response);
        record = paytvUssd.storageMap(status);
    } else if (productName == "ikedcpin") {
        ElectricityUssdViewModel electricityUssd(vasMenuString(VAS_USSD), postman);
        if (electricityUssd.setService(IKEJA).error != NO_ERRORS) {
            return -1;
        }
        status = electricityUssd.processPaymentResponse(response);
        record = electricityUssd.storageMap(status);
    } else if (productName == "ekedcpin") {
        ElectricityUssdViewModel electricityUssd(vasMenuString(VAS_USSD), postman);
        if (electricityUssd.setService(EKEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricityUssd.processPaymentResponse(response);
        record = electricityUssd.storageMap(status);
    } else if (productName == "eedcpin") {
        ElectricityUssdViewModel electricityUssd(vasMenuString(VAS_USSD), postman);
        if (electricityUssd.setService(EEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricityUssd.processPaymentResponse(response);
        record = electricityUssd.storageMap(status);
    } else if (productName == "ibedcpin") {
        ElectricityUssdViewModel electricityUssd(vasMenuString(VAS_USSD), postman);
        if (electricityUssd.setService(IBEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricityUssd.processPaymentResponse(response);
        record = electricityUssd.storageMap(status);
    } else if (productName == "phedcpin") {
        ElectricityUssdViewModel electricityUssd(vasMenuString(VAS_USSD), postman);
        if (electricityUssd.setService(PHED).error != NO_ERRORS) {
            return -1;
        }
        status = electricityUssd.processPaymentResponse(response);
        record = electricityUssd.storageMap(status);
    } else if (productName == "aedcpin") {
        ElectricityUssdViewModel electricityUssd(vasMenuString(VAS_USSD), postman);
        if (electricityUssd.setService(AEDC).error != NO_ERRORS) {
            return -1;
        }
        status = electricityUssd.processPaymentResponse(response);
        record = electricityUssd.storageMap(status);
    } else if (productName == "kedcopin") {
        ElectricityUssdViewModel electricityUssd(vasMenuString(VAS_USSD), postman);
        if (electricityUssd.setService(KEDCO).error != NO_ERRORS) {
            return -1;
        }
        status = electricityUssd.processPaymentResponse(response);
        record = electricityUssd.storageMap(status);
    }

    const iisys::JSObject& account = transaction("VASCustomerAccount");
    const iisys::JSObject& paymentMethod = transaction("paymentMethod");
    const iisys::JSObject& address = transaction("VASCustomerAddress");
    const iisys::JSObject& phone = transaction("customerPhoneNumber");
    const iisys::JSObject& name = transaction("VASCustomerName");
    const iisys::JSObject& seq = transaction("sequence");
    const iisys::JSObject& product = transaction("product");
    const iisys::JSObject& wallet = transaction("wallet");
    const iisys::JSObject& tid = transaction("terminal");


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
    
    const unsigned long amount = (unsigned long) lround(transaction("amount").getNumber() * 100.0f);
    record[VASDB_AMOUNT] = vasimpl::to_string(amount);
    record[VASDB_DATE] = transaction("created_at").getString();

    const iisys::JSObject& cardName = transaction("cardName");
    if (!cardName.isNull()) {
        record[DB_NAME] = cardName.getString();
    }

    const iisys::JSObject& cardPan = transaction("mPan");
    if (!cardPan.isNull()) {
        record[DB_PAN] = cardPan.getString();
    }

    const iisys::JSObject& cardExpiry = transaction("expiry");
    if (!cardExpiry.isNull()) {
        record[DB_EXPDATE] = cardExpiry.getString();
    }

    const iisys::JSObject& cardStan = transaction("stan");
    if (!cardStan.isNull()) {
        record[DB_STAN] = cardStan.getString();
    }

    const iisys::JSObject& cardRef = transaction("rrn");
    if (!cardRef.isNull()) {
        record[DB_RRN] = cardRef.getString();
    }

    const iisys::JSObject& cardTid = transaction("vTid");
    if (!cardTid.isNull()) {
        record[VASDB_VIRTUAL_TID] = cardTid.getString();
    }

    const iisys::JSObject& card = transaction("card");
    if (card.isObject()) {
        const iisys::JSObject& cardAuthID = card("acode");
        if (!cardAuthID.isNull()) {
            record[DB_AUTHID] = cardAuthID.getString();
        }

        const iisys::JSObject& cardRespCode = card("resp");
        if (!cardRespCode.isNull()) {
            record[DB_RESP] = cardRespCode.getString();
        }
    }

    // typedef std::vector<std::map<std::string, std::string> >::const_iterator vi;
    // typedef std::map<std::string, std::string>::const_iterator mi;
    // for(mi col = record.begin(); col != record.end(); ++col) 
    //     LOGF_INFO(LogManager("TLITE").handle, "(%s) -> (%s : %s)", __FUNCTION__, col->first.c_str(), col->second.c_str());


    // fillReceiptHeader(record);
    // fillReceiptFooter(record);
    record["walletId"] = wallet.getString();
    if (!tid.isNull()) {
        record["terminalId"] = tid.getString();
    }
    if (record[VASDB_CATEGORY] == vasMenuString(ENERGY)) {
        record["receipt_copy"] += "- Customer Copy -";
    } else {
        record["receipt_copy"] += "- Requery Reprint -";
    }
    printVas(record);

    return 0;
}

static bool isSmartCardRequery(const iisys::JSObject& data)
{
    const iisys::JSObject& response = data("response");
    const iisys::JSObject& accountTypeObj = data("VASAccountType");

    std::string product = data("product").getString();
    std::string vasAccountType = accountTypeObj.isString() ? accountTypeObj.getString() : "";

    std::transform(product.begin(), product.end(), product.begin(), ::tolower);
    std::transform(vasAccountType.begin(), vasAccountType.end(), vasAccountType.begin(), ::tolower);

    if (product == "ikedc" && vasAccountType == "smartcard") {
        return true;
    }

    return false;
}

void vasRequery()
{
    iisys::JSObject transaction;
    std::string seqNumber;
    std::string cardRef;

    const char* optionStr[] = { "Sequence", "Card Reference"};
    std::vector<std::string> optionMenu(optionStr, optionStr + sizeof(optionStr) / sizeof(char*));

    switch (UI_ShowSelection(30000, "Options", optionMenu, 0)) {
    case 0:
        seqNumber = getNumeric(0, 30000, "Sequence No", "", UI_DIALOG_TYPE_NONE);
        if (seqNumber.empty()) {
            return;
        }
        break;
    case 1:
        cardRef = getNumeric(0, 30000, "Card Reference", "", UI_DIALOG_TYPE_NONE);
        if (cardRef.empty()) {
            return;
        }
        break;
    default:
        return;
    }

    Demo_SplashScreen("Please wait...", "www.payvice.com");
    VasError requeryErr = Postman::manualRequery(transaction, seqNumber, cardRef);
    if (requeryErr == TXN_NOT_FOUND) {
        UI_ShowButtonMessage(2000, "Transaction Not Found", "", "OK", UI_DIALOG_TYPE_CAUTION);
    } else if (requeryErr == TXN_PENDING) {
        UI_ShowButtonMessage(2000, "Transaction Pending", "", "OK", UI_DIALOG_TYPE_CAUTION);
    } else if (requeryErr != NO_ERRORS && transaction.isNull()) {
        UI_ShowButtonMessage(2000, "Requery Failed", "", "OK", UI_DIALOG_TYPE_CAUTION);
    }

    
    if (isSmartCardRequery(transaction)) {
        VasResult revalidate = ElectricityViewModel::revalidateSmartCard(transaction);
        if (revalidate.error != NO_ERRORS) {
            UI_ShowButtonMessage(2000, "Error", revalidate.message.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        }

        UI_ShowOkCancelMessage(2000, "SMART CARD", revalidate.message.c_str(), UI_DIALOG_TYPE_NONE);
    }

    printRequery(transaction);
}

int vasAdmin()
{
    Payvice payvice;
    const char* optionStr[] = { "Requery", "End of Day", "Reprint Today", "Reprint with Date", "Fund Wallet", "Balance Enquiry", "Commission Transfer", "Log out"};

    if (!loggedIn(payvice)) {
        logIn(payvice);
        return -1;
    }

    std::vector<std::string> optionMenu(optionStr, optionStr + sizeof(optionStr) / sizeof(char*));
    switch (UI_ShowSelection(30000, "Vas Admin", optionMenu, 0)) {
    case 0:
        vasRequery();
        break;
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
        fundPayviceWallet();
        break;
    case 5:
        walletBalance();
        break;
    case 6:
        walletTransfer();
        break;
    case 7:
        if (UI_ShowOkCancelMessage(10000, "Confirm Logout", "Are you sure you want to log out?", UI_DIALOG_TYPE_CAUTION)) {
            break;
        } else if (logOut(payvice) == 0) {
            remove(VASDB_FILE);
            initVasTables();
        }
        return -1;
    default:
        break;
    }

    return 0;
}
