/**
 * File: EftDBImpl.cpp
 * -------------------
 * Implements EftDBImpl.h's interface.
 */

#include <stdio.h>
#include <string.h>

#include "EftDbImpl.h"
#include "EmvDBUtil.h"
#include "Nibss8583.h"

#include "libapi_xpos/inc/libapi_util.h"



 const  std::string DBNAME = "emvdb.db";
 #define EFT_DEFAULT_TABLE "Transactions"
#define EMVDBUTILLOG "EMVDBUTILLOG"

//int EmvDB::updateTransaction(long atPrimaryIndex, const std::map<std::string, std::string>& record)

static short updateEftRequired(const Eft * eft) 
{
    if (!strncmp(eft->responseCode, "00", 2)) return 1;

    //transaction not to be updated when declined.
    if ((eft->transType == EFT_REVERSAL || eft->transType == EFT_COMPLETION || eft->transType == EFT_REFUND)) 
    {
        fprintf(stderr, "No need to update, transaction declined...\n");
        return 0;
    }

    return 1;
}

short updateEft(const Eft * eft)
{
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    std::map<std::string, std::string> dbmap;

    if (!updateEftRequired(eft)) {
        return -1;
    }

    if (ctxToUpdateMap(dbmap, eft)) {
        return -3;
    }

    if (db.updateTransaction(eft->atPrimaryIndex, dbmap)) {
        return -3;
    }

    return 0;
}

short saveEft(Eft *eft)
{
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    std::map<std::string, std::string> dbmap;

    if (ctxToInsertMap(dbmap, eft)) {
        return -1;
    }

    printf("Started Saving to DB\n");

    eft->atPrimaryIndex = db.insertTransaction(dbmap);

    if (eft->atPrimaryIndex < 0) {
        return -2;
    }

    printf("Successfully saved to DB\n");

    return 0;
}

short getEft(Eft * eft)
{
    TrxType txtype;
    unsigned char processingCodeBcd[3];
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    std::vector<std::map<std::string, std::string> > vec_map;

    std::map<std::string, std::string> dbmap;
    //txtype = toEMVDBTransactionType(ALL_TRX_TYPES);

    if (db.selectTransactionsByRef(vec_map, eft->rrn, ALL_TRX_TYPES)) {
        return -1;
    }

    if(vec_map.empty()) {
        printf("Ref doesnt exist\n");
        return -1;
    }
    
    dbmap = vec_map[0];

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), sizeof(eft->pan));
    strncpy(eft->aid, dbmap[DB_AID].c_str(), sizeof(eft->aid));
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), sizeof(eft->additionalAmount));
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), sizeof(eft->authorizationCode));
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), sizeof(eft->cardHolderName));
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), sizeof(eft->cardLabel));
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), sizeof(eft->responseCode));
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), sizeof(eft->stan));
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), sizeof(eft->rrn));
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), sizeof(eft->tvr));
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), sizeof(eft->tsi));
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), sizeof(eft->originalMti));
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), sizeof(eft->originalMti));
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), sizeof(eft->expiryDate));

    sprintf(eft->amount, "%012zu", atol(dbmap[DB_AMOUNT].c_str()));
    normalizeDateTime(eft->yyyymmddhhmmss, dbmap[DB_DATE].c_str());
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, sizeof(eft->originalYyyymmddhhmmss));

    if (decodeProcessingCode(&eft->transType, &eft->fromAccount, &eft->toAccount, dbmap[DB_PS].c_str(), dbmap[DB_MTI].c_str())) {
        fprintf(stderr, "Error decoding processing code...\n");
        return -2;
    }

    eft->atPrimaryIndex = atoll(dbmap[DB_ID].c_str());

    return 0;
}

short getLastTransaction(Eft * eft)
{
    unsigned char processingCodeBcd[3];
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    
    std::map<std::string, std::string> dbmap;
    eft->atPrimaryIndex = db.lastTransactionId();

    if(eft->atPrimaryIndex <= 0) {
        return -1;
    }

    printf("Last Primary index : %d\n", eft->atPrimaryIndex);

    db.select(dbmap, eft->atPrimaryIndex);

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), sizeof(eft->pan));
    strncpy(eft->aid, dbmap[DB_AID].c_str(), sizeof(eft->aid));
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), sizeof(eft->additionalAmount));
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), sizeof(eft->authorizationCode));
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), sizeof(eft->cardHolderName));
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), sizeof(eft->cardLabel));
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), sizeof(eft->responseCode));
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), sizeof(eft->stan));
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), sizeof(eft->rrn));
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), sizeof(eft->tvr));
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), sizeof(eft->tsi));
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), sizeof(eft->originalMti));
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), sizeof(eft->originalMti));
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), sizeof(eft->expiryDate));

    sprintf(eft->amount, "%012zu", atol(dbmap[DB_AMOUNT].c_str()));
    normalizeDateTime(eft->yyyymmddhhmmss, dbmap[DB_DATE].c_str());
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, sizeof(eft->originalYyyymmddhhmmss));

    if (decodeProcessingCode(&eft->transType, &eft->fromAccount, &eft->toAccount, dbmap[DB_PS].c_str(), dbmap[DB_MTI].c_str())) {
        fprintf(stderr, "Error decoding processing code...\n");
        return -2;
    }

    return 0;
}



void printTodayTransaction()
{

    typedef std::vector<std::string>::iterator DateIter_type;

    EmvDB db("EMV",DBNAME);
    TrxType txtype = PURCHASE;
    std::vector<std::string> datelist;
    std::vector<std::map<std::string, std::string> > transaction_list;
    db.selectUniqueDates(datelist, txtype);
    db.selectTransactionsOnDate(transaction_list, datelist.at(0).c_str(), txtype);
  //  for(DateIter_type index = datelist.begin(); index < datelist.end(); index++){
      //  db.se
   // }
   

    printf("size of vector is %ld\n first item is %s\n", transaction_list.size(), datelist.at(0).c_str());

}