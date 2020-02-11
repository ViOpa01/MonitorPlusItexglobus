#ifndef VAS_CASHIO_H
#define VAS_CASHIO_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsobject.h"


struct ViceBanking : FlowDelegate {

    typedef enum {
        BANKUNKNOWN = -1,
        GTB,
        ACCESS,
        CITI,
        DIAMOND,
        ECOBANK,
        ENTERPRISE,
        ETB,
        FCMB,
        FIDELITY,
        FIRSTBANK,
        HERITAGE,
        JAIZ,
        KEYSTONE,
        MAINSTREET,
        NIB,
        POLARIS,
        STANBIC,
        STANDARDCHARTERED,
        STERLING,
        UBA,
        UNION,
        UNITY,
        WEMA,
        ZENITH,
        BANK_TOTAL
    } Bank_t;

    struct Bank {
        char name[32];
        char code[32];
        Bank(const char* _name, const char* _code)
        {
            strncpy(name, _name, sizeof(name));
            strncpy(code, _code, sizeof(code));
        }
    };

    ViceBanking(const char* title, VasComProxy& proxy);

    VasStatus beginVas();
    VasStatus lookup(const VasStatus&);
    VasStatus initiate(const VasStatus&);
    VasStatus complete(const VasStatus&);

    Service vasServiceType();
    std::map<std::string, std::string> storageMap(const VasStatus& completionStatus);

    VasStatus processPaymentResponse(iisys::JSObject& json, Service service);
    
protected:
    std::string _title;
    Service service;
    VasComProxy& comProxy;

    unsigned long amount;
    std::string beneficiary;
    std::string phoneNumber;
    std::string vendorBankCode;
    std::string bankName;

    PaymentMethod payMethod;
    CardPurchase cardPurchase;

    struct {
        std::string name;
        std::string productCode;
        unsigned long convenience;
        unsigned long amountSettled;
        unsigned long amountToDebit;
    } lookupResponse;

    struct {
        std::string message;
        std::string reference;
        std::string transactionSeq;
        std::string date;
    } paymentResponse;

    Bank bankCode(Bank_t);
    Bank_t selectBank();
    std::string beneficiaryAccount(const char *title, const char* prompt);
    VasStatus lookupCheck(const VasStatus& lookupStatus);
    VasStatus displayLookupInfo(const iisys::JSObject& data);
    const char* paymentPath(Service service);

    int getPaymentJson(iisys::JSObject& json, Service service);
};

#endif
