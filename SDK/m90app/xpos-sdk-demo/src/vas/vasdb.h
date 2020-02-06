
#ifndef VASDB_DATABASE_H
#define VASDB_DATABASE_H

#include <stdlib.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>


#include "vas.h"

#define VASDB_CARD_TABLE           "vascard"
#define VASDB_CARD_TABLE_ID        "id"

#define VASDB_ID                   "vasid"
#define VASDB_SERVICE              "service"
#define VASDB_AMOUNT               "amount"
#define VASDB_DATE                 "date"
#define VASDB_REF                  "reference"
#define VASDB_BENEFICIARY          "beneficiary"
#define VASDB_BENEFICARY_NAME      "beneficiaryName"
#define VASDB_BENEFICIARY_ADDR     "beneficiaryAddr"
#define VASDB_BENEFICIARY_PHONE    "beneficiaryPhone"
#define VASDB_PRODUCT              "product"
#define VASDB_CATEGORY             "category"
#define VASDB_FEE                  "transactionFee"
#define VASDB_VIRTUAL_TID          "virtualTid"
#define VASDB_TRANS_SEQ            "transactionSeq"
#define VASDB_PAYMENT_METHOD       "paymentMethod"
#define VASDB_STATUS               "paymentStatus"
#define VASDB_CARD_ID              "cardId"
#define VASDB_STATUS_MESSAGE       "message"
#define VASDB_SERVICE_DATA         "serviceData"



struct VasDB {

    typedef enum {
        STATUS_UNKNOWN,
        APPROVED,
        DECLINED,
        ALL
    } TrxStatus;

    VasDB();
    ~VasDB();
    // int selectUniqueCategories(std::vector<std::string>& services);
    int selectUniqueDates(std::vector<std::string>& dates, std::string service = "");
    int selectUniqueServices(std::vector<std::string>& services, std::string date = "");
    int selectTransactions(std::vector<std::map<std::string, std::string> >& transactions, TrxStatus status, const std::string& service);
    int selectTransactionsOnDate(std::vector<std::map<std::string, std::string> >& transactions, const char *date, const std::string& service);
    int selectTransactionsByRef(std::vector<std::map<std::string, std::string> >& transactions, const char* ref, const std::string& service);
    
    int sumTransactionsInDateRange(std::string& amount, const char *minDate, const char *maxDate, TrxStatus status, const std::string& service);
    int countTransactionsInDateRange(std::string& count, const char *minDate, const char *maxDate, TrxStatus status, const std::string& service);

    int sumTransactionsOnDate(std::string& amount, const char *date, TrxStatus status, const std::string& service);
    int countTransactionsOnDate(std::string& count, const char *date, TrxStatus status, const std::string& service);

    // unsigned long sumTransactions(TrxStatus status, std::string service);
    // int countTransactions(TrxStatus status, Service service);

    // long beginTransaction(const EMV_TRX_CONTEXT *transaction);

    static const char * trxStatusString(TrxStatus status);
    static int saveVasTransaction(std::map<std::string, std::string>& record);
    static int init();    // Use only during startup
    static int countAllTransactions();
    

private:
    sqlite3* db;
    std::map<std::string, std::string> record;

    static int prepareMapInsert(std::string& query, std::map<std::string, std::string>& record);
};


#endif
