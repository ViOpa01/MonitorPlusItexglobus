#ifndef AIRTIME_USSD_VIEW_MODEL_H
#define AIRTIME_USSD_VIEW_MODEL_H

#include "vas.h"
#include "vascomproxy.h"


struct AirtimeUssdViewModel {

    AirtimeUssdViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult complete(const std::string& pin);

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);
    
    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setUnits(const int units);
    VasResult setAmount(unsigned long amount);
    VasResult setPaymentMethod(PaymentMethod payMethod);

    int getUnits() const;
    Service getService() const;
    unsigned long getAmount() const;
    PaymentMethod getPaymentMethod() const;
    const std::string& getRetrievalReference() const;

private:
    std::string title;
    VasComProxy& comProxy;

    CardData cardData;
    std::string productCode;
    std::string clientReference;
    std::string retrievalReference;

    const Service service;
    PaymentMethod payMethod;
    unsigned long amount;
    int units;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string serviceData;
    } paymentResponse;

    int getLookupJson(iisys::JSObject& json) const;
    const char* paymentPath() const;

};

#endif
