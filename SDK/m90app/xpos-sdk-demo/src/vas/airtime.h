#ifndef VAS_AIRTIME_H
#define VAS_AIRTIME_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsobject.h"


struct Airtime : FlowDelegate {

    typedef enum {
        AIRTEL,
        ETISALAT,
        GLO,
        MTN
    } Network;

    Airtime(const char* title, VasComProxy& proxy);

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
    PaymentMethod payMethod;
    long long amount;

    CardPurchase cardPurchase;

    struct {
        std::string message;
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string serviceData;
    } paymentResponse;

    Service getAirtimeService(Network network);
    std::string networkString(Network network);

    int getPaymentJson(iisys::JSObject& json, Service service);


    const char* paymentPath(Service service);

};

#endif
