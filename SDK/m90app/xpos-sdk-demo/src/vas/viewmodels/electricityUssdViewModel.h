#ifndef ELECTRICITY_USSD_VIEW_MODEL
#define ELECTRICITY_USSD_VIEW_MODEL

#include "vas.h"
#include "vascomproxy.h"


struct ElectricityUssdViewModel {

    ElectricityUssdViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult complete(const std::string& pin);
    const char* apiServiceString(Service service) const;
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);
    
    VasResult setUnits(const int units);
    VasResult setService(Service service);
    VasResult setAmount(unsigned long ammount);
    VasResult setPaymentMethod(PaymentMethod payMethod);

    int getUnits() const;
    Service getService() const;
    unsigned long getAmount() const;
    PaymentMethod getPaymentMethod() const;

    const std::string& getRetrievalReference() const;

private:
    std::string title;
    VasComProxy& comProxy;
    Service service;

    PaymentMethod payMethod;


    CardData cardData;
    std::string clientReference;
    std::string retrievalReference;
    std::string productCode;

    int units;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string serviceData;
    } paymentResponse;

    int getLookupJson(iisys::JSObject& json) const;

    unsigned long amount;
    const char* paymentPath();

};


#endif
