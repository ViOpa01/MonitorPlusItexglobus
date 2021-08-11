#ifndef DATA_USSD_VIEW_MODEL
#define DATA_USSD_VIEW_MODEL

#include "vas.h"
#include "vascomproxy.h"

struct DataUssdViewModel {

    DataUssdViewModel(const char* title, VasComProxy& proxy);
    
    VasResult packageLookup();
    VasResult lookup();
    VasResult initiate(const std::string& pin);
    VasResult complete(const std::string& pin);
    const char* apiServiceString(Service service) const;
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setUnits(const int units);
    VasResult setService(Service service);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setSelectedPackageIndex(int selectedPackageIndex);
    const iisys::JSObject& getPackages() const;

    int getUnits() const;
    Service getService() const;
    unsigned long getAmount() const;
    PaymentMethod getPaymentMethod() const;
    const std::string& getRetrievalReference() const;


private:
    std::string _title; 
    VasComProxy& comProxy;
    Service service;

    PaymentMethod payMethod;

    CardData cardData;
    std::string clientReference;
    std::string retrievalReference;
    
    iisys::JSObject selectedPackage;
    unsigned long amount;

    int units;
    std::string productCode;

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

    int getLookupJson(iisys::JSObject& json) const;
    int getPackageLookupJson(iisys::JSObject& json) const;

    VasResult packageLookupCheck(const VasResult& lookupStatus);
    const char* paymentPath();
};


#endif