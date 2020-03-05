#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vas/simpio.h"
#include "vas/jsobject.h"
#include "EmvDBUtil.h"
#include "Receipt.h"

#include "ussdb.h"
#include "ussdservices.h"

#include "ussdadmin.h"

extern "C"{
#include "remoteLogo.h"
#include "libapi_xpos/inc/libapi_print.h"
}

static int formatAmount(std::string& ulAmount)
{
    int position;

    if (ulAmount.empty()) return -1;

    position = ulAmount.length();
    if ((position -= 2) > 0) {
        ulAmount.insert(position, ".");
    } else if (position == 0) {
        ulAmount.insert(0, "0.");
    } else {
        ulAmount.insert(0, "0.0");
    }

    while ((position -= 3) > 0) {
        ulAmount.insert(position, ",");
    }

    return 0;
}

void printPayAttitude(std::map<std::string, std::string> &record)
{
    const char* keys[] = {USSD_REF, USSD_PHONE};
    const char* labels[] = {"TRANS. REF", "MOBILE"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

void printMcash(std::map<std::string, std::string> &record)
{
    const char* keys[] = {USSD_DATE, USSD_REF, USSD_OP_ID, USSD_CHARGE_REQ_ID, USSD_PHONE};
    const char* labels[] = {"DATE","RECEIPT ID", "OPRN ID","CHARGE RQST ID", "PHONE"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

void printCGate(std::map<std::string, std::string> &record)
{
    //phone, institution code, ref, audit ref,
    const char* keys[] = {USSD_PHONE, USSD_INSTITUTION_CODE, USSD_REF, USSD_AUDIT_REF};
    const char* labels[] = {"MOBILE", "INSTITUTION CODE", "TRANS. REF","AUDIT REF"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}
static void printAsteric(size_t len)
{
    char line[32] = {'\0'};

    memset(line, '*', len + 4);
    UPrint_StrBold(line, 1, 4, 1);
}

int generateReceipt(std::map<std::string, std::string> &record, const USSDService service)
{
    MerchantData mParam = {'\0'};
    int ret = 0;
    char buff[32] = {'\0'};
    char logoFileName[64] = {'\0'};
    
    std::map<std::string, std::string>::iterator itr;

    for(itr = record.begin(); itr != record.end(); ++itr) {
        printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
    }

    readMerchantData(&mParam);

    while(1) {
        ret = UPrint_Init();

        if (ret == UPRN_OUTOF_PAPER) {
            if (UI_ShowButtonMessage(0, "Print", "No paper", "confirm", UI_DIALOG_TYPE_CONFIRMATION)) {
                break;
            }
        }

      
        sprintf(logoFileName, "xxxx\\%s", BANKLOGO);

        printReceiptLogo(logoFileName);
        printReceiptHeader(record[USSD_DATE].c_str());

        strcpy(buff, record[USSD_SERVICE].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record["receipt_copy"].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        UPrint_SetDensity(3); //Set print density to 3 normal
        UPrint_SetFont(7, 2, 2);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record[USSD_STATUS].c_str());
        UPrint_StrBold(buff, 1, 4, 1);
        UPrint_Feed(12);

        if(mParam.tid[0]) {
            printLine("TID", mParam.tid);
        }

        if (service == CGATE_PURCHASE || service == CGATE_CASHOUT) {
            printCGate(record);
            printLine("PAYMENT METHOD", "CGATE");

        } else if(service == PAYATTITUDE_PURCHASE) {
            printPayAttitude(record);
            printLine("PAYMENT METHOD", "PAYATTITUDE");
        } else if(service == MCASH_PURCHASE) {
            printMcash(record);
            printLine("PAYMENT METHOD", "MCASH");
        }

        memset(buff, '\0', sizeof(buff));
        sprintf(buff, "NGN %s", record[USSD_AMOUNT].c_str());

        printAsteric(strlen(buff));
        UPrint_StrBold(buff, 1, 4, 1);
        printAsteric(strlen(buff));

        UPrint_Feed(12);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record[USSD_STATUS_MESSAGE].c_str());
        if(buff[0]) {
            UPrint_StrBold(buff, 1, 4, 1);
        }
        
        printDottedLine();
        printFooter();

        ret = UPrint_Start();
        if(getPrinterStatus(ret) < 0 ) {
            break;
        }
    }
    

    return ret;
}

int printUSSDEod(std::map<std::string, std::string> &records)
{
    int ret = 0;
    char buff[64] = {'\0'};
    char filename[32] = {'\0'};

  
    iisys::JSObject json;

    if(!json.load(records["menu"]) || !json.isArray()) {
        return -1;
    }

    while(1) {

        ret = UPrint_Init();
        if (ret == UPRN_OUTOF_PAPER) {
            if(UI_ShowButtonMessage(0, "Print", "No paper \nDo you wish to reload Paper?r", "confirm", UI_DIALOG_TYPE_CONFIRMATION)) {
                break;
            }
        }

        sprintf(filename, "xxxx\\%s", BANKLOGO);
        printReceiptLogo(filename);

        printReceiptHeader(records[USSD_DATE].c_str());

        UPrint_SetFont(8, 2, 2);
        UPrint_StrBold("SUMMARY", 1, 4, 1);
        printDottedLine();

        UPrint_Feed(12);

        printLine("Approved Amnt", records["approvedAmount"].c_str());
        printLine("Approved Count", records["approvedCount"].c_str());
        printLine("Declined Amnt", records["declinedAmount"].c_str());
        printLine("Declined Count", records["declinedCount"].c_str());
        printLine("Total Count", records["totalCount"].c_str());

        UPrint_Feed(12);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, records["trxType"].c_str());
        UPrint_StrBold(buff, 1, 4, 1);
        printDottedLine();

        UPrint_Feed(10);

        for(int i = 0; i < json.size(); i++) {

            char leftAlign[32] = {'\0'};
            char rightAlign[32] = {'\0'};
            char amount[14] = {'\0'};
            char time[8] = {'\0'};
            char status[3] = {'\0'};
            char preferredField[24] = {'\0'};
            
            iisys::JSObject& itex = json[i];

            strcpy(amount, itex("amount").getString().c_str());
            strcpy(time, itex("tstamp").getString().c_str());
            strcpy(preferredField, itex("preferredField").getString().c_str());
            strcpy(status, itex("status").getString().c_str());

            sprintf(leftAlign, "%s %s", time, preferredField);
            sprintf(rightAlign, "%s %s", amount, status);
            printLine(leftAlign, rightAlign);

            printDottedLine();

        }

        printFooter();

        ret = UPrint_Start();
        if(getPrinterStatus(ret) < 0) {
            break;
        }

    }
    
    return ret;

}

int printUSSDReceipt(std::map<std::string, std::string>& record)
{
    int printStatus = -1;


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
        
        if (record[USSD_SERVICE] == ussdServiceToString(CGATE_PURCHASE)) {
            printStatus = generateReceipt(record, CGATE_PURCHASE);
        } else if (record[USSD_SERVICE] == ussdServiceToString(CGATE_CASHOUT)) {
            printStatus = generateReceipt(record, CGATE_CASHOUT);
        }else if (record[USSD_SERVICE] == ussdServiceToString(MCASH_PURCHASE)) {
            printStatus = generateReceipt(record, MCASH_PURCHASE);
        } else if (record[USSD_SERVICE] == ussdServiceToString(PAYATTITUDE_PURCHASE)) {
            printStatus = generateReceipt(record, PAYATTITUDE_PURCHASE);
        } 
    }

    std::map<std::string, std::string>::iterator itr;
    for(itr = record.begin(); itr != record.end(); ++itr) {
        printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
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
    
    record["receipt_copy"] += "- Reprint -";

    printUSSDReceipt(record);

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
    // using namespace vfigui;

    // fillReceiptHeader(values);
    // fillReceiptFooter(values);
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

        printUSSDEod(values);

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

