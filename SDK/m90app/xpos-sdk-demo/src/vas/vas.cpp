#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iterator>
#include <algorithm>

#include "platform/platform.h"

#include "vasdb.h"
#include "vasmenu.h"
#include "payvice.h"

#include "jsonwrapper/jsobject.h"
#include "./platform/simpio.h"
#include "./platform/vasutility.h"

#include "vas.h"
#include "airtime.h"
#include "data.h"
#include "electricity.h"
#include "paytv.h"
#include "smile.h"

#include "vasadmin.h"
#include "../EmvDB.h"
#include "../merchant.h"
#include "../Receipt.h"
#include "../nibss.h"

extern "C" {
#include "../util.h"
#include "../remoteLogo.h"
#include "../ezxml.h"
#include "../appInfo.h"
#include "libapi_xpos/inc/libapi_gui.h"
}

static int printVasUssdPin(std::map<std::string, std::string> &record, const VAS_Menu_T type);

const char* serviceToString(Service service)
{
    switch (service) {
    case IKEJA:
        return "Ikeja Electric";
    case EKEDC:
        return "Eko Electric";
    case EEDC:
        return "Enugu Electric";
    case IBEDC:
        return "Ibadan Electric";
    case PHED:
        return "Port-Harcourt Elec.";
    case AEDC:
        return "Abuja Electric";
    case KEDCO:
        return "Kano Electric";
    case AIRTELVTU:
        return "Airtel VTU";
    case AIRTELVOT:
        return "Airtel VOT";
    case AIRTELVOS:
        return "Airtel VOS";
    case AIRTELDATA:
        return "Airtel Data";
    case ETISALATVTU:
        return "9MOBILE VTU";
    case ETISALATVOT:
        return "9MOBILE VOT";
    case ETISALATVOS:
        return "9MOBILE VOS";
    case ETISALATDATA:
        return "9Mobile Data";
    case GLOVTU:
        return "Glo VTU";
    case GLOVOT:
        return "Glo VOT";
    case GLOVOS:
        return "Glo VOS";
    case GLODATA:
        return "Glo Data";
    case MTNVTU:
        return "MTN VTU";
    case MTNVOT:
        return "MTN VOT";
    case MTNVOS:
        return "MTN VOS";
    case MTNDATA:
        return "MTN Data";
    case DSTV:
        return "DSTv";
    case GOTV:
        return "GOTV";
    case SMILETOPUP:
        return "Smile Topup";
    case SMILEBUNDLE:
        return "Smile Bundle";
    case STARTIMES:
        return "Startimes";
    case TRANSFER:
        return "Transfer";
    case WITHDRAWAL:
        return "Withdrawal";
    case JAMB_UTME_PIN:
        return "JAMB UTME PIN";
    case JAMB_DE_PIN:
        return "JAMB DE PIN";
    case WAEC_REGISTRATION:
        return "WAEC Registration";
    case WAEC_RESULT_CHECKER:
        return "WAEC Result Checker";
    case AIRTIME_USSD:
        return "Airtime PIN";
    case DATA_USSD:
        return "Data PIN";
    case ELECTRICITY_USSD:
        return "Electricity PIN";
    case PAYTV_USSD:
        return "Pay-TV PIN";
    case WALLET_FUNDING:
        return "Wallet Funding";
    default:
        return "";
    }
}

const char* serviceStringToLogoFile(const std::string& serviceString)
{
    if (serviceString == serviceToString(IKEJA)) {
        return "vaslogos\\ikeja.bmp";
    } else if (serviceString == serviceToString(EKEDC)) {
        return "vaslogos\\ekedc.bmp";
    } else if (serviceString == serviceToString(EEDC)) {
        return "vaslogos\\eedc.bmp";
    } else if (serviceString == serviceToString(IBEDC)) {
        return "vaslogos\\ibedc.bmp";
    } else if (serviceString == serviceToString(PHED)) {
        return "vaslogos\\phed.bmp";
    } else if (serviceString == serviceToString(AEDC)) {
        return "vaslogos\\aedc.bmp";
    } else if (serviceString == serviceToString(KEDCO)) {
        return "vaslogos\\kano.bmp";
    } else if (serviceString == serviceToString(DSTV)) {
        return "vaslogos\\dstv.bmp";
    } else if (serviceString == serviceToString(GOTV)) {
        return "vaslogos\\gotv.bmp";
    } else if (serviceString == serviceToString(STARTIMES)) {
        return "vaslogos\\stimes.bmp";  // startimes
    } else if (serviceString == serviceToString(SMILETOPUP)
                || serviceString == serviceToString(SMILEBUNDLE)) {
        return "vaslogos\\smile.bmp";
    } else if (serviceString == serviceToString(ETISALATVTU)
                || serviceString == serviceToString(ETISALATDATA)
                ||  serviceString == serviceToString(ETISALATVOS)
                ||  serviceString == serviceToString(ETISALATVOT) ) {
        return "vaslogos\\esalat.bmp";  // etisalat
    } else if (serviceString == serviceToString(AIRTELVTU)
                || serviceString == serviceToString(AIRTELDATA)
                ||  serviceString == serviceToString(AIRTELVOS)
                ||  serviceString == serviceToString(AIRTELVOT) ) {
        return "vaslogos\\airtel.bmp";
    } else if (serviceString == serviceToString(GLOVTU)
                || serviceString == serviceToString(GLODATA)
                ||  serviceString == serviceToString(GLOVOS)
                ||  serviceString == serviceToString(GLOVOT) ) {
        return "vaslogos\\glo.bmp";
    } else if (serviceString == serviceToString(MTNVTU)
                || serviceString == serviceToString(MTNDATA)
                ||  serviceString == serviceToString(MTNVOS)
                ||  serviceString == serviceToString(MTNVOT) ) {
        return "vaslogos\\mtn.bmp";

    } else if(serviceString == serviceToString(WITHDRAWAL)
                || serviceString == serviceToString(TRANSFER)) {
        // return "itex/bank.bmp";
        return "vaslogos\\itex.bmp";
    } else if(serviceString == serviceToString(JAMB_UTME_PIN)
                || serviceString == serviceToString(JAMB_DE_PIN)) {
        return "vaslogos\\jamb.bmp";
    } else if(serviceString == serviceToString(WAEC_REGISTRATION)
                || serviceString == serviceToString(WAEC_RESULT_CHECKER)) {
        return "vaslogos\\waec.bmp";
    }
    return "";
}

const char* serviceToProductString(Service service)
{
    switch (service) {
    case IKEJA:
        return "IKEJAELECTRIC";
    case EKEDC:
        return "EKOELECTRIC";
    case EEDC:
        return "ENUGUELECTRIC";
    case IBEDC:
        return "IBADANELECTRIC";
    case PHED:
        return "PORTELECTRIC";
    case AEDC:
        return "ABUJAELECTRIC";
    case KEDCO:
        return "KANOELECTRIC";
    case ETISALATVTU:
        return "ETISALATVTU";
    case AIRTELVTU:
        return "AIRTELVTU";
    case GLOVTU:
        return "GLOVTU";
    case MTNVTU:
        return "MTNVTU";
    case ETISALATDATA:
        return "ETISALATDATA";
    case AIRTELDATA:
        return "AIRTELDATA";
    case GLODATA:
        return "GLODATA";
    case MTNDATA:
        return "MTNDATA";
    case DSTV:
        return "DSTV";
    case GOTV:
        return "GOTV";
    case WITHDRAWAL:
        return "WITHDRAWAL";
    case TRANSFER:
        return "TRANSFER";
    case SMILETOPUP:
        return "SMILE TOPUP";
    case SMILEBUNDLE:
        return "SMILE BUNDLE";
    // case VAS_GENESIS:
    // 	return "GENESIS";
    case MTNVOS:
        return "MTNVOS";
    case ETISALATVOS:
        return "ETISALATVOS";
    case AIRTELVOS:
        return "AIRTELVOS";
    case GLOVOS:
        return "GLOVOS";
    case MTNVOT:
        return "MTNVOT";
    case ETISALATVOT:
        return "ETISALATVOT";
    case AIRTELVOT:
        return "AIRTELPIN";
    case GLOVOT:
        return "GLOVOT";
    case JAMB_UTME_PIN:
        return "JAMBUTMEPIN";
    case JAMB_DE_PIN:
        return "JAMBDEPIN";
    case WAEC_REGISTRATION:
        return "WAEC REGISTRATION";
    case WAEC_RESULT_CHECKER:
        return "WAEC RESULT CHECKER";
    case AIRTIME_USSD:
        return "AIRTIMEUSSD";
    default:
        return "VAS";
    }
}

const char* paymentString(PaymentMethod method)
{
    switch (method) {
    case PAY_WITH_CARD:
        return "Card";
    case PAY_WITH_CASH:
        return "Cash";
    case PAY_WITH_MCASH:
        return "mCash";
    case PAY_WITH_CGATE:
        return "CGate";
    case PAY_WITH_NQR:
        return "QR";
    default:
        return "";
    }
}

const char* apiPaymentMethodString(PaymentMethod method)
{
    switch (method) {
    case PAY_WITH_CARD:
        return "card";
    case PAY_WITH_CASH:
        return "cash";
    case PAY_WITH_MCASH:
    case PAY_WITH_CGATE:
        return "USSD";
    case PAY_WITH_NQR:
        return "NQR";
    default:
        return "";
    }
}

int initVasTables()
{
    EmvDB::init(VASDB_CARD_TABLE, VASDB_FILE);    
    VasDB::init();
    return 0;
}

std::map<std::string, std::string> pinObjToMap(const iisys::JSObject& pinObj)
{
    std::map<std::string, std::string> pin;

    if (!pinObj.isObject()) {
        return pin;
    }

    for (iisys::JSObject::const_iterator begin = pinObj.begin(), end = pinObj.end();
                                        begin != end; ++begin) {
        if (begin->second.isString()) {

            pin[begin->first] = begin->second.getString();
        }
    }

    return pin;
}

int printVas(std::map<std::string, std::string>& record)
{
    iisys::JSObject serviceData;
    int printStatus;
    int printPins = -1;

    if (serviceData.load(record[VASDB_SERVICE_DATA]) && serviceData.isObject()) {
        for (iisys::JSObject::iterator begin = serviceData.begin(),
                                        end = serviceData.end(); begin != end; ++begin) {
            if (record.find(begin->first) == record.end() && begin->second.isString())
                record[begin->first] = begin->second.getString();
        }
    }

    vasimpl::formatAmount(record[VASDB_AMOUNT]);
    record["currencySymbol"] = getVasCurrencySymbol();
    if(record.find(VASDB_DATE) != record.end() && record[VASDB_DATE].length() > 19) {
        record[VASDB_DATE].erase(19, std::string::npos);
    }

    record["vaslogo"] = serviceStringToLogoFile(record[VASDB_SERVICE]);

    const char* copies[] = {"- Customer Copy -", "- Agent Copy -"};    
    int count = 1;
    if (record.find("receipt_copy") == record.end()) {
        if (record[VASDB_STATUS] == VasDB::trxStatusString(VasDB::APPROVED)) {
            count = (int) sizeof(copies) / sizeof(char*);
        }
        record["receipt_copy"] = copies[0];
    } else if (record[VASDB_CATEGORY] == vasMenuString(VAS_USSD)){
        if (UI_ShowOkCancelMessage(10000, "Print", "Confirm to print pin(s)", UI_DIALOG_TYPE_CONFIRMATION) == 0) {
            printPins = 1;
        }
    }

    if (record.find(DB_MTI) != record.end() && !strncmp(record[DB_MTI].c_str(), "04", 2)) {
        std::string resp = record[DB_RESP];
        record[DB_RESP] = "Reversal(" + resp + ")";
    }

    for (int i = 0; i < count; ++i) {
        if (count > 1) {
            record["receipt_copy"] = copies[i];
            if (i > 0 && UI_ShowOkCancelMessage(10000, copies[i], "Print", UI_DIALOG_TYPE_CONFIRMATION) != 0) {
                break;
            }
        }
 

        if (record[VASDB_CATEGORY] == vasMenuString(ENERGY)) {
            printStatus = printVasReceipt(record, ENERGY);
        } else if (record[VASDB_CATEGORY] == vasMenuString(AIRTIME)) {
            printStatus = printVasReceipt(record, AIRTIME);
        } else if (record[VASDB_CATEGORY] == vasMenuString(DATA)) {
            printStatus = printVasReceipt(record, DATA);
        } else if (record[VASDB_CATEGORY] == vasMenuString(TV_SUBSCRIPTIONS)) {
            printStatus = printVasReceipt(record, TV_SUBSCRIPTIONS);
        } else if (record[VASDB_CATEGORY] == vasMenuString(SMILE)) {
            printStatus = printVasReceipt(record, SMILE);
        } else if(record[VASDB_CATEGORY] == vasMenuString(CASHIN)){
            std::map<std::string, std::string>::iterator itr;

            for(itr = record.begin(); itr != record.end(); ++itr) {
                printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
            }

            printStatus = printVasReceipt(record, CASHIN);
            
        } else if(record[VASDB_CATEGORY] == vasMenuString(CASHOUT)){
            std::map<std::string, std::string>::iterator itr;

            for(itr = record.begin(); itr != record.end(); ++itr) {
                printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
            }

            printStatus = printVasReceipt(record, CASHOUT);
            
        } else if (record[VASDB_CATEGORY] == vasMenuString(JAMB_EPIN)) {
            printStatus = printVasReceipt(record, JAMB_EPIN);
        } else if (record[VASDB_CATEGORY] == vasMenuString(WAEC)) {
            printStatus = printVasReceipt(record, WAEC);
        } else if (record[VASDB_CATEGORY] == vasMenuString(VAS_USSD)) {
            const iisys::JSObject& pins = serviceData("pins");
            
            vasimpl::formatAmount(record["amount"]);

            if ((count != 1 && i == 0 || printPins == 1)  && pins.isArray()) {
                const iisys::JSObject& hintObj = serviceData("hint");
                const iisys::JSObject& batchNoObj = serviceData("batchNo");
                const iisys::JSObject& productObj = serviceData("product");
                const iisys::JSObject& descriptionObj = serviceData("description");

                const std::string hint = hintObj.isString() ? hintObj.getString() : "";
                std::string amount = record["amount"];
                const std::string batchNo = batchNoObj.isString() ? batchNoObj.getString() : "";
                const std::string product = productObj.isString() ? productObj.getString() : "";
                const std::string description = descriptionObj.isString() ? descriptionObj.getString() : "";

                for (int index = 0; index < pins.size(); ++index) {
                    std::map<std::string, std::string> pinrecord = pinObjToMap(pins[index]);
                    pinrecord[VASDB_SERVICE] = record[VASDB_SERVICE];
                    pinrecord["currencySymbol"] = record["currencySymbol"];
                    pinrecord["merchantName"] = record["merchantName"];
                    pinrecord["termianlId"] = record["termianlId"];
                    pinrecord["VASDB_DATE"] = record[VASDB_DATE];
                    pinrecord["hint"] = hint;
                    pinrecord["amount"] = amount;
                    pinrecord["batchNo"] = batchNo;
                    pinrecord["product"] = product;
                    pinrecord["description"] = description;
                    printVasUssdPin(pinrecord, VAS_USSD);
                }
            }
            printStatus = printVasReceipt(record, VAS_USSD);
        } else {
            printStatus = printDefaultVasReceipt(record);
        }
    }


    return printStatus;
}

void printAirtime(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_PRODUCT, "pin", "pin_ref", "serial", "expiry", "dial"};
    const char* labels[] = {"WALLET", "TXN TID", "BENEFICIARY", "BUNDLE", "PIN", "PIN REF", "SERIAL", "EXPIRY", "TO LOAD"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            if(*(record[keys[i]].c_str()))
                printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}


void printCashio(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_BENEFICIARY_NAME, "recBank", "narration"};
    const char* labels[] = {"WALLET", "TID","RECIPIENT", "ACC NAME ", "BANK ", "NARRATION "};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            if(*(record[keys[i]].c_str()))
                printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

void printTokenList(const char* token, const char* description, std::map<std::string, std::string> &record) {

    if(!record[token].empty() && !record[description].empty()) {
        char buff[128] = {'\0'};
        UPrint_SetFont(8, 2, 2);

        sprintf(buff, "***%s***", record[description].c_str());
        UPrint_StrBold(buff, 1, 1, 1);

        memset(buff, '\0', sizeof(buff));
        strncpy(buff, record[token].c_str(), record[token].length());
        UPrint_StrBold(buff, 1, 4, 1);
        printDottedLine();
    }
}

void printElectricity(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_BENEFICIARY_NAME, VASDB_BENEFICIARY_ADDR, VASDB_BENEFICIARY_PHONE
        , "account_type", "type", "tran_id", "client_id", "sgc", "msno", "krn", "ti", "tt", "unit", "sgcti", "accountNo", "tariffCode"
        , "rate", "units", "region", "token", "token1", "token2", "token3", "unit_value", "unit_cost", "vat", "agent", "arrears", "receipt_no", "invoiceNumber"
        , "tariff", "lastTxDate", "collector", "csp"};
    const char* labels[] = {"WALLET", "TXN TID", "METER NO", "NAME ", "ADDRESS ", "PHONE"
        , "ACCOUNT TYPE", "TYPE", "TRAN ID", "CLIENT ID", "SGC", "MSNO", "KRN", "TI", "TT", "UNIT", "SGCTI", "ACCOUNT NO", "TARIFF CODE"
        , "RATE", "UNITS", "REGION", "TOKEN", "TOKEN1", "TOKEN2", "TOKEN3", "UNIT VALUE", "UNIT COST", "VAT", "AGENT", "ARREARS", "RECEIPT NO", "INVOICE NUMBER"
        , "TARIFF", "LAST TXN DATE", "COLLECTOR", "CSP"};


    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {

            if(!strcmp("TOKEN", labels[i]) && !record["token"].empty()) {

                char buff[128] = {'\0'};

                printDottedLine();
                UPrint_SetFont(8, 2, 2);

                strncpy(buff, "**Token**", 9);
                UPrint_StrBold(buff, 1, 1, 1);

                memset(buff, '\0', sizeof(buff));
                strncpy(buff, record[keys[i]].c_str(), record[keys[i]].length());
                UPrint_StrBold(buff, 1, 4, 1);
                printDottedLine();
                
            } else if (!strcmp("TOKEN1", labels[i]) && !record["token1"].empty() && !record["token1_desc"].empty()) {

                printDottedLine();
                printTokenList("token1", "token1_desc", record);

            } else if (!strcmp("TOKEN2", labels[i]) && !record["token2"].empty() && !record["token2_desc"].empty()) {

                printDottedLine();
                printTokenList("token2", "token2_desc", record);

            } else if (!strcmp("TOKEN3", labels[i]) && !record["token3"].empty() && !record["token3_desc"].empty()) {

                printDottedLine();
                printTokenList("token3", "token3_desc", record);

            } else {
                char buff[25] = {'\0'};
                strncpy(buff, record[keys[i]].c_str(), sizeof(buff) - 1);
                printLine(labels[i], buff);
            }
            
        }
    } 
}

void printTv(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_BENEFICIARY_NAME, VASDB_BENEFICIARY_PHONE, "paymentMethod", "reference", "packageName", "packageMonth"};
    const char* labels[] = {"WALLET", "TXN TID", "IUC", "NAME", "PHONE", "PAYMENT METHOD", "REF", "PACKAGE NAME", "PACKAGE MONTH"};

     for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            if(*(record[keys[i]].c_str()))
                printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

void printJambEpin(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_BENEFICIARY_NAME, VASDB_BENEFICIARY_PHONE, VASDB_PRODUCT, "pin", "email", "expiry", "dial"};
    const char* labels[] = {"WALLET", "TXN TID", "CONF. CODE", "NAME", "PHONE", "BUNDLE", "PIN", "EMAIL", "EXPIRY", "TO LOAD"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            if(*(record[keys[i]].c_str()))
                printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

void printWaec(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_BENEFICIARY_NAME, VASDB_BENEFICIARY_PHONE, VASDB_PRODUCT, "pin", "token", "email", "expiry", "dial"};
    const char* labels[] = {"WALLET", "TXN TID", "CONF. CODE", "NAME", "PHONE", "BUNDLE", "PIN", "TOKEN", "EMAIL", "EXPIRY", "TO LOAD"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            if(*(record[keys[i]].c_str()))
                printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

void printVasUssd(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_BENEFICIARY_NAME, VASDB_BENEFICIARY_PHONE, VASDB_PRODUCT, "expiry", "batchNo", "description"};
    const char* labels[] = {"WALLET", "TXN TID", "BENF. NO.", "NAME", "PHONE", "BUNDLE", "EXPIRY", "BATCH NO", "DESCRIPTION"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            if(*(record[keys[i]].c_str()))
                printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

void printDefault(std::map<std::string, std::string> &record)
{
    const char* keys[] = {"walletId", "virtualTid", VASDB_BENEFICIARY, VASDB_BENEFICIARY_NAME, VASDB_BENEFICIARY_PHONE, VASDB_PRODUCT};
    const char* labels[] = {"WALLET", "TXN TID", "BENEFICIARY", "PHONE", "NAME", "BUNDLE"};

    for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
        if (record.find(keys[i]) != record.end()) {
            if(*(record[keys[i]].c_str()))
                printLine(labels[i], record[keys[i]].c_str());
        }
    } 
}

static void printAsteric(size_t len)
{
    char line[32] = {'\0'};

    memset(line, '*', len + 4);
    UPrint_StrBold(line, 1, 4, 1);
}

int printVasReceipt(std::map<std::string, std::string> &record, const VAS_Menu_T type)
{
    MerchantData mParam = {'\0'};
    MerchantParameters parameter = {'\0'};
    int ret = 0;
    char buff[32] = {'\0'};
    char logoFileName[64] = {'\0'};
    
    std::map<std::string, std::string>::iterator itr;

    for(itr = record.begin(); itr != record.end(); ++itr) {
        printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
    }

    readMerchantData(&mParam);
    getParameters(&parameter);

    while(1) {
        ret = UPrint_Init();

        if (ret == UPRN_OUTOF_PAPER) {
            if (UI_ShowButtonMessage(0, "Print", "No paper", "confirm", UI_DIALOG_TYPE_CONFIRMATION)) {
                break;
            }
        }

        // Print Bank Logo
        strcpy(logoFileName, record["vaslogo"].c_str());
        printReceiptLogo(logoFileName);
        printReceiptHeader(record[VASDB_DATE].c_str());

        if(type == ENERGY) {
            memset(buff, '\0', sizeof(buff));
            strcpy(buff, record[VASDB_PRODUCT].c_str());
            UPrint_StrBold(buff, 1, 4, 1);
        }

        strcpy(buff, record[VASDB_SERVICE].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record["receipt_copy"].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        UPrint_SetDensity(3); //Set print density to 3 normal
        UPrint_SetFont(7, 2, 2);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record[VASDB_STATUS].c_str());
        UPrint_StrBold(buff, 1, 4, 1);
        UPrint_Feed(12);

        if (type == ENERGY) {
            printElectricity(record);
        } else if(type == AIRTIME || type == DATA || type == SMILE) {
            printAirtime(record);
        } else if(type == TV_SUBSCRIPTIONS) {
            printTv(record);
        } else if(type == CASHIN || type == CASHOUT) {
            printCashio(record);
        } else if(type == JAMB_EPIN) {
            printJambEpin(record);
        } else if(type == WAEC) {
            printWaec(record);
        } else if(type == VAS_USSD) {
            printVasUssd(record);
        }

        printLine("PAYMENT METHOD", record[VASDB_PAYMENT_METHOD].c_str());

        if(!record[VASDB_REF].empty())
        {
            printLine("REF ", record[VASDB_REF].c_str());
        }
        
        if(!record[VASDB_TRANS_SEQ].empty())
        {
            printLine("TRANS SEQ", record[VASDB_TRANS_SEQ].c_str());
        }

        if(!record[VASDB_VIRTUAL_TID].empty())
        {
            printLine("VID", record[VASDB_VIRTUAL_TID].c_str());
        }

        if(mParam.tid[0]) {
            printLine("TID", mParam.tid);
        }

        // "card" is added because requery uses lower case
        if(record[VASDB_PAYMENT_METHOD] == paymentString(PAY_WITH_CARD) || record[VASDB_PAYMENT_METHOD] == "card") {

            const char *keys[] = {DB_NAME, DB_PAN, DB_AID, DB_LABEL, DB_EXPDATE, DB_RRN, DB_AUTHID, DB_RESP, DB_TVR, DB_TSI};
            const char *labels[] = {"CARD NAME", "PAN", "AID", "LABEL", "EXPIRY", "CREF", "AUTH CODE", "RESP CODE", "TVR", "TSI"};

            for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
                if (record.find(keys[i]) != record.end()) {
                    if(*(record[keys[i]].c_str()))
                        printLine(labels[i], record[keys[i]].c_str());
                }
            } 
        }
		printLine("MID", parameter.cardAcceptiorIdentificationCode);
        printLine("DATE TIME", record[VASDB_DATE].c_str());

        memset(buff, '\0', sizeof(buff));
        sprintf(buff, "NGN %s", record[VASDB_AMOUNT].c_str());

        printAsteric(strlen(buff));
        UPrint_StrBold(buff, 1, 4, 1);
        printAsteric(strlen(buff));

        UPrint_Feed(12);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record[VASDB_STATUS_MESSAGE].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        printDottedLine();
        printVasFooter();

        ret = UPrint_Start();
        if(getPrinterStatus(ret) < 0 ) {
            break;
        }
    }
    

    return ret;
}

int printDefaultVasReceipt(std::map<std::string, std::string> &record)
{
    MerchantData mParam = {'\0'};
    int ret = 0;
    char buff[32] = {'\0'};
    char logoFileName[64] = {'\0'};
    
    std::map<std::string, std::string>::iterator itr;

    for(itr = record.begin(); itr != record.end(); ++itr) {
        printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
    }

    readMerchantData(&mParam);

    while(1) {
        ret = UPrint_Init();

        if (ret == UPRN_OUTOF_PAPER) {
            if (UI_ShowButtonMessage(0, "Print", "No paper", "confirm", UI_DIALOG_TYPE_CONFIRMATION)) {
                break;
            }
        }

        // Print Bank Logo
        strcpy(logoFileName, record["vaslogo"].c_str());
        printReceiptLogo(logoFileName);
        printReceiptHeader(record[VASDB_DATE].c_str());

        strcpy(buff, record[VASDB_SERVICE].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record["receipt_copy"].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        UPrint_SetDensity(3); //Set print density to 3 normal
        UPrint_SetFont(7, 2, 2);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record[VASDB_STATUS].c_str());
        UPrint_StrBold(buff, 1, 4, 1);
        UPrint_Feed(12);

        printDefault(record);

        printLine("PAYMENT METHOD", record[VASDB_PAYMENT_METHOD].c_str());

        if(!record[VASDB_REF].empty())
        {
            printLine("REF ", record[VASDB_REF].c_str());
        }
        
        if(!record[VASDB_TRANS_SEQ].empty())
        {
            printLine("TRANS SEQ", record[VASDB_TRANS_SEQ].c_str());
        }

        if(!record[VASDB_VIRTUAL_TID].empty())
        {
            printLine("VID", record[VASDB_VIRTUAL_TID].c_str());
        }

        if(mParam.tid[0]) {
            printLine("TID", mParam.tid);
        }

        // "card" is added because requery uses lower case
        if(record[VASDB_PAYMENT_METHOD] == paymentString(PAY_WITH_CARD) || record[VASDB_PAYMENT_METHOD] == "card") {

            const char *keys[] = {DB_NAME, DB_PAN, DB_AID, DB_LABEL, DB_EXPDATE, DB_RRN, DB_AUTHID, DB_RESP};
            const char *labels[] = {"CARD NAME", "PAN", "AID", "LABEL", "EXPIRY", "CREF", "AUTH CODE", "RESP CODE"};

            for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
                if (record.find(keys[i]) != record.end()) {
                    if(*(record[keys[i]].c_str()))
                        printLine(labels[i], record[keys[i]].c_str());
                }
            } 
        }

        memset(buff, '\0', sizeof(buff));
        sprintf(buff, "NGN %s", record[VASDB_AMOUNT].c_str());

        printAsteric(strlen(buff));
        UPrint_StrBold(buff, 1, 4, 1);
        printAsteric(strlen(buff));

        UPrint_Feed(12);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, record[VASDB_STATUS_MESSAGE].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        printDottedLine();
        printVasFooter();

        ret = UPrint_Start();
        if(getPrinterStatus(ret) < 0 ) {
            break;
        }
    }
    

    return ret;
}

int printVasUssdPin(std::map<std::string, std::string> &record, const VAS_Menu_T type)
{
    MerchantData mParam = {'\0'};
    int ret = 0;
    char buff[32] = {'\0'};
    char logoFileName[64] = {'\0'};
    
    std::map<std::string, std::string>::iterator itr;

    for(itr = record.begin(); itr != record.end(); ++itr) {
        printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
    }

    readMerchantData(&mParam);

    while(1) {
        ret = UPrint_Init();

        if (ret == UPRN_OUTOF_PAPER) {
            if (UI_ShowButtonMessage(0, "Print", "No paper", "confirm", UI_DIALOG_TYPE_CONFIRMATION)) {
                break;
            }
        }

        printReceiptHeader(record["VASDB_DATE"].c_str());

        strcpy(buff, record[VASDB_SERVICE].c_str());
        UPrint_StrBold(buff, 1, 4, 1);

        UPrint_SetDensity(3); //Set print density to 3 normal
        
        {

            const char *keys[] = {"batchNo", "hint", "sequenceNo", "product", "description", "pin"};
            const char *labels[] = {"BATCH NO", "HINT", "SEQ NO", "PRODUCT", "DESC", "PIN"};

            for (size_t i = 0; i < sizeof(keys) / sizeof(char*); ++i) {
                if (record.find(keys[i]) != record.end()) {
                    if(*(record[keys[i]].c_str()))
                        printLine(labels[i], record[keys[i]].c_str());
                }
            } 
        }

        memset(buff, '\0', sizeof(buff));
        sprintf(buff, "NGN %s", record["amount"].c_str());

        printAsteric(strlen(buff));
        UPrint_StrBold(buff, 1, 4, 1);
        printAsteric(strlen(buff));

        printDottedLine();
	    UPrint_Feed(100);


        ret = UPrint_Start();
        if(getPrinterStatus(ret) < 0 ) {
            break;
        }
    }
    

    return ret;
}

int printVasEod(std::map<std::string, std::string> &records)
{
    int ret = 0;
    char buff[64] = {'\0'};
    char filename[32] = {'\0'};

    /*
    std::map<std::string, std::string>::iterator itr;

    for(itr = records.begin(); itr != records.end(); ++itr) {
        printf("%s : %s\n", itr->first.c_str(), itr->second.c_str());
    }
    */

    iisys::JSObject json;

    if(!json.load(records["menu"]) || !json.isArray()) {
        return -1;
    }

    while(1) {

        ret = UPrint_Init();
        if (ret == UPRN_OUTOF_PAPER) {
            if(UI_ShowButtonMessage(0, "Print", "No paper \nDo you wish to reload Paper?r", "confirm", UI_DIALOG_TYPE_CONFIRMATION)) {
                break;
            }
        }

        sprintf(filename, "xxxx\\%s", BANKLOGO);
        printReceiptLogo(filename);

        printReceiptHeader(records[VASDB_DATE].c_str());

        UPrint_SetFont(8, 2, 2);
        UPrint_StrBold("SUMMARY", 1, 4, 1);
        printDottedLine();

        UPrint_Feed(12);

        printLine("Approved Amnt", records["approvedAmount"].c_str());
        printLine("Approved Count", records["approvedCount"].c_str());
        printLine("Declined Amnt", records["declinedAmount"].c_str());
        printLine("Declined Count", records["declinedCount"].c_str());
        printLine("Total Count", records["totalCount"].c_str());

        UPrint_Feed(12);

        memset(buff, '\0', sizeof(buff));
        strcpy(buff, records["trxType"].c_str());
        UPrint_StrBold(buff, 1, 4, 1);
        printDottedLine();

        UPrint_Feed(10);

        for(int i = 0; i < json.size(); i++) {

            char leftAlign[32] = {'\0'};
            char rightAlign[32] = {'\0'};
            char amount[14] = {'\0'};
            char time[8] = {'\0'};
            char status[3] = {'\0'};
            char preferredField[24] = {'\0'};
            
            iisys::JSObject& itex = json[i];

            strcpy(amount, itex("amount").getString().c_str());
            strcpy(time, itex("tstamp").getString().c_str());
            strcpy(preferredField, itex("preferredField").getString().c_str());
            strcpy(status, itex("status").getString().c_str());

            sprintf(leftAlign, "%s %s", time, preferredField);
            sprintf(rightAlign, "%s %s", amount, status);
            printLine(leftAlign, rightAlign);

            printDottedLine();

        }

        printVasFooter();

        ret = UPrint_Start();
        if(getPrinterStatus(ret) < 0) {
            break;
        }

    }
    
    return ret;

}

std::string getVasCurrencySymbol()
{
    return "NGN";
    //TODO
    // std::string currency;
    // iisys::JSObject currency = Merchant().object(config::CURRENCY_SYMBOL);

    // if (currency.isNull() || currency.getString().empty()) {
    //     return std::string("NGN");
    // }

    // return currency.getString();

    // return currency;
}

unsigned long getVasAmount(const char* title)
{
    return getAmount(title);
}

PaymentMethod getPaymentMethod(const PaymentMethod preferredMethods)
{
    int selection;
    std::vector<std::string> menu;
    std::vector<PaymentMethod> methods;
    PaymentMethod knownMethods[] = { PAY_WITH_CARD, PAY_WITH_NQR, PAY_WITH_CASH, PAY_WITH_MCASH, PAY_WITH_CGATE };

    for (size_t i = 0; i < sizeof(knownMethods) / sizeof(PaymentMethod); ++i) {
        if (preferredMethods & knownMethods[i]) {
            methods.push_back(knownMethods[i]);
            menu.push_back(paymentString(knownMethods[i]));
        }
    }

    if (methods.size() == 0) {
        return PAYMENT_METHOD_UNKNOWN;
    }

    selection = UI_ShowSelection(30000, "Payment Options", menu, 0);
    if (selection < 0) {
        return PAYMENT_METHOD_UNKNOWN;
    }

    return methods[selection];
}

Service selectService(const char* title, const std::vector<Service>& services)
{
    int selection;
    std::vector<std::string> menu;

    if (services.size() == 0) {
        return SERVICE_UNKNOWN;
    }

    for (size_t i = 0; i < services.size(); ++i) {
        menu.push_back(serviceToString(services[i]));
    }

    selection = UI_ShowSelection(30000, title, menu, 0);
    if (selection < 0) {
        return SERVICE_UNKNOWN;
    }

    return services[selection];
}

VasResult vasResponseCheck(const iisys::JSObject& response)
{
    const iisys::JSObject& messageObj = response("message");
    const iisys::JSObject& responseCodeObj = response("responseCode");

    const std::string message = messageObj.isString() ? messageObj.getString() : "";

    if (!responseCodeObj.isString()) {
        return VasResult(STATUS_ERROR, message.c_str());
    }

    const std::string responseCode = responseCodeObj.getString();

    if (responseCode == VAS_PENDING_RESPONSE_CODE || responseCode == VAS_IN_PROGRESS_RESPONSE_CODE) {
        return VasResult(TXN_PENDING, message.c_str());
    }

    if (responseCode == VAS_TXN_NOT_FOUND_RESPONSE_CODE) {
        return VasResult(TXN_NOT_FOUND, message.c_str());
    }

    if (responseCode == VAS_EXCEPTION_RESPONSE_CODE || responseCode == VAS_STATUS_UNKNOWN_RESPONSE_CODE || responseCode == VAS_SYSTEM_ERROR_RESPONSE_CODE) {
        return VasResult(VAS_STATUS_UNKNOWN, message.c_str());
    }

    if (responseCode != "00") {
        return VasResult(VAS_DECLINED, message.c_str());
    }

    return VasResult(NO_ERRORS, message.c_str());
}

void getVasTransactionReference(std::string& reference, const iisys::JSObject& responseData)
{
    const iisys::JSObject& ref = responseData("reference");
    if (ref.isString()) {
        reference = ref.getString();
    }
}

void getVasTransactionSequence(std::string& sequence, const iisys::JSObject& responseData)
{
    const iisys::JSObject& seq = responseData("sequence");
    if (seq.isString()) {
        sequence = seq.getString();
    }
}

void getVasTransactionDateTime(std::string& dateTime, const iisys::JSObject& responseData)
{
    const iisys::JSObject& date = responseData("date");
    if (date.isString()) {
        dateTime = date.getString() + ".000";
    }
}

void getVasTransactionMessage(std::string& message, const iisys::JSObject& responseData)
{
    const iisys::JSObject& msg = responseData("message");
    if (msg.isString()) {
        message = msg.getString();
    }
}

std::string majorDenomination(unsigned long amount)
{
    char amountStr[16] = { 0 };

    snprintf(amountStr, sizeof(amountStr), "%.2f", amount / 100.0);

    return std::string(amountStr);
}

VasError getVasPin(std::string& pin, const char* message)
{
    switch (getPin(pin, message)) {
    case EMV_CT_PIN_INPUT_OKAY:
        return NO_ERRORS;
    case EMV_CT_PIN_INPUT_ABORT:
        return INPUT_ABORT;
    case EMV_CT_PIN_INPUT_TIMEOUT:
        return INPUT_TIMEOUT_ERROR;
    case EMV_CT_PIN_INPUT_OTHER_ERR:
        return INPUT_ERROR;
        // default: Prefer to get a warning if there are omitted cases
    }

    return INPUT_ERROR;
}

std::string vasApplicationVersion()
{
    std::string appVersion(APP_VER);

    return appVersion;
}

const char* vasChannel()
{
    return "LINUXPOS";
}

std::string getRefCode(const std::string& vasCategory, const std::string& vasProduct)
{
    Payvice payvice;
    char refCode[1024] = {0};
    const std::string terminalId    = vasimpl::getDeviceTerminalId();
    const iisys::JSObject& walletId = payvice.object(Payvice::WALLETID);

    sprintf(refCode, "%s~%s%s%s~%s", 
            walletId.getString().c_str(), 
            vasCategory.c_str(), vasProduct.empty() ? "" : "~",
            vasProduct.c_str(), 
            terminalId.c_str()
    );

    return std::string(refCode);
}

void printVasFooter()
{
	char buff[64] = {'\0'};

	sprintf(buff, "%s %s, %s", APP_NAME, APP_VER, POWERED_BY);
    UPrint_StrBold(buff, 1, 4, 1);
	// UPrint_StrBold(POWERED_BY, 1, 4, 1); 09062774420 09070314-511/443
    // UPrint_StrBold("09062774420 09070314-511/443", 1, 4, 1);
    UPrint_StrBold("09070314511 090703144-19/20", 1, 4, 1);
    // 09070314511, 09070314420, 09070314419
	// UPrint_StrBold("0906 277 4420, 0907 031 4511", 1, 4, 1);
	// UPrint_StrBold("0907 031 4443", 1, 4, 1);
	UPrint_StrBold("support@iisysgroup.com", 1, 4, 1);
	UPrint_StrBold("customercare@iisysgroup.com", 1, 4, 1);
	// UPrint_StrBold("vassupport@iisysgroup.com", 1, 4, 1);
	// UPrint_StrBold("agencybanking@iisysgroup.com", 1, 4, 1);
	UPrint_Feed(108);
}

std::string vasApiKey()
{
#ifdef VAS_TEST
    char key[] = "a6Q6aoHHESonso27xAkzoBQdYFtr9cKC";
#else
    char key[] = "o83prs088n4943231342p7sq53o6502q";
#endif
    rot13(key);
    return std::string(key);
}
