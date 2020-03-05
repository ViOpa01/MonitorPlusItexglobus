
#ifndef USSD_DATABASE_H
#define USSD_DATABASE_H

#include <stdlib.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>

#define USSD_FILE                 "itex/ussd.db"

#define USSD_ID                   "id"
#define USSD_SERVICE              "service"
#define USSD_AMOUNT               "amount"
#define USSD_DATE                 "date"
#define USSD_REF                  "reference"
#define USSD_AUDIT_REF            "auditReference"
#define USSD_OP_ID                "operationId"
#define USSD_CHARGE_REQ_ID        "chargeReqId"
#define USSD_MERCHANT_NAME        "merchantName"
#define USSD_PHONE                "phone"
#define USSD_INSTITUTION_CODE     "institutionCode"
#define USSD_FEE                  "transactionFee"
#define USSD_STATUS               "paymentStatus"
#define USSD_STATUS_MESSAGE       "message"


struct USSDB {

    typedef enum {
        STATUS_UNKNOWN,
        APPROVED,
        DECLINED,
        ALL
    } TrxStatus;

    USSDB();
    ~USSDB();

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

    static long saveUssdTransaction(std::map<std::string, std::string>& record);
    int updateUssdTransaction(std::map<std::string, std::string>& record, long atPrimaryIndex);
    static int init();    // Use only during startup
    static int countAllTransactions();
    

private:
    sqlite3* db;
    std::map<std::string, std::string> record;

    static int prepareMapInsert(std::string& query, std::map<std::string, std::string>& record);
    static std::string updateQuery(const std::map<std::string, std::string>& record);
};


#endif
