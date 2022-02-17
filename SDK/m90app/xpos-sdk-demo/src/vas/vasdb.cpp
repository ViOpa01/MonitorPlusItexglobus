
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include "vasdb.h"

#define VASDBLOG                    "VASDB"
#define VASDB_TABLE                 "vas"
#define VASDB_TABLE_ROW_LIMIT       "14400"  

#define LEFT_JOIN_CARD " LEFT JOIN " VASDB_CARD_TABLE" ON " VASDB_TABLE"." VASDB_CARD_ID " = " VASDB_CARD_TABLE "." VASDB_CARD_TABLE_ID

VasDB::VasDB()
{
    int rc;

    rc = sqlite3_open_v2(VASDB_FILE, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        sqlite3_close_v2(db);
        db = 0;
        return;
    }
}

 VasDB::~VasDB()
{
    sqlite3_close_v2(db);
}

int VasDB::prepareMapInsert(std::string& query, std::map<std::string, std::string>& record)
{
    using namespace std;
    string insert;
    string params;
    unsigned int i = 0;
    const unsigned int size = record.size();

    if (!size) return -1;

    for (map<string, string>::const_iterator element = record.begin(); element != record.end(); ++element, ++i) {
        insert += element->first;
        params += std::string("@") + element->first;
        if (i < size - 1) {
            insert += ", ";
            params += ", ";
        }
    }

    query = std::string(VASDB_TABLE) + " (" + insert + ")\nVALUES (" + params + ")";

    return 0;
}

const char * VasDB::trxStatusString(TrxStatus status)
{
    switch (status) {
    case APPROVED:
        return "Approved";
    case DECLINED:
        return "Declined";
    case PENDING:
        return "Pending";
    case CARDAPPROVED:
        return "Card Approved";
    case STATUS_UNKNOWN:
        return "Connection Timeout";
    default:
        return "";
    }
}

VasDB::TrxStatus VasDB::vasErrToTrxStatus(VasError error)
{
    switch (error) {
    case NO_ERRORS:
        return VasDB::APPROVED;
    case VAS_DECLINED:
    case CARD_PAYMENT_DECLINED:
        return VasDB::DECLINED;
    case CARD_APPROVED:
        return VasDB::CARDAPPROVED;
    case TXN_PENDING:
        return VasDB::PENDING;
    case CARD_STATUS_UNKNOWN:
    case CASH_STATUS_UNKNOWN:
    case VAS_STATUS_UNKNOWN:
    case INVALID_JSON:
    case TYPE_MISMATCH:
    case KEY_NOT_FOUND:
    case STATUS_ERROR:
    case EMPTY_VAS_RESPONSE:
    //
    case VAS_ERROR:
    case LOOKUP_ERROR:
    case USER_CANCELLATION:
    case INPUT_ABORT:
    case INPUT_TIMEOUT_ERROR:
    case INPUT_ERROR:
    case TXN_NOT_FOUND:
    case NOT_LOGGED_IN:
    case TOKEN_UNAVAILABLE:
    default:
        return VasDB::STATUS_UNKNOWN;
    }
}

int VasDB::select(std::map<std::string, std::string>& record, unsigned long atIndex)
{
    char pIndex[64] = { 0 };
    snprintf(pIndex, sizeof(pIndex), "%lu", atIndex);

    return select(record, pIndex);
}

int VasDB::select(std::map<std::string, std::string>& record, const std::string& atIndex)
{
    sqlite3_stmt* stmt;
    std::string sql;
    int ret = -1;
    int rc;
    int columns;

    sql = "SELECT * FROM " VASDB_TABLE LEFT_JOIN_CARD " WHERE " VASDB_ID " = " + atIndex + ";";

    printf("-> %s\n", sql.c_str());

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return ret;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        return ret;
    }

    columns = sqlite3_column_count(stmt);

    for (int i = 0; i < columns; i++) {
        if (sqlite3_column_type(stmt, i) == SQLITE_NULL) {
            continue;
        }
        const char* name = sqlite3_column_name(stmt, i);
        const unsigned char* text = sqlite3_column_text(stmt, i);
        record[name] = reinterpret_cast<const char*>(text);
    }

    sqlite3_finalize(stmt);

    return 0;
}

long VasDB::saveVasTransaction(std::map<std::string, std::string>& record)
{
    sqlite3_stmt* res;
    int step;
    int ret = -1;
    int rc;
    sqlite3* db;

    using namespace std;
    string sql;
    string query;

    rc = sqlite3_open_v2(VASDB_FILE, &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
        return ret;
    }

    if (prepareMapInsert(query, record)) {
        return ret;
    }

    sql = string("INSERT INTO ") + query;

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, 0);

    if (rc != SQLITE_OK) {
        return ret;
    }

    for (map<string, string>::const_iterator begin = record.begin(); begin != record.end(); ++begin) {
        std::string iStr = string("@") +  begin->first;
        int idx = sqlite3_bind_parameter_index(res, iStr.c_str());
        sqlite3_bind_text(res, idx, begin->second.c_str(), -1, SQLITE_STATIC);
    }

    step = sqlite3_step(res);
    if (step != SQLITE_DONE) {
        sqlite3_finalize(res);
        sqlite3_close_v2(db);
        return ret;
    }

    sqlite3_finalize(res);

    long rowid = (long)sqlite3_last_insert_rowid(db);
    sqlite3_close_v2(db);

    return rowid;
}

int VasDB::selectUniqueServices(std::vector<std::string>& services, std::string date)
{
    int ret = -1;
    sqlite3_stmt *stmt;
    int rc;
    std::string selectQuery;
    
    if (date.empty()) {
        selectQuery = "SELECT DISTINCT " VASDB_SERVICE " FROM " VASDB_TABLE " ORDER BY " VASDB_DATE " DESC";
    } else {
        selectQuery = "SELECT DISTINCT " VASDB_SERVICE " FROM " VASDB_TABLE " WHERE strftime('%Y-%m-%d', " VASDB_DATE ") = '" + date.substr(0, 10) + "' ORDER BY " VASDB_DATE " DESC";
    }

    // LOGF_INFO(log.handle, "%s -> %s\n", __FUNCTION__, selectQuery.c_str());

    rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmt, NULL); 
    if (rc != SQLITE_OK) {
        return -1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        const unsigned char *text;
        text = sqlite3_column_text (stmt, 0);
        services.push_back(reinterpret_cast<const char*>(text));
        ret = 0;
    }

    sqlite3_finalize(stmt);
    return ret;
}

int VasDB::selectUniqueDates(std::vector<std::string>& dates, std::string service)
{
    int ret = -1;
    sqlite3_stmt *stmt;
    int rc;
    std::string selectQuery;
    
    if (service.empty()) {
        selectQuery = "SELECT DISTINCT strftime('%Y-%m-%d', " VASDB_DATE ") FROM " VASDB_TABLE " ORDER BY " VASDB_DATE " DESC";
    } else {
        selectQuery = "SELECT DISTINCT strftime('%Y-%m-%d', " VASDB_DATE ") FROM " VASDB_TABLE " WHERE " VASDB_SERVICE " = " + service + " ORDER BY " VASDB_DATE " DESC";
    }

    rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmt, NULL); 
    if (rc != SQLITE_OK) {
        return -1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        const unsigned char *text;
        text = sqlite3_column_text (stmt, 0);
        dates.push_back(reinterpret_cast<const char*>(text));
        ret = 0;
    }

    sqlite3_finalize(stmt);
    return ret;
}

int VasDB::selectTransactions(std::vector<std::map<std::string, std::string> >& transactions, TrxStatus status, const std::string& service)
{
    return 0;
}


int VasDB::selectTransactionsOnDate(std::vector<std::map<std::string, std::string> >& transactions, const char *date, const std::string& service)
{
    char sql[256] = {0};
    sqlite3_stmt *stmt;
    int rc;
    char dateTrim[16] = {0};

    if (service.empty()) {
        sprintf(sql, "SELECT * FROM " VASDB_TABLE LEFT_JOIN_CARD " WHERE strftime('%%Y-%%m-%%d', " VASDB_TABLE"." VASDB_DATE ") = '%s' ORDER BY " VASDB_TABLE"." VASDB_DATE " ASC"
        , strncpy(dateTrim, date, 10));
    } else {
        sprintf(sql, "SELECT * FROM " VASDB_TABLE LEFT_JOIN_CARD " WHERE strftime('%%Y-%%m-%%d', " VASDB_TABLE"." VASDB_DATE ") = '%s' and " VASDB_TABLE"." VASDB_SERVICE " = '%s' ORDER BY " VASDB_TABLE"." VASDB_DATE " ASC"
        , strncpy(dateTrim, date, 10), service.c_str());
    }

    // LOGF_INFO(log.handle, "%s\n", sql);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    const int columns = sqlite3_column_count(stmt);
    for (int row = 0; sqlite3_step(stmt) == SQLITE_ROW; row++) {
        transactions.push_back(std::map<std::string, std::string>());
        for (int i = 0; i < columns; i++) {
            if (sqlite3_column_type(stmt, i) == SQLITE_NULL)
                continue;
            const char * name = sqlite3_column_name(stmt, i);
            const unsigned char *text = sqlite3_column_text (stmt, i);
            transactions[row][name] = reinterpret_cast<const char *>(text);
        }
    }

    sqlite3_finalize(stmt);
    return 0;
}

int VasDB::selectTransactionsByRef(std::vector<std::map<std::string, std::string> >& transactions, const char* ref, const std::string& service)
{
    return 0;
}

int VasDB::sumTransactionsInDateRange(std::string& amount, const char *minDate, const char *maxDate, TrxStatus status, const std::string& service)
{
    char sql[256] = {0};
    sqlite3_stmt *stmt;
    int rc;
    char minDateTrim[16] = {0};
    char maxDateTrim[16] = {0};

    if (status == ALL) {
        if (service.empty()) {
            sprintf(sql, "SELECT sum(" VASDB_AMOUNT ") FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10));
        } else {
            sprintf(sql, "SELECT sum(" VASDB_AMOUNT ") FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s' and " VASDB_SERVICE " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str());
        }
    } else {
        if (service.empty()) {
            sprintf(sql, "SELECT sum(" VASDB_AMOUNT ") FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s' and " VASDB_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), trxStatusString(status));
        } else {
            sprintf(sql, "SELECT sum(" VASDB_AMOUNT ") FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s' and " VASDB_SERVICE " = '%s' and " VASDB_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str(), trxStatusString(status));
        }
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    // LOGF_INFO(log.handle, "%s - Prepared: %s\n", __FUNCTION__, sql);

    rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            amount = "0";   
        } else {
            amount = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        rc = 0;
    }

    sqlite3_finalize(stmt);
    return rc;
}

int VasDB::countTransactionsInDateRange(std::string& count, const char *minDate, const char *maxDate, TrxStatus status, const std::string& service)
{
    char sql[256] = {0};
    sqlite3_stmt *stmt;
    int rc;
    char minDateTrim[16] = {0};
    char maxDateTrim[16] = {0};

    if (status == ALL) {
        if (service.empty()) {
            sprintf(sql, "SELECT count(*) FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10));
        } else {
            sprintf(sql, "SELECT count(*) FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s' and " VASDB_SERVICE " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str());
        }
    } else {
        if (service.empty()) {
            sprintf(sql, "SELECT count(*) FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s' and " VASDB_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), trxStatusString(status));
        } else {
            sprintf(sql, "SELECT count(*) FROM " VASDB_TABLE " WHERE strftime('%%Y-%%m-%%d', " VASDB_DATE ") BETWEEN '%s' and '%s' and " VASDB_SERVICE " = '%s' and " VASDB_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str(), trxStatusString(status));
        }
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    // LOGF_INFO(log.handle, "%s - Prepared: %s\n", __FUNCTION__, sql);

    rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            count = "0";   
        } else {
            count = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        rc = 0;
    }

    sqlite3_finalize(stmt);
    return rc;
}

int VasDB::sumTransactionsOnDate(std::string& amount, const char *date, TrxStatus status, const std::string& service)
{
    return sumTransactionsInDateRange(amount, date, date, status, service);
}

int VasDB::countTransactionsOnDate(std::string& count, const char *date, TrxStatus status, const std::string& service)
{
    return countTransactionsInDateRange(count, date, date, status, service);
}

int  VasDB::countAllTransactions()
{
    char sql[] = "SELECT count(*) from "  VASDB_TABLE;
    sqlite3_stmt *stmt;
    sqlite3* db;
    int rc;

    rc = sqlite3_open_v2(VASDB_FILE, &db, SQLITE_OPEN_READONLY, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close_v2(db);
        return -1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close_v2(db);
        return -1;
    } else if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            rc = 0;
        } else {
            rc = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close_v2(db);

    return rc;
}

int  VasDB::init()
{
    char* errMsg;
    const char* sql;
    int rc;
    sqlite3* db;

    rc = sqlite3_open_v2(VASDB_FILE, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        sqlite3_close_v2(db);
        return -1;
    }

    sql = "PRAGMA foreign_keys = ON; "
          "CREATE TABLE IF NOT EXISTS " VASDB_TABLE" ("
          VASDB_ID                " INTEGER PRIMARY KEY AUTOINCREMENT, "
          VASDB_SERVICE           " TEXT NOT NULL, "
          VASDB_AMOUNT            " TEXT NOT NULL, "
          VASDB_DATE              " TEXT NOT NULL, "
          VASDB_REF               " TEXT, "
          VASDB_BENEFICIARY       " TEXT, "
          VASDB_BENEFICIARY_NAME  " TEXT, "
          VASDB_BENEFICIARY_ADDR  " TEXT, "
          VASDB_BENEFICIARY_PHONE " TEXT, "
          VASDB_PRODUCT           " TEXT, "
          VASDB_CATEGORY          " TEXT, "
          VASDB_FEE               " TEXT, "
          VASDB_VIRTUAL_TID       " TEXT, "
          VASDB_TRANS_SEQ         " TEXT, "
          VASDB_PAYMENT_METHOD    " TEXT, "
          VASDB_STATUS            " TEXT, "
          VASDB_STATUS_MESSAGE    " TEXT, "
          VASDB_CARD_ID           " INTEGER REFERENCES " VASDB_CARD_TABLE "(" VASDB_CARD_TABLE_ID ") ON UPDATE CASCADE ON DELETE CASCADE, "
          VASDB_SERVICE_DATA      " TEXT  "
          "); "
          "VACUUM;";

    rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errMsg);
        sqlite3_close_v2(db);
        return -1;
    }

    sqlite3_close_v2(db);
    return 0;
}



