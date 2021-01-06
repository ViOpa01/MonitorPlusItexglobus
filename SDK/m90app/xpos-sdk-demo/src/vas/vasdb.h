
#ifndef VASDB_DATABASE_H
#define VASDB_DATABASE_H

#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>

#include "vas.h"

#define VASDB_FILE                 "itex/vas.db"
#define VASDB_CARD_TABLE           "vascard"
#define VASDB_CARD_TABLE_ID        "id"

#define VASDB_ID                   "vas_id"
#define VASDB_SERVICE              "vas_service"
#define VASDB_AMOUNT               "vas_amount"
#define VASDB_DATE                 "vas_date"
#define VASDB_REF                  "vas_reference"
#define VASDB_BENEFICIARY          "vas_beneficiary"
#define VASDB_BENEFICIARY_NAME     "vas_beneficiaryName"
#define VASDB_BENEFICIARY_ADDR     "vas_beneficiaryAddr"
#define VASDB_BENEFICIARY_PHONE    "vas_beneficiaryPhone"
#define VASDB_PRODUCT              "vas_product"
#define VASDB_CATEGORY             "vas_category"
#define VASDB_FEE                  "vas_transactionFee"
#define VASDB_VIRTUAL_TID          "vas_virtualTid"
#define VASDB_TRANS_SEQ            "vas_transactionSeq"
#define VASDB_PAYMENT_METHOD       "vas_paymentMethod"
#define VASDB_STATUS               "vas_paymentStatus"
#define VASDB_CARD_ID              "vas_cardId"
#define VASDB_STATUS_MESSAGE       "vas_message"
#define VASDB_SERVICE_DATA         "vas_serviceData"

struct VasDB {

    typedef enum {
        STATUS_UNKNOWN,
        APPROVED,
        DECLINED,
        PENDING,
        CARDAPPROVED,
        ALL
    } TrxStatus;

    VasDB();
    ~VasDB();

    int select(std::map<std::string, std::string>& record, unsigned long atIndex);
    int select(std::map<std::string, std::string>& record, const std::string& atIndex);
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
    static TrxStatus vasErrToTrxStatus(VasError error);

    static long saveVasTransaction(std::map<std::string, std::string>& record);
    static int init();    // Use only during startup
    static int countAllTransactions();
    

private:
    sqlite3* db;
    std::map<std::string, std::string> record;

    static int prepareMapInsert(std::string& query, std::map<std::string, std::string>& record);
};


#endif
