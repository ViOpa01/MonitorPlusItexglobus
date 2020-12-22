#ifndef PAYTV_VIEW_MODEL
#define PAYTV_VIEW_MODEL

#include "vas.h"
#include "vascomproxy.h"

struct PayTVViewModel {

    typedef enum{
        UNKNOWN_TYPE,
        STARTIMES_BUNDLE,
        STARTIMES_TOPUP
    } StartimesType;

    PayTVViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult complete(const std::string& pin);
    std::map<std::string, std::string>storageMap(const VasResult& completionStatus);

    const char* apiServiceString(Service service) const;
    const char* apiServiceType(Service service) const;
    
    VasResult setAmount(unsigned long amount);
    VasResult setPhoneNumber(const std::string& phoneNumber);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setService(Service service);
    VasResult setStartimesType(StartimesType startimestype);
    VasResult setIUC(const std::string& iuc);

    VasResult setSelectedPackage(int selectedPackageIndex, const std::string& cycle = "");
    
    VasResult processPaymentResponse(const iisys::JSObject& json, Service service);    
    
    unsigned long getAmount() const;
    std::string getIUC() const;
    const iisys::JSObject& getBouquets() const;
    Service getService() const;
    StartimesType getStartimesTypes() const;

    struct {
        std::string name;
        std::string unit;
        std::string bouquet;
        std::string balance;
        std::string accountNo;
        std::string productCode;
    } lookupResponse;

private:
    std::string _title;
    VasComProxy& comProxy;
    Service service;

    std::string phoneNumber;
    std::string iuc;
    PaymentMethod payMethod;
    unsigned long amount;
    StartimesType startimestype;

    CardData cardData;

    iisys::JSObject selectedPackage;
    std::string cycle;
    iisys::JSObject bouquets;
    
    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
    } paymentResponse;

    int getLookupJson(iisys::JSObject& json, Service service) const;
    int getPaymentJson(iisys::JSObject& json, Service service) const;

    const char* paymentPath();
    const char* serviceTypeString(StartimesType type);
    VasResult lookupCheck(const VasResult& lookupStatus);
};

#endif
