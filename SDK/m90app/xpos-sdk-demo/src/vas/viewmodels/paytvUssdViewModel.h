#ifndef PAYTV_USSD_VIEW_MODEL
#define PAYTV_USSD_VIEW_MODEL

#include "vas.h"
#include "vascomproxy.h"

struct PaytvUssdViewModel {

    PaytvUssdViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult complete(const std::string& pin);
    const char* apiServiceString(Service service) const;
    const char* apiServiceType(Service service) const;
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setUnits(const int units);
    VasResult setService(Service service);
    VasResult setAmount(unsigned long ammount);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setSelectedPackage(int selectedPackageIndex, const std::string& cycle);

    int getUnits() const;
    Service getService() const;
    VasResult bouquetLookup();
    unsigned long getAmount() const;
    PaymentMethod getPaymentMethod() const;

    const iisys::JSObject& getBouquets() const;
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
    iisys::JSObject bouquets;
    iisys::JSObject selectedPackage;

    int units;
    std::string cycle;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string serviceData;
    } paymentResponse;

    int getLookupJson(iisys::JSObject& json) const;
    VasResult bouquetLookupCheck(const VasResult& lookupStatus);

    unsigned long amount;
    const char* paymentPath();
};

#endif
