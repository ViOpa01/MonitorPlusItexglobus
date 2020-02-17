#ifndef VAS_PAYTV_H
#define VAS_PAYTV_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsobject.h"


struct PayTV : FlowDelegate {

    PayTV(const char* title, VasComProxy& proxy);

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

    std::string phoneNumber;
    std::string iuc;
    PaymentMethod payMethod;
    unsigned long amount;

    CardPurchase cardPurchase;

    struct {
        std::string name;
        std::string unit;
        iisys::JSObject selectedPackage;
    } multichoice;

    struct {
        std::string name;
        std::string bouquet;
        std::string balance;
        std::string productCode;
    } startimes;

    struct {
        std::string message;
        std::string reference;
        std::string transactionSeq;
        std::string date;
    } paymentResponse;

    VasStatus startimesLookupCheck(const VasStatus& lookupStatus);
    VasStatus multichoiceLookupCheck(const VasStatus& lookupStatus);
    int getPaymentJson(iisys::JSObject& json, Service service);


    const char* paymentPath(Service service);

};

#endif
