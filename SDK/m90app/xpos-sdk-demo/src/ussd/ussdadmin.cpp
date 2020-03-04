#include "vas/simpio.h"
#include "vas/jsobject.h"
#include "EmvDBUtil.h"
#include "Receipt.h"

#include "ussdb.h"
#include "ussdservices.h"

#include "ussdadmin.h"

int printUSSDReceipt(std::map<std::string, std::string>& record, const char* printfile)
{
    int printStatus = -1;

    fillReceiptHeader(record);
    fillReceiptFooter(record);

    formatAmount(record[USSD_AMOUNT]);
    record["currencySymbol"] = "NGN";
    if(record.find(USSD_DATE) != record.end() && record[USSD_DATE].length() > 19) {
        record[USSD_DATE].erase(19, std::string::npos);
    }

    const char* copies[] = {"- Customer Copy -", "- Agent Copy -"};    
    int count = 1;
    if (record.find("receipt_copy") == record.end()) {
        if (record[USSD_STATUS] == USSDB::trxStatusString(USSDB::APPROVED)) {
            count = (int) sizeof(copies) / sizeof(char*);
        }
        record["receipt_copy"] = copies[0];
    }

    for (int i = 0; i < count; ++i) {
        if (i > 0) {
            if (UI_ShowOkCancelMessage(10000, copies[i], "Print", UI_DIALOG_TYPE_CONFIRMATION) != 0) {
                break;
            }
            record["receipt_copy"] = copies[i];
        }
        // printStatus = checkedPrint(record, printfile);       // Just Print here
    }


    return printStatus;
}

int showUssdTransactions(int timeout, const char* title, std::vector<std::map<std::string, std::string> >& elements, int preselect)
{
    std::string amountStr;
    std::vector<std::string> transvector;
    std::string amountSymbol = "NGN";

    for (size_t i = 0; i < elements.size(); ++i) {
        std::map<std::string, std::string>& element = elements[i];
        const std::string& status = element[USSD_STATUS];
        amountStr = element[USSD_AMOUNT];
        formatAmount(amountStr);

        transvector.push_back(element[USSD_SERVICE] + '\n'
            + status + ", " + amountSymbol + " " + amountStr
            + "\nREF: " + element[USSD_REF]);
    }

    return UI_ShowSelection(timeout, title, transvector, preselect);
}

bool listUssdTransactions(USSDB& db, const std::string& dateTime, const std::string& service, const char* title = NULL)
{
    std::vector<std::map<std::string, std::string> > transactions;

    if (db.selectTransactionsOnDate(transactions, dateTime.c_str(), service) < 0) {
        return false;
    }

    if (!transactions.size()) {
        UI_ShowButtonMessage(2000, "No Transactions For Specified Date", dateTime.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        return false;
    }

    int selection = showUssdTransactions(60000, title ? title : service.c_str(), transactions, 0);

    if (selection < 0) {
        return false;
    }

    std::map<std::string, std::string>& record = transactions[selection];
    
    std::string receiptfile;
    const std::string& actualService = record[USSD_SERVICE] ;
    if (actualService == ussdServiceToString(CGATE_PURCHASE) || actualService == ussdServiceToString(CGATE_CASHOUT)) {
        receiptfile = "cgate.html";
    } else if (actualService == ussdServiceToString(MCASH_PURCHASE)) {
        receiptfile = "mcash.html";
    } else if (actualService == ussdServiceToString(PAYATTITUDE_PURCHASE)) {
        receiptfile = "payWithPhoneReceipt.html";
    }

    record["receipt_copy"] += "- Reprint -";

    /*
    if (printUSSDReceipt(record, receiptfile.c_str()) != vfiprt::PRT_OK) {
        return false;
    }
    */

    return true;
}

int requestDateAndService(USSDB& database, std::string& date, std::string& service)
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

int ussdEodMap(USSDB& db, const char* date, const std::string& service, std::map<std::string, std::string>& values)
{
    std::string amountApproved;
    std::string approvedCount;
    std::string amountDeclined;
    std::string declinedCount;
    std::string totalCount;
    std::string amountStr;

    std::string amountSymbol = "NGN";

    int ret = -1;

    if (db.sumTransactionsOnDate(amountApproved, date, USSDB::APPROVED, service)) {
        return ret;
    } else if (db.countTransactionsOnDate(approvedCount, date, USSDB::APPROVED, service)) {
        return ret;
    } else if (db.sumTransactionsOnDate(amountDeclined, date, USSDB::DECLINED, service)) {
        return ret;
    } else if (db.countTransactionsOnDate(declinedCount, date, USSDB::DECLINED, service)) {
        return ret;
    } else if (db.countTransactionsOnDate(totalCount, date, USSDB::ALL, service)) {
        return ret;
    }

    std::vector<std::map<std::string, std::string> > transactions;
    if (db.selectTransactionsOnDate(transactions, date, service) < 0) {
        return -1;
    }

    iisys::JSObject jHelp;
    using namespace vfigui;

    fillReceiptHeader(values);
    fillReceiptFooter(values);
    values["date"] = date;

    for (size_t i = 0; i < transactions.size(); ++i) {
        std::map<std::string, std::string>& element = transactions[i];
        
        const std::string& status = element[USSD_STATUS];
        if (status == USSDB::trxStatusString(USSDB::APPROVED)) {
            jHelp[i]("status") = " A ";
        } else if (status == USSDB::trxStatusString(USSDB::DECLINED)) {
            jHelp[i]("status") = " D ";
        } else if (status == USSDB::trxStatusString(USSDB::STATUS_UNKNOWN)) {
            jHelp[i]("status") = " U ";
        } else {
            jHelp[i]("status") = " UU ";
        }

        amountStr = element[USSD_AMOUNT];
        formatAmount(amountStr);

        if (service.empty()) {
            jHelp[i]("preferredField") = element[USSD_SERVICE];
        } else {
            jHelp[i]("preferredField") = element[USSD_REF];
        }
        jHelp[i]("amount") = amountStr;
        jHelp[i]("tstamp") = element[USSD_DATE].substr(11, 5);
    }
    values["menu"] = jHelp.dump();

    formatAmount(amountApproved);
    formatAmount(amountDeclined);

    values["approvedAmount"] = amountSymbol + " " + amountApproved;
    values["declinedAmount"] = amountSymbol + " " + amountDeclined;

    values["approvedCount"] = approvedCount;
    values["declinedCount"] = declinedCount;
    values["totalCount"] = totalCount;

    return 0;
}

bool ussdReprintByDate()
{
    std::string date, service;
    USSDB database;

    while (1) {
        if (requestDateAndService(database, date, service) < 0) {
            return false;
        }
        break;
    }

    return listUssdTransactions(database, date, service, service.empty() ? "All Transactions" : service.c_str());
}

void ussdEndofDay()
{
    std::string date, service;
    USSDB database;

    while (1) {
        if (requestDateAndService(database, date, service) < 0) {
            return;
        }

        std::map<std::string, std::string> values;
        if (ussdEodMap(database, date.c_str(), service, values) < 0) {
            continue;
        }

        values["trxType"] = service.empty() ? "Services" : service;
        if (previewReceiptMap(values, 0, "../print/eodReceipt.html") == 0) {
            checkedPrint(values, "eodReceipt.html");
        }
    }
}

int ussdAdmin()
{
    const char* optionStr[] = { "Reprint", "End of Day" };
    std::vector<std::string> optionMenu(optionStr, optionStr + sizeof(optionStr) / sizeof(char*));

    switch (UI_ShowSelection(30000, "USSD Admin", optionMenu, 0)) {
    case 0:
        ussdReprintByDate();
        break;
    default:
        ussdEndofDay();
        break;
    }

    return 0;
}

