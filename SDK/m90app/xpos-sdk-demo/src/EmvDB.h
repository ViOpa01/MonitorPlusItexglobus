
#ifndef SRC_DATABASE_H
#define SRC_DATABASE_H

#include <stdlib.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <ctype.h>

//

#define DB_ID "id"
#define DB_MTI "mti"
#define DB_PS "ps"
#define DB_RRN "rrn"
#define DB_NAME "name"
#define DB_AID "aid"
#define DB_PAN "pan"
#define DB_LABEL "label"
#define DB_TEC "tecMode"
#define DB_AMOUNT "amount"
#define DB_ADDITIONAL_AMOUNT "additionalAmount"
#define DB_AUTHID "authId"
#define DB_EXPDATE "expDate"
#define DB_DATE "date"
#define DB_RESP "resp"
#define DB_TVR "tvr"
#define DB_TSI "tsi"
#define DB_FISC "fisc"
#define DB_STAN "stan"

#define DBFILE "itex/emvdb.db"


typedef enum
    {
        ALL,
        APPROVED,
        DECLINED
    } TrxRespCode;

#ifndef TXTYPE
    typedef enum
    {
        ALL_TRX_TYPES = 999,
        REVERSAL = 0420,
        PURCHASE = 0,
        PURCHASE_WC = 9,
        PRE_AUTH = 60,
        PRE_AUTH_C = 61,
        CASH_ADV = 1,
        REFUND = 20,
        DEPOSIT = 21
    } TrxType;
#define TXTYPE
#endif
struct EmvDB
{
    

    explicit EmvDB(const std::string &tableName, const std::string &file = DBFILE);
    ~EmvDB();
    int select(std::map<std::string, std::string> &record, unsigned long atIndex);
    int selectUniqueDates(std::vector<std::string> &dates, TrxType trxType);
    int selectTransactions(std::vector<std::map<std::string, std::string> > &transactions, TrxRespCode status, TrxType trxType);
    int selectTransactionsOnDate(std::vector<std::map<std::string, std::string> > &transactions, const char *date, TrxType trxType);
    int selectTransactionsByRef(std::vector<std::map<std::string, std::string> > &transactions, const char *ref, TrxType trxType);

    int sumTransactionsInDateRange(std::string &amount, const char *minDate, const char *maxDate, TrxRespCode status, TrxType trxType);
    int countTransactionsInDateRange(std::string &count, const char *minDate, const char *maxDate, TrxRespCode status, TrxType trxType);

    int sumTransactionsOnDate(std::string &amount, const char *date, TrxRespCode status, TrxType trxType);
    int countTransactionsOnDate(std::string &count, const char *date, TrxRespCode status, TrxType trxType);

    unsigned long sumTransactions(TrxRespCode status, TrxType trxType);
    int countTransactions(TrxRespCode status, TrxType trxType);

    long lastTransactionId();
    long insertTransaction(const std::map<std::string, std::string> &record);
    int updateTransaction(long atPrimaryIndex, const std::map<std::string, std::string> &record);

    int clear(); // Use sparingly
    int countAllTransactions();
    int lastTransactionTime(std::string &time);
    int getStan(char *stanString, size_t size);
    static const char *trxTypeToPS(TrxType type);

    int getUserVersion(sqlite3 *db);
    int tableExists(sqlite3 *db, const char *tableName);

private:
    sqlite3 *db;
    const std::string table;
    const std::string dbFile;
    std::map<std::string, std::string> record;
    
    int prepareMap(std::string &columns, std::string &params, const std::map<std::string, std::string> &record);
    std::string updateQuery(const std::map<std::string, std::string> &record);
    int init(const std::string &tableName);
};

#endif
