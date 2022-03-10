
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include "ussdb.h"


#define USSDBLOG                   "USSDB"
#define USSD_TABLE                 "ussd"
#define USSD_TABLE_ROW_LIMIT       "14400"  

USSDB::USSDB() : db(NULL)
{
    int rc;

    rc = sqlite3_open_v2(USSD_FILE, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        printf("%s -> Can't open database: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        db = 0;
        return;
    }
}

USSDB::~USSDB()
{
    sqlite3_close_v2(db);
}

int USSDB::prepareMapInsert(std::string& query, std::map<std::string, std::string>& record)
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

    query = std::string(USSD_TABLE) + " (" + insert + ")\nVALUES (" + params + ")";

    return 0;
}

const char * USSDB::trxStatusString(TrxStatus status)
{
    switch (status) {
    case APPROVED:
        return "Approved";
    case DECLINED:
        return "Declined";
    case STATUS_UNKNOWN:
        return "Connection Timeout";
    default:
        return "";
    }
}

int USSDB::select(std::map<std::string, std::string>& record, unsigned long atIndex)
{
    char pIndex[64] = { 0 };
    snprintf(pIndex, sizeof(pIndex), "%lu", atIndex);

    return select(record, pIndex);
}

int USSDB::select(std::map<std::string, std::string>& record, const std::string& atIndex)
{
    sqlite3_stmt* stmt;
    std::string sql;
    int ret = -1;
    int rc;
    int columns;

    sql = "SELECT * FROM " USSD_TABLE " WHERE " USSD_ID " = " + atIndex + ";";

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

long USSDB::saveUssdTransaction(std::map<std::string, std::string>& record)
{
    sqlite3_stmt* res;
    int step;
    int ret = -1;
    int rc;
    sqlite3* db;

    using namespace std;
    string sql;
    string query;

    rc = sqlite3_open_v2(USSD_FILE, &db, SQLITE_OPEN_READWRITE, NULL);
    if (rc != SQLITE_OK) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        return ret;
    }

    if (prepareMapInsert(query, record)) {
        printf("Failed To Create Query String");
        return ret;
    }

    sql = string("INSERT INTO ") + query;

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, 0);

    if (rc != SQLITE_OK) {
        printf("Can't prepare statement: %s\n", sqlite3_errmsg(db));
        return ret;
    }

    for (map<string, string>::const_iterator begin = record.begin(); begin != record.end(); ++begin) {
        std::string iStr = string("@") +  begin->first;
        int idx = sqlite3_bind_parameter_index(res, iStr.c_str());
        sqlite3_bind_text(res, idx, begin->second.c_str(), -1, SQLITE_STATIC);
    }

    step = sqlite3_step(res);
    if (step != SQLITE_DONE) {
        printf("Step statement failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(res);
        sqlite3_close_v2(db);
        return ret;
    }

    sqlite3_finalize(res);

    long rowid = (long)sqlite3_last_insert_rowid(db);
    sqlite3_close_v2(db);

    return rowid;
}

std::string USSDB::updateQuery(const std::map<std::string, std::string>& record)
{
    using namespace std;
    size_t i = 0;
    const size_t size = record.size();
    ostringstream ostream;

    if (!size) return "";

    ostream << "UPDATE " USSD_TABLE " SET ";
    for (map<string, string>::const_iterator element = record.begin(); element != record.end(); ++element, ++i) {
        ostream << element->first << " = @" << element->first << ((i < size - 1) ? ", " : " WHERE " USSD_ID " = @" USSD_ID "; ");
    }

    return ostream.str();
}

int USSDB::updateUssdTransaction(std::map<std::string, std::string>& record, long atPrimaryIndex)
{
    sqlite3_stmt* res;
    int step;
    int ret = -1;
    int idx;

    std::string sql;
    int rc;


    if (atPrimaryIndex == 0) {
        printf("%s: Invalid primary index -> %ld\n", __FUNCTION__, atPrimaryIndex);
        return -1;
    }
    
    record.erase(USSD_ID);
    
    sql = updateQuery(record);
    printf("%s", sql.c_str());

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, 0);

    if (rc != SQLITE_OK) {
        printf("%s: Can't prepare statement: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        return ret;
    }

    using namespace std;
    for (map<string, string>::const_iterator element = record.begin(); element != record.end(); ++element) {
        idx = sqlite3_bind_parameter_index(res, string("@" + element->first).c_str());
        sqlite3_bind_text(res, idx, element->second.c_str(), -1, SQLITE_STATIC);
    }

    idx = sqlite3_bind_parameter_index(res, "@" USSD_ID);
    sqlite3_bind_int(res, idx, atPrimaryIndex);

    step = sqlite3_step(res);
    if (step != SQLITE_DONE) {
        printf("Step statement failed: %s\n", sqlite3_errmsg(db));
    } else {
        ret = 0;
    }

    sqlite3_finalize(res);

    return ret;
}

int USSDB::selectUniqueServices(std::vector<std::string>& services, std::string date)
{
    int ret = -1;
    sqlite3_stmt *stmt;
    int rc;
    std::string selectQuery;
    
    if (date.empty()) {
        selectQuery = "SELECT DISTINCT " USSD_SERVICE " FROM " USSD_TABLE " ORDER BY " USSD_DATE " DESC";
    } else {
        selectQuery = "SELECT DISTINCT " USSD_SERVICE " FROM " USSD_TABLE " WHERE strftime('%Y-%m-%d', " USSD_DATE ") = '" + date.substr(0, 10) + "' ORDER BY " USSD_DATE " DESC";
    }


    rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmt, NULL); 
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed (%s): %s\n", __FUNCTION__, selectQuery.c_str(), sqlite3_errmsg(db));
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

int USSDB::selectUniqueDates(std::vector<std::string>& dates, std::string service)
{
    int ret = -1;
    sqlite3_stmt *stmt;
    int rc;
    std::string selectQuery;
    
    if (service.empty()) {
        selectQuery = "SELECT DISTINCT strftime('%Y-%m-%d', " USSD_DATE ") FROM " USSD_TABLE " ORDER BY " USSD_DATE " DESC";
    } else {
        selectQuery = "SELECT DISTINCT strftime('%Y-%m-%d', " USSD_DATE ") FROM " USSD_TABLE " WHERE " USSD_SERVICE " = \"" + service + "\" ORDER BY " USSD_DATE " DESC";
    }

    rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmt, NULL); 
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed (%s): %s\n", __FUNCTION__, selectQuery.c_str(), sqlite3_errmsg(db));
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

int USSDB::selectTransactions(std::vector<std::map<std::string, std::string> >& transactions, TrxStatus status, const std::string& service)
{
    return 0;
}


int USSDB::selectTransactionsOnDate(std::vector<std::map<std::string, std::string> >& transactions, const char *date, const std::string& service)
{
    char sql[256] = {0};
    sqlite3_stmt *stmt;
    int rc;
    char dateTrim[16] = {0};

    if (service.empty()) {
        sprintf(sql, "SELECT * FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") = '%s' ORDER BY " USSD_DATE " ASC"
        , strncpy(dateTrim, date, 10));
    } else {
        sprintf(sql, "SELECT * FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") = '%s' and " USSD_SERVICE " = \"%s\" ORDER BY " USSD_DATE " ASC"
        , strncpy(dateTrim, date, 10), service.c_str());
    }

    printf("%s\n", sql);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));
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

int USSDB::selectTransactionsByRef(std::vector<std::map<std::string, std::string> >& transactions, const char* ref, const std::string& service)
{
    std::string sql;
    sqlite3_stmt* stmt;
    int rc;


    if (service.empty()) {
        sql = "SELECT * FROM " USSD_TABLE " WHERE " USSD_REF " ='" + std::string(ref) +"';";
    } else {
        sql = "SELECT * FROM " USSD_TABLE " WHERE " USSD_REF " = '" + std::string(ref) +"' and " USSD_SERVICE " = \"" + service + "\"; ";// , ref, static_cast<int>(trxType));
    }

    printf("%s -> %s\n", __FUNCTION__, sql.c_str());

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        return -1;
    }

    const int columns = sqlite3_column_count(stmt);
    for (int row = 0; sqlite3_step(stmt) == SQLITE_ROW; row++) {
        transactions.push_back(std::map<std::string, std::string>());
        for (int i = 0; i < columns; i++) {
            if (sqlite3_column_type(stmt, i) == SQLITE_NULL)
                continue;
            const char* name = sqlite3_column_name(stmt, i);
            const unsigned char* text = sqlite3_column_text(stmt, i);
            transactions[row][name] = reinterpret_cast<const char*>(text);
        }
    }

    sqlite3_finalize(stmt);
    return 0;
}

int USSDB::sumTransactionsInDateRange(std::string& amount, const char *minDate, const char *maxDate, TrxStatus status, const std::string& service)
{
    char sql[256] = {0};
    sqlite3_stmt *stmt;
    int rc;
    char minDateTrim[16] = {0};
    char maxDateTrim[16] = {0};

    if (status == ALL) {
        if (service.empty()) {
            sprintf(sql, "SELECT sum(" USSD_AMOUNT ") FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10));
        } else {
            sprintf(sql, "SELECT sum(" USSD_AMOUNT ") FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s' and " USSD_SERVICE " = \"%s\""
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str());
        }
    } else {
        if (service.empty()) {
            sprintf(sql, "SELECT sum(" USSD_AMOUNT ") FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s' and " USSD_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), trxStatusString(status));
        } else {
            sprintf(sql, "SELECT sum(" USSD_AMOUNT ") FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s' and " USSD_SERVICE " = \"%s\" and " USSD_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str(), trxStatusString(status));
        }
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        return -1;
    }

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

int USSDB::countTransactionsInDateRange(std::string& count, const char *minDate, const char *maxDate, TrxStatus status, const std::string& service)
{
    char sql[256] = {0};
    sqlite3_stmt *stmt;
    int rc;
    char minDateTrim[16] = {0};
    char maxDateTrim[16] = {0};

    if (status == ALL) {
        if (service.empty()) {
            sprintf(sql, "SELECT count(*) FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10));
        } else {
            sprintf(sql, "SELECT count(*) FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s' and " USSD_SERVICE " = \"%s\""
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str());
        }
    } else {
        if (service.empty()) {
            sprintf(sql, "SELECT count(*) FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s' and " USSD_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), trxStatusString(status));
        } else {
            sprintf(sql, "SELECT count(*) FROM " USSD_TABLE " WHERE strftime('%%Y-%%m-%%d', " USSD_DATE ") BETWEEN '%s' and '%s' and " USSD_SERVICE " = \"%s\" and " USSD_STATUS " = '%s'"
            , strncpy(minDateTrim, minDate, 10), strncpy(maxDateTrim, maxDate, 10), service.c_str(), trxStatusString(status));
        }
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        return -1;
    }

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

int USSDB::sumTransactionsOnDate(std::string& amount, const char *date, TrxStatus status, const std::string& service)
{
    return sumTransactionsInDateRange(amount, date, date, status, service);
}

int USSDB::countTransactionsOnDate(std::string& count, const char *date, TrxStatus status, const std::string& service)
{
    return countTransactionsInDateRange(count, date, date, status, service);
}

// unsigned long  USSDB::sumTransactions(TrxStatus status, std::string service)
// {
//     std::string amount;
//     std::vector<std::string> dates;

//     if (selectUniqueDates(dates, service) != 0) {
//         return 0;
//     }

//     // selectUniqueDates is currently in descending mode, but if that fails try ascending mode
//     if (sumTransactionsInDateRange(amount, dates.rbegin()->c_str(), dates.begin()->c_str(), status, service) 
//     && sumTransactionsInDateRange(amount, dates.begin()->c_str(), dates.rbegin()->c_str(), status, service)) {
//         return 0;
//     }

//     // I assume strtoul will never fail because sqlite select sum() will not sum non-numeric values
//     return strtoul(amount.c_str(), NULL, 10);
// }

// int  USSDB::countTransactions(TrxStatus status, Service service)
// {
//     std::string count;
//     std::vector<std::string> dates;

//     if (selectUniqueDates(dates, service) != 0) {
//         return 0;
//     }

//     if (countTransactionsInDateRange(count, dates.rbegin()->c_str(), dates.begin()->c_str(), status, service) 
//     && countTransactionsInDateRange(count, dates.begin()->c_str(), dates.rbegin()->c_str(), status, service)) {
//         return 0;
//     }

//     return strtoul(count.c_str(), NULL, 10);
// }

int  USSDB::countAllTransactions()
{
    char sql[] = "SELECT count(*) from "  USSD_TABLE;
    sqlite3_stmt *stmt;
    sqlite3* db;
    int rc;


    rc = sqlite3_open_v2(USSD_FILE, &db, SQLITE_OPEN_READONLY, NULL);
    if (rc != SQLITE_OK) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return -1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));
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

int  USSDB::init()
{
    char* errMsg;
    const char* sql;
    int rc;
    sqlite3* db;

    rc = sqlite3_open_v2(USSD_FILE, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        printf("%s -> Can't open database: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return -1;
    }

    sql = "CREATE TABLE IF NOT EXISTS " USSD_TABLE" ("
          USSD_ID                " INTEGER PRIMARY KEY AUTOINCREMENT, "
          USSD_SERVICE           " TEXT NOT NULL, "
          USSD_AMOUNT            " TEXT NOT NULL, "
          USSD_DATE              " TEXT NOT NULL, "
          USSD_REF               " TEXT, "
          USSD_AUDIT_REF         " TEXT, "
          USSD_OP_ID             " TEXT, "
          USSD_CHARGE_REQ_ID     " TEXT, "
          USSD_MERCHANT_NAME     " TEXT, "
          USSD_CUSTOMER_NAME     " TEXT, "
          USSD_PHONE             " TEXT, "
          USSD_INSTITUTION_CODE  " TEXT, "
          USSD_PAYMENT_REF       " TEXT, "
          USSD_FEE               " TEXT, "
          USSD_STATUS            " TEXT, "
          USSD_STATUS_MESSAGE    " TEXT  "
          "); "
          "CREATE TRIGGER IF NOT EXISTS sanitize AFTER INSERT ON " USSD_TABLE " "
          "BEGIN "
          "DELETE FROM " USSD_TABLE " WHERE (strftime('%Y-%m-%d', " USSD_DATE ") = strftime('%Y-%m-%d', (SELECT min(" USSD_DATE ") from " USSD_TABLE ")))"
          "and (select count(*) from " USSD_TABLE ") >= " USSD_TABLE_ROW_LIMIT " and strftime('%Y-%m-%d', (select min(" USSD_DATE ") from " USSD_TABLE ")) != strftime('%Y-%m-%d', 'now');"
          "END; "
          "VACUUM;";

    rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        printf("%s -> exec failed: %s\n", __FUNCTION__, errMsg);
        sqlite3_free(errMsg);
        sqlite3_close_v2(db);
        return -1;
    }

    printf("%s -> Ok\n", __FUNCTION__);
    sqlite3_close_v2(db);
    return 0;
}



