#ifndef AIRTIME_VIEW_DATA
#define AIRTIME_VIEW_DATA

#include "vas.h"
#include "vascomproxy.h"


struct AirtimeViewModel {

    AirtimeViewModel(const char* title, VasComProxy& proxy);

    VasResult initiate(const std::string& pin);
    VasResult complete(const std::string& pin);

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);
    
    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setService(Service service);
    VasResult setAmount(unsigned long amount);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setPhoneNumber(const std::string& phoneNumber);

    Service getService() const;
    PaymentMethod getPaymentMethod() const;
    const std::string& getRetrievalReference() const;

private:
    std::string title;
    VasComProxy& comProxy;

    CardData cardData;
    std::string productCode;
    std::string clientReference;
    std::string retrievalReference;

    Service service;
    std::string phoneNumber;
    PaymentMethod payMethod;
    unsigned long amount;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string serviceData;
    } paymentResponse;

    std::string apiServiceString(Service service) const;

    int getPaymentJson(iisys::JSObject& json) const;
    const char* paymentPath() const;

};

#endif
