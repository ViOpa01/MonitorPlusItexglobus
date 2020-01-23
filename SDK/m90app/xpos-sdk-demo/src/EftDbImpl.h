/**
 * File: EftDbImpl.h 
 * -----------------
 * Implements EmvDB interface.
 */

#ifndef _EFT_DB_IMPLEMENTATION_INCLUDED
#define _EFT_DB_IMPLEMENTATION_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "Nibss8583.h"

//Used By EOD RECIEPT PRINTING DECLARED IN EMVDB//
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

short getEft(Eft * eft);
short saveEft(Eft *eft);
short updateEft(const Eft * eft);
void getListOfEod(Eft * eft, TrxType txtype);
short getLastTransaction(Eft * eft);

#ifdef __cplusplus
}
#endif

#endif 
