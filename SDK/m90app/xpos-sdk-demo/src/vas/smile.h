#ifndef VAS_SMILE_H
#define VAS_SMILE_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsobject.h"


struct Smile : FlowDelegate {

    Smile(const char* title, VasComProxy& proxy);

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

    std::string customerID;
    std::string phoneNumber;
    PaymentMethod payMethod;
    long long amount;

    CardPurchase cardPurchase;

    struct {
        std::string name;
        iisys::JSObject selectedPackage;
    } lookupResponse;

    struct {
        std::string message;
        std::string reference;
        std::string transactionSeq;
        std::string date;
    } paymentResponse;

    VasStatus bundleCheck(const VasStatus& bundleCheck);
    VasStatus displayLookupInfo(const iisys::JSObject& data);
    int getPaymentJson(iisys::JSObject& json, Service service);


    const char* paymentPath(Service service);

};

#endif
