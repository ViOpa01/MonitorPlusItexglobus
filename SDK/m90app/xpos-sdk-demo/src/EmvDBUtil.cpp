#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>

#include "unistd.h"
#include "EmvDBUtil.h"
#include "EmvDB.h"
#include "util.h"

void normalizeDateTime(char * yyyymmddhhmmss, const char * formatedDateTime)
{
    int i;
    short pos = 0;
    int len = strlen(formatedDateTime);

    for (i = 0; i < len; i++) {

        if (pos >= 14) break;
        if (isdigit(formatedDateTime[i])) {
            yyyymmddhhmmss[pos++] = formatedDateTime[i];
        }
    }
}

static void formartAmount(const char *amountIn, char *amountOut)
{
    int len;
    for (len = 0; len < strlen(amountIn); len++)
    {
        if (amountIn[len] != '0')
        {
            break;
        }
    }
    sprintf(amountOut, "%s", &amountIn[len]);
}



int ctxToInsertMap(std::map<std::string, std::string> &trx, const Eft *eft)
{
    const char * mti = NULL;
    char pan[24] = {0};
    char ps[8] = {0};
    char amount[13] = {0};
    char additionalAmount[13] = {0};
    char date[32] = {0};
    char expDate[16] = {0};
    unsigned char yymmddhhmmss[6] = {0};
    char aid[32] = {0};
    unsigned char label[32] = {0};
    char name[64] = {0};
    char tvr[12] = {0};
    char tsi[6] = {0};
    char stan[8] = {0};

    unsigned long TAGR = 0x9F12;
    unsigned short TAGL;
    char formattedDate[26] = {0};
    char formattedAmount[10] = {0};

    sprintf(ps, "%02X%02X%02X", transTypeToCode(eft->transType), accountTypeToCode(eft->fromAccount), accountTypeToCode(eft->toAccount));
    
    trx[DB_LABEL] = eft->cardLabel;
    if (!eft)
        return -1;

    mti = transTypeToMti(eft->transType);

    if (mti == NULL) {
        printf("Error getting mti...\n");
        return -2;
    }


    if (eft->techMode == CONTACTLESS_MODE)
    {
        trx[DB_TEC] = "CTLS";
    }
    else if (eft->techMode == CHIP_MODE)
    {
        trx[DB_TEC] = "CT";
    } 
    else if(eft->techMode == MAGSTRIPE_MODE)
    {
        trx[DB_TEC] = "MSR";
    }
    else if (eft->techMode == CONTACTLESS_MAGSTRIPE_MODE)
    {

        trx[DB_TEC] = "CTLS-MSR";
    }
    

    if (beautifyDateTime(formattedDate, sizeof(formattedDate), eft->yyyymmddhhmmss)) return -3;
    formartAmount(eft->amount, formattedAmount);

    trx[DB_NAME] = eft->dbName;
    trx[DB_MTI] = mti;
    trx[DB_PS] = ps;
    trx[DB_RRN] = eft->rrn;
    trx[DB_STAN] = eft->stan;
    trx[DB_DATE] = formattedDate;
    trx[DB_AID] = eft->aid;
    trx[DB_EXPDATE] = eft->expiryDate;
    trx[DB_TVR] = eft->tvr;
    trx[DB_TSI] = eft->tsi;
    trx[DB_RESP] = eft->responseCode;

    trx[DB_AMOUNT] = formattedAmount;
    if (eft->additionalAmount)
    {
        trx[DB_ADDITIONAL_AMOUNT] = eft->additionalAmount;
    }
    trx[DB_PAN] = eft->pan;

    for (std::map<std::string, std::string>::iterator it = trx.begin(); it != trx.end(); ++it)
    {
        printf("%s -> '%s'\n", it->first.c_str(), it->second.c_str());
    }

    return 0;
}

int ctxToUpdateMap(std::map<std::string, std::string> &trx, const Eft *eft)
{
    const char * mti = NULL;
    char ps[8] = {0};
    char amount[13] = {0};
    // LogManager log(EMVDBUTILLOG);

    if (!eft)
        return -1;
    int transactionCode;
    sprintf(ps, "%02X%02X%02X", transTypeToCode(eft->transType), accountTypeToCode(eft->fromAccount), accountTypeToCode(eft->toAccount));
    mti = transTypeToMti(eft->transType);
    
    if (mti == NULL) {
        printf("Can't get mti...\n");
        return -2;
    }

    strcpy(amount, eft->amount);
    trx[DB_AMOUNT] = amount;
    trx[DB_PS] = ps;

    trx[DB_MTI] = mti;
    trx[DB_RESP] = eft->responseCode;

    trx[DB_RRN] = eft->rrn;
    trx[DB_STAN] = eft->stan;
    trx[DB_FISC] = eft->forwardingInstitutionIdCode;
    trx[DB_AUTHID] = eft->authorizationCode;

    for (std::map<std::string, std::string>::iterator it = trx.begin(); it != trx.end(); ++it)
    {
        printf("%s -> '%s'", it->first.c_str(), it->second.c_str());
    }

    return 0;
}

std::map<std::string, std::string> ctxToInsertMap(const Eft *eft)
{
    std::map<std::string, std::string> trx;
    ctxToInsertMap(trx, eft);
    return trx;
}

std::map<std::string, std::string> ctxToUpdateMap(const Eft *eft)
{
    std::map<std::string, std::string> trx;
    ctxToUpdateMap(trx, eft);
    return trx;
}


TrxType toEMVDBTransactionType(TransType txtype)
{
    switch (txtype)
    {
    case EFT_PURCHASE :
        return PURCHASE;
    case EFT_PREAUTH :
        return PRE_AUTH;
    case EFT_CASHADVANCE :
        return CASH_ADV;
    case EFT_REVERSAL:
        return REVERSAL;
    case EFT_COMPLETION:
        return PRE_AUTH_C;
    case EFT_CASHBACK:
        return PURCHASE_WC;
    case EFT_REFUND:
        return REFUND;
    default:
     return ALL_TRX_TYPES;
    }
    return ALL_TRX_TYPES;
}

