#ifndef VAS_ELECTRICITY_H
#define VAS_ELECTRICITY_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsobject.h"

int electricity(const char* title);

struct Electricity : FlowDelegate {

    typedef enum {
        TYPE_UNKNOWN,
        GENERIC_ENERGY,
        PREPAID_TOKEN,
        PREPAID_SMARTCARD,
        POSTPAID
    } EnergyType;

    Electricity(const char* title, VasComProxy& proxy);

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


    std::string meterNo;
    std::string phoneNumber;
    PaymentMethod payMethod;
    EnergyType energyType;
    long long amount;

    CardPurchase cardPurchase;

    struct {
        std::string name;
        std::string address;
        std::string productCode;
        unsigned long minPayableAmount;
    } lookupResponse;

    struct {
        std::string message;
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string serviceData;
    } paymentResponse;

    VasStatus lookupCheck(const VasStatus& lookupStatus);
    EnergyType getEnergyType(Service service, const char * title);
    const char* payloadServiceTypeString(EnergyType energyType);
    
    std::string getMeterNo(const char* prompt);
    const char* abujaMeterType(EnergyType energyType);

    const char* productString(EnergyType type);

    int getLookupJson(iisys::JSObject& json, Service service);
    int getPaymentJson(iisys::JSObject& json, Service service);


    const char* paymentPath(Service service);

};

#endif
