#ifndef SRC_DB_UTIL_H
#define SRC_DB_UTIL_H

#include <map>
#include <string>
#include "EmvDB.h"
//#include "DemoForEMV.h"

//#include <html/prt.h>

#include "Nibss8583.h"

struct remove_punct {
    remove_punct()
    {
    }
    bool operator()(char val)
    {
        return ispunct(val);
    }

private:
    char _sentinel;
};



int bcdToSqlDate(char *datetime, size_t dateLen, const unsigned char * bcd, int bcdLen);

int ctxToInsertMap(std::map<std::string, std::string>& trx, const Eft *eft);
int ctxToUpdateMap(std::map<std::string, std::string>& trx, const Eft *eft);

std::map<std::string, std::string> ctxToInsertMap(const Eft *eft);
std::map<std::string, std::string> ctxToUpdateMap(const Eft *eft);

//int printFromEmvDB(const EMV_TRX_CONTEXT *transaction, EMV_ADK_INFO eEMVInfo, bool isPrintingEnabled);
int formatEmvMap(std::map<std::string, std::string>& transaction);

int previewReceiptMap(std::map<std::string, std::string>& values, int display, const char *url);

//vfiprt::PrtError checkedPrint(std::map<std::string, std::string>& values, const char *url);

int fillReceiptHeader(std::map<std::string, std::string>& values);
int fillReceiptFooter(std::map<std::string, std::string>& values);

TrxType toEMVDBTransactionType(TransType txtype);

void printEftReceipt(Eft *eft, unsigned char ucReader, bool printReceipt);
void normalizeDateTime(char * yyyymmddhhmmss, const char * formatedDateTime);
//const char * printerMessage(vfiprt::PrtError status);

#endif
