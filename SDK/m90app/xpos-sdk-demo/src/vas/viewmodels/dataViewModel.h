#ifndef DATA_VIEW_MODEL
#define DATA_VIEW_MODEL

#include "vas.h"
#include "vascomproxy.h"

struct DataViewModel {

    DataViewModel(const char* title, VasComProxy& proxy);
    
    VasResult lookup();
    VasResult complete(const std::string& pin);
    const char* apiServiceString(Service service) const;
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setService(Service service);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setPhoneNumber(const std::string& phoneNumber);
    VasResult setSelectedPackageIndex(int selectedPackageIndex);
    const iisys::JSObject& getPackages() const;

  
    Service getService() const;
    PaymentMethod getPaymentMethod() const;


private:
    std::string _title; 
    VasComProxy& comProxy;
    Service service;

    std::string phoneNumber;
    PaymentMethod payMethod;

    CardData cardData;
    iisys::JSObject selectedPackage;
    unsigned long amount;

    struct {
        iisys::JSObject packages;
        std::string productCode;
    } lookupResponse;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string description;
        std::string serviceData;
    } paymentResponse;

    int getLookupJson(iisys::JSObject& json, Service service) const;
    int getPaymentJson(iisys::JSObject& json, Service service);

    VasResult lookupCheck(const VasResult& lookupStatus);
    const char* paymentPath();
};


#endif