
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <unistd.h>

#include "EmvDB.h"



#define EMVDBLOG "EMVDB"
// #define EMV_DB "flash/testemv.db"
#define EMV_TABLE_USER_VERSION "2"
#define EMV_TABLE_ROW_LIMIT "14400"  


static std::map<std::string, int> initializationMap;

EmvDB::EmvDB(const std::string& tableName, const std::string& file) 
    : 
        table(tableName), 
        dbFile(file)
{

    if (initializationMap[tableName] == 0 && init(tableName) == 0) {
        initializationMap[tableName] = 1;
    }


    int rc = sqlite3_open_v2(dbFile.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        printf("%s -> Can't open database: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        printf("Database open failed\n");
        sqlite3_close_v2(db);
        db = 0;
        return;
    }
    printf("Database opened successfully\n");
}

EmvDB::~EmvDB()
{
    sqlite3_close_v2(db);
}

int EmvDB::prepareMap(std::string& columns, std::string& params, const std::map<std::string, std::string>& record)
{
    using namespace std;
    size_t i = 0;
    const size_t size = record.size();

    if (!size) return -1;

    for (map<string, string>::const_iterator element = record.begin(); element != record.end(); ++element, ++i) {
        columns += element->first;
        params += std::string("@") + element->first;
        if (i < size - 1) {
            columns += ", ";
            params += ", ";
        }
    }

    return 0;
}

std::string EmvDB::updateQuery(const std::map<std::string, std::string>& record)
{
    using namespace std;
    size_t i = 0;
    const size_t size = record.size();
    ostringstream ostream;

    if (!size) return "";

    ostream << "UPDATE " + table + " SET ";
    for (map<string, string>::const_iterator element = record.begin(); element != record.end(); ++element, ++i) {
        ostream << element->first << " = @" << element->first << ((i < size - 1) ? ", " : " WHERE " DB_ID " = @" DB_ID "; ");
    }

    return ostream.str();
}

long EmvDB::insertTransaction(const std::map<std::string, std::string>& record)
{
    sqlite3_stmt* res;
    int step;
    int ret = -1;
    int rc;

    using namespace std;
    string sql;
    string columns, params;

    if (prepareMap(columns, params, record)) {
        printf("Failed To Create Query String\n");
        return -1;
    }

    sql = string("INSERT INTO ") + table + " (" + columns + ") VALUES (" + params + ")";
    printf("%s\n", sql.c_str());

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
        return ret;
    }

    sqlite3_finalize(res);

    return (long)sqlite3_last_insert_rowid(db);
}

int EmvDB::updateTransaction(long atPrimaryIndex, const std::map<std::string, std::string>& record)
{
    sqlite3_stmt* res;
    int step;
    int ret = -1;
    int idx;

    std::string sql;
    char *err = 0;
    int rc;

    if (atPrimaryIndex <= 0) {
        printf("%s: Invalid primary index -> %ld\n", __FUNCTION__, atPrimaryIndex);
        return -1;
    }

    sql = updateQuery(record);
    printf("%s\n", sql.c_str());

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

    idx = sqlite3_bind_parameter_index(res, "@" DB_ID);
    sqlite3_bind_int(res, idx, atPrimaryIndex);

    step = sqlite3_step(res);
    if (step != SQLITE_DONE) {
        printf("Step statement failed: %s\n", sqlite3_errmsg(db));
    } else {
        ret = 0;
    }

    sqlite3_finalize(res);

    // TODO: To Delete
    // printf("Quick DB Check");
    // std::vector<std::map<std::string, std::string> > transactions;
    // selectTransactionsOnDate(transactions, "2018-09-18 16:44:59.000");
    // typedef std::vector<std::map<std::string, std::string> >::const_iterator vi;
    // typedef std::map<std::string, std::string>::const_iterator mi;
    // for(vi record = transactions.begin(); record != transactions.end(); ++record) 
    //     for(mi col = record->begin(); col != record->end(); ++col) 
    //         printf("(%s) -> (%s : %s)", __FUNCTION__, col->first.c_str(), col->second.c_str());
    /////////////////// Delete

    return ret;
}

int EmvDB::select(std::map<std::string, std::string>& record, unsigned long atIndex)
{
    sqlite3_stmt* stmt;
    std::string sql;
    int ret = -1;
    int rc;
    int columns;
    char pIndex[64] = { 0 };

    snprintf(pIndex, sizeof(pIndex), "%lu", atIndex);

    sql = "SELECT * FROM " + table + " WHERE id = " + std::string(pIndex) + ";";

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


int EmvDB::selectUniqueDates(std::vector<std::string>& dates, TrxType trxType)
{
    int ret = -1;
    sqlite3_stmt *stmt;
    int rc;
    std::string selectQuery;
    
    if (trxType == ALL_TRX_TYPES) {
        selectQuery = "SELECT DISTINCT strftime('%Y-%m-%d', " DB_DATE ") FROM " + table  + " ORDER BY " DB_DATE " DESC";
    } else if (trxType == REVERSAL) {
        selectQuery = "SELECT DISTINCT strftime('%Y-%m-%d', " DB_DATE ") FROM " + table + " WHERE " DB_MTI " LIKE '04%' ORDER BY " DB_DATE " DESC";
    } else {
        char txTypeStr[8] = {0};
        sprintf(txTypeStr, "'%02d%%'", static_cast<int>(trxType));
      //  snprintf(selectQuery, sizeof(selectQuery), "SELECT DISTINCT strftime('%%Y-%%m-%%d', " DB_DATE ") FROM "+ table +" WHERE " DB_PS " LIKE '%02d%%' ORDER BY " DB_DATE " DESC", static_cast<int>(trxType));
         selectQuery = "SELECT DISTINCT strftime('%Y-%m-%d', " DB_DATE ") FROM "+ table +" WHERE " DB_PS " LIKE " + std::string(txTypeStr) + " ORDER BY " DB_DATE " DESC";
    }
   printf(selectQuery.c_str());
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

int EmvDB::selectTransactions(std::vector<std::map<std::string, std::string> >& transactions, TrxRespCode status, TrxType trxType)
{
    std::string sql;
    sqlite3_stmt *stmt;
    int rc;
    // char dateTrim[16] = {0};

    if (trxType == ALL_TRX_TYPES) {
        if (status == APPROVED) {
            sql = "SELECT * FROM "+ table +" WHERE " DB_RESP " = '00' and " DB_MTI " NOT LIKE '04%' ORDER BY " DB_DATE " ASC";
        } else if (status == DECLINED) {
            sql = "SELECT * FROM "+ table + " WHERE (  " DB_RESP " IS NULL OR " DB_RESP " != '00' OR " DB_MTI " LIKE '04%' )";
        } else if (status == ALL) {
            sql = "SELECT * FROM " + table;
        }
    } else if (trxType == REVERSAL) {
        if (status == APPROVED) {
            sql = "SELECT * FROM " + table + " WHERE " DB_MTI " LIKE '04%' and " DB_RESP " = '00' ORDER BY " DB_DATE " ASC";
        } else if (status == DECLINED) {
            sql = "SELECT * FROM " + table + " WHERE " DB_MTI " LIKE '04%' and (  " DB_RESP " IS NULL OR " DB_RESP " != '00' )";
        } else if (status == ALL) {
            sql = "SELECT * FROM " + table + " WHERE " DB_MTI " LIKE '04%'";
        }
    } else {
        char txTypeStr[8] = {0};
        sprintf(txTypeStr, "'%02d%%'", static_cast<int>(trxType));
        if (status == APPROVED) {
            sql = "SELECT * FROM " + table + " WHERE " DB_PS " LIKE "+ std::string(txTypeStr) +" and " DB_RESP " = '00' and " DB_MTI " NOT LIKE '04%' ORDER BY " DB_DATE " ASC";
        } else if (status == DECLINED) {
            sql = "SELECT * FROM " + table + " WHERE " DB_PS " LIKE "+ std::string(txTypeStr) +" and (" DB_RESP " IS NULL OR " DB_RESP " != '00' OR " DB_MTI " LIKE '04%' )";
        } else if (status == ALL) {
            sql = "SELECT * FROM "+ table + " WHERE " DB_PS " LIKE " + std::string(txTypeStr);
        }
    }

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s -> prepare statement (%s) -> failed: %s\n", __FUNCTION__, sql.c_str(), sqlite3_errmsg(db));
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


int EmvDB::selectTransactionsOnDate(std::vector<std::map<std::string, std::string> >& transactions, const char *date, TrxType trxType)
{
    std::string sql;
    sqlite3_stmt *stmt;
    int rc;
    char dateTrim[16] = {0};

    strncpy(dateTrim, date, 10);
    if (trxType == ALL_TRX_TYPES) {
        sql = "SELECT * FROM "  + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") = '" + std::string(dateTrim) + "' ORDER BY " DB_DATE " ASC";
    } else if (trxType == REVERSAL) {
        sql = "SELECT * FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") = '" + std::string(dateTrim) + "' and " DB_MTI " LIKE '04%' ORDER BY " DB_DATE " ASC";
    } else {
        char txTypeStr[8] = {0};
        sprintf(txTypeStr, "'%02d%%'", static_cast<int>(trxType));
        sql = "SELECT * FROM " + table  + " WHERE strftime('%Y-%m-%d', " DB_DATE ") = '" + std::string(dateTrim) + "' and " DB_PS " LIKE " + std::string(txTypeStr) + "ORDER BY " DB_DATE " ASC";
    }

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
            const char * name = sqlite3_column_name(stmt, i);
            const unsigned char *text = sqlite3_column_text (stmt, i);
            transactions[row][name] = reinterpret_cast<const char *>(text);
        }
    }

    sqlite3_finalize(stmt);
    return 0;
}

int EmvDB::selectTransactionsByRef(std::vector<std::map<std::string, std::string> >& transactions, const char* ref, TrxType trxType)
{
    std::string sql;
    sqlite3_stmt* stmt;
    int rc;


    if (trxType == ALL_TRX_TYPES) {
        sql = "SELECT * FROM " + table + " WHERE " DB_RRN " ='" + std::string(ref) + "'";
    } else if (trxType == REVERSAL) {
        sql = "SELECT * FROM " + table + " WHERE " DB_RRN " = '" + std::string(ref) +"' and " DB_MTI " LIKE '04%'";
    } else {
        char txTypeStr[8] = {0};
        sprintf(txTypeStr, "'%02d%%'", static_cast<int>(trxType));
        sql = "SELECT * FROM " + table + " WHERE " DB_RRN " = '" + std::string(ref) +"' and " DB_PS " LIKE" + std::string(txTypeStr);// , ref, static_cast<int>(trxType));
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

int EmvDB::sumTransactionsInDateRange(std::string& amount, const char *minDate, const char *maxDate, TrxRespCode status, TrxType trxType)
{
    std::string sql;
    sqlite3_stmt *stmt;
    int rc;
    char minDateTrim[16] = {0};
    char maxDateTrim[16] = {0};
    char txTypeStr[20] = {0};

    sprintf(txTypeStr, "'%02d%%'", static_cast<int>(trxType));
    printf("\n Tx type is :%s\n",std::string(txTypeStr).c_str());
    strncpy(minDateTrim, minDate, 10); 
    strncpy(maxDateTrim, maxDate, 10);

    if (trxType == ALL_TRX_TYPES) {   
        if (status == APPROVED) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_MTI " NOT LIKE '04%' and " DB_RESP " = '00' ";
        } else if (status == DECLINED) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM "+ table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "'  and (  " DB_RESP " IS NULL OR " DB_RESP " != '00' OR " DB_MTI " LIKE '04%' )";
        } else if (status == ALL) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' ";
        }
    } else if (trxType == REVERSAL) {
        if (status == APPROVED) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE " DB_MTI " LIKE '04%' and strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "'  and " DB_RESP " = '00' ";
        } else if (status == DECLINED) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE " DB_MTI " LIKE '04%' and strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and (  " DB_RESP " IS NULL OR " DB_RESP " != '00' )";
        } else if (status == ALL) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE " DB_MTI " LIKE '04%' and strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "'";
        }
    } else {
        if (status == APPROVED) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_PS " LIKE '" + txTypeStr + "' and " DB_MTI " NOT LIKE '04%' and "  DB_RESP " = '00' ";
        } else if (status == DECLINED) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE + ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_PS " LIKE '" + txTypeStr + "' and  (  " DB_RESP " IS NULL OR " DB_RESP " != '00' OR " DB_MTI " LIKE '04%' )";
        } else if (status == ALL) {
            sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE + ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_PS " LIKE '" + txTypeStr + "'";
        }
    }
    printf(sql.c_str());
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        return -1;
    }
    // printf("%s - Prepared: %s\n", __FUNCTION__, sql);

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

int EmvDB::countTransactionsInDateRange(std::string& count, const char *minDate, const char *maxDate, TrxRespCode status, TrxType trxType)
{
    std::string sql;
    sqlite3_stmt *stmt;
    int rc;
    char minDateTrim[16] = {0};
    char maxDateTrim[16] = {0};
    char txTypeStr[8] = {0};

    sprintf(txTypeStr, "'%02d%%'", static_cast<int>(trxType));
    strncpy(minDateTrim, minDate, 10); 
    strncpy(maxDateTrim, maxDate, 10);

    if (trxType == ALL_TRX_TYPES) {
        if (status == APPROVED) {
            sql = "SELECT count(*) FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_MTI " NOT LIKE '04%' and " DB_RESP " = '00' ";
        } else if (status == DECLINED) {
            sql = "SELECT count(*) FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and (  " DB_RESP " IS NULL OR " DB_RESP " != '00' OR " DB_MTI " LIKE '04%' )";
        } else if (status == ALL) {
            sql = "SELECT count(*) FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' ";
        }
    } else if (trxType == REVERSAL) {
        if (status == APPROVED) {
            sql = "SELECT count(*) FROM " + table + " WHERE " DB_MTI " LIKE '04%' and strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_RESP " = '00' ";
        } else if (status == DECLINED) {
            sql = "SELECT count(*) FROM " + table + " WHERE " DB_MTI " LIKE '04%' and strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and (  " DB_RESP " IS NULL OR " DB_RESP " != '00' )";
        } else if (status == ALL) {
            sql = "SELECT count(*) FROM " + table + " WHERE " DB_MTI " LIKE '04%' and strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' ";
        }
    } else {
        if (status == APPROVED) {
            sql = "SELECT count(*) FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_PS " LIKE '" + std::string(txTypeStr) + "' and " DB_MTI " NOT LIKE '04%' and " DB_RESP " = '00' ";
        } else if (status == DECLINED) {
            sql = "SELECT count(*) FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_PS " LIKE '" + std::string(txTypeStr) + "' and  (  " DB_RESP " IS NULL OR " DB_RESP " != '00' OR " DB_MTI " LIKE '04%' )";
        } else if (status == ALL) {
            sql = "SELECT count(*) FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_PS " LIKE '" + std::string(txTypeStr) + "'";
        }
    }

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
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

int EmvDB::sumTransactionsOnDate(std::string& amount, const char *date, TrxRespCode status, TrxType trxType)
{
    return sumTransactionsInDateRange(amount, date, date, status, trxType);
}

int EmvDB::countTransactionsOnDate(std::string& count, const char *date, TrxRespCode status, TrxType trxType)
{
    return countTransactionsInDateRange(count, date, date, status, trxType);
}

unsigned long EmvDB::sumTransactions(TrxRespCode status, TrxType trxType)
{
    std::string amount;
    std::vector<std::string> dates;

    if (selectUniqueDates(dates, trxType) != 0) {
        return 0;
    }

    // selectUniqueDates is currently in descending mode, but if that fails try ascending mode
    if (sumTransactionsInDateRange(amount, dates.rbegin()->c_str(), dates.begin()->c_str(), status, trxType) 
    && sumTransactionsInDateRange(amount, dates.begin()->c_str(), dates.rbegin()->c_str(), status, trxType)) {
        return 0;
    }

    // I assume strtoul will never fail because sqlite select sum() will not sum non-numeric values
    return strtoul(amount.c_str(), NULL, 10);
}

int EmvDB::countTransactions(TrxRespCode status, TrxType trxType)
{
    std::string count;
    std::vector<std::string> dates;

    if (selectUniqueDates(dates, trxType) != 0) {
        return 0;
    }

    if (countTransactionsInDateRange(count, dates.rbegin()->c_str(), dates.begin()->c_str(), status, trxType) 
    && countTransactionsInDateRange(count, dates.begin()->c_str(), dates.rbegin()->c_str(), status, trxType)) {
        return 0;
    }

    return strtoul(count.c_str(), NULL, 10);
}

/*
    if (!strncmp(ps, "00", 2))
        return "PURCHASE";
    if (!strncmp(ps, "01", 2))
        return "Cash Advance";
    if (!strncmp(ps, "20", 2))
        return "Refund/Return";
    if (!strncmp(ps, "21", 2))
        return "Deposit";
    if (!strncmp(ps, "09", 2))
        return "Purchase with Cashback";
    if (!strncmp(ps, "31", 2))
        return "Balance Inquiry";
    if (!strncmp(ps, "60", 2))
        return "Pre-Auth";
    if (!strncmp(ps, "61", 2))
        return "Pre-Auth Completion";

     if (!strncmp(ps, "fd", 2))
        return "Reversal";
*/

const char* EmvDB::trxTypeToPS(TrxType type)
{
    switch (type) {
    case PURCHASE:
        return "000000";
    case PURCHASE_WC:
        return "090000";
    case PRE_AUTH:
        return "600000";
    case PRE_AUTH_C:
        return "610000";
    case CASH_ADV:
        return "010000";
    case REFUND:
        return "200000";
    case DEPOSIT:
        return "210000";
    case REVERSAL:
        return "fd";
    default:
        return "N/A";
    }
}

int EmvDB::getStan(char *stanString, size_t size)
{
    int stan = -1;
    int rc = -1;
    sqlite3_stmt *stmt;


    rc = sqlite3_prepare_v2(db, std::string("SELECT SEQ from sqlite_sequence WHERE name='" + table + "'").c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        //LOGF_WARN(log.handle, "%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));\
        return stan;
    } else if (sqlite3_step(stmt) == SQLITE_ROW) {
        stan = (sqlite3_column_int64(stmt, 0) + 1) % (999999 + 1);
        if (!stan) {
            stan = 1;
        }
    } else if (!(stan = countAllTransactions())) {
        stan = 1;
    }

    snprintf(stanString, size, "%06d", stan);
    // printf("DB STAN: %s\n", stanString);
    sqlite3_finalize(stmt);
    return stan;
}

int EmvDB::countAllTransactions()
{
    std::string sql = "SELECT count(*) from " + table;
    sqlite3_stmt *stmt;
 
    int rc = -1;
    
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));   
        return -1;
    } else if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            rc = 0;
        } else {
            rc = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return rc;
}

int EmvDB::lastTransactionTime(std::string& time)
{
    std::string sql = "SELECT " DB_DATE " FROM " + table + " ORDER BY " DB_ID " DESC LIMIT 1";
    sqlite3_stmt* stmt;
    int rc;

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        time = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        rc = 0;
    }

    sqlite3_finalize(stmt);


    return rc;
}

long EmvDB::lastTransactionId()
{
    std::string sql = "SELECT " DB_ID " FROM " + table + " ORDER BY " DB_ID " DESC LIMIT 1";
    sqlite3_stmt* stmt;
    long rowId = 0;
    int rc;

    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return 0;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        rowId = (long)sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return rowId;
}

int EmvDB::clear()
{
    std::string sql = "DELETE FROM " + table;
    sqlite3_stmt *stmt;
    int rc = -1;
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("%s prepare statement failed: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        return -1;
    } else if (sqlite3_step(stmt) == SQLITE_DONE) {
        rc = 0;
    }

    sqlite3_finalize(stmt);

    return rc;
}

int EmvDB::tableExists(sqlite3* db, const char* tableName)
{
    // get current database version of schema
    static sqlite3_stmt *check_table;
    char query[256] = {0};
    int rc = 0;

    sprintf(query, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';", tableName);

    if(sqlite3_prepare_v2(db, query, -1, &check_table, NULL) == SQLITE_OK) {
        if (sqlite3_step(check_table) == SQLITE_ROW) {
            rc = sqlite3_column_int(check_table, 0);
        } else {
            printf("%s -> %s\n", __FUNCTION__, sqlite3_errmsg(db));
        }
    } else {
        printf("%s: ERROR Preparing: , %s\n", __FUNCTION__, sqlite3_errmsg(db) );
    }
    sqlite3_finalize(check_table);

    return rc;
}

int EmvDB::getUserVersion(sqlite3* db)
{
    // get current database version of schema
    static sqlite3_stmt *stmt_version;
    int databaseVersion = -1;

    if(sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt_version, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt_version) == SQLITE_ROW) {
            databaseVersion = sqlite3_column_int(stmt_version, 0);
        } else {
            // printf("%s: Step DB : %s\n", __FUNCTION__, sqlite3_errmsg(db) );
        }
    } else {
        // printf("%s: ERROR Preparing: , %s\n", __FUNCTION__, sqlite3_errmsg(db) );
    }
    sqlite3_finalize(stmt_version);

    return databaseVersion;
}

int EmvDB::init(const std::string& tableName)
{
    int rc;
    sqlite3* db;
    char* errMsg;
    std::string sql;
    
    int version, configuredVersion;

    rc = sqlite3_open_v2(dbFile.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        printf("%s -> Can't open database: %s\n", __FUNCTION__, sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return -1;
    }

    version = EmvDB::getUserVersion(db);
    configuredVersion = atoi(EMV_TABLE_USER_VERSION);

    if (tableExists(db, tableName.c_str()) && configuredVersion != version) {
        std::string dropTable;
        dropTable = "DROP TABLE " + tableName + ";";

        printf("%s -> Current DB version: %d, Recommended version: %d\n", __FUNCTION__, version, configuredVersion);
        rc = sqlite3_exec(db, dropTable.c_str(), NULL, NULL, &errMsg);
        if (rc != SQLITE_OK) {
            printf("%s -> exec failed: %s", __FUNCTION__, errMsg);
            sqlite3_free(errMsg);
            sqlite3_close_v2(db);
            return -1;
        }
    }

    sql = "BEGIN TRANSACTION;"
          "CREATE TABLE IF NOT EXISTS " + tableName + " ("
          DB_ID                 " INTEGER PRIMARY KEY AUTOINCREMENT, "
          DB_MTI                " TEXT NOT NULL, "
          DB_PS                 " TEXT NOT NULL, "
          DB_RRN                " TEXT NOT NULL, "
          DB_STAN               " TEXT NOT NULL, "
          DB_NAME               " TEXT, "
          DB_AID                " TEXT, "
          DB_PAN                " TEXT, "
          DB_LABEL              " TEXT, "
          DB_TEC                " TEXT, "
          DB_AMOUNT             " TEXT, "
          DB_ADDITIONAL_AMOUNT  " TEXT, "
          DB_AUTHID             " TEXT, "
          DB_FISC               " TEXT, "
          DB_EXPDATE            " TEXT NOT NULL, "
          DB_DATE               " TEXT NOT NULL, "
          DB_RESP               " TEXT, "
          DB_TVR                " TEXT, "
          DB_TSI                " TEXT  "
          " ); "
          "PRAGMA user_version = " EMV_TABLE_USER_VERSION "; "
          "CREATE TRIGGER IF NOT EXISTS sanitize AFTER INSERT ON " + tableName + " "
          "BEGIN "
          "DELETE FROM " + tableName + " WHERE (strftime('%Y-%m-%d', " DB_DATE ") = strftime('%Y-%m-%d', (SELECT min(" DB_DATE ") from " + tableName + ")))"
          "and (select count(*) from " + tableName + ") >= " EMV_TABLE_ROW_LIMIT " and strftime('%Y-%m-%d', (select min(" DB_DATE ") from " + tableName + ")) != strftime('%Y-%m-%d', 'now');"
          "END; "
          "COMMIT;"
          "VACUUM;";

    printf("Querry is \n%s\n", sql.c_str());

    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        printf("Table creation failed %s\n", errMsg);
        printf("%s -> exec failed: %s\n", __FUNCTION__, errMsg);
        sqlite3_free(errMsg);
        sqlite3_close_v2(db);
        return -1;
    }
    printf("Table created successfully\n");
    printf("%s -> Ok\n", __FUNCTION__);
    sqlite3_close_v2(db);
    return 0;
}

