#ifndef SMILE_VIEW_MODEL
#define SMILE_VIEW_MODEL

#include "vas.h"
#include "vascomproxy.h"

struct SmileViewModel {

    enum RecipientType {
        RECIPIENT_TYPE_NOT_SET,
        PHONE_NO,
        CUSTOMER_ACC_NO
    };

    SmileViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup(); 
    VasResult initiate(const std::string& pin);
    VasResult complete(const std::string& pin);
    VasResult fetchBundles();
    const char* apiServiceString(Service service) const;
    const char* recipientTypeString(RecipientType recipient) const;
    const char* apiCompleteServiceType(Service service) const;
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setCustomerID(std::string customerID);
    VasResult setPhoneNumber(std::string phoneNumber);
    VasResult setAmount(unsigned long amount);
    VasResult setService(Service service);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setRecepientType(RecipientType recepientType);
    VasResult setSelectedPackageIndex(int index);

    std::string getCustomerID();
    unsigned long getAmount() const;
    const iisys::JSObject& getBundles() const;
    std::string getPhoneNumber();
    Service getService() const;
    PaymentMethod getPaymentMethod() const;
    const std::string& getRetrievalReference() const;
    
    struct {
        std::string productCode;
        std::string error;
        std::string customerName;
        std::string responseCode;
        std::string description;
        std::string message;
        std::string bundles;
    } lookupResponse;

private:
    std::string _title;
    VasComProxy& comProxy;
    Service service;

    std::string customerID;
    std::string phoneNumber;
    iisys::JSObject bundles;
    iisys::JSObject selectedPackage;
    PaymentMethod payMethod;

    RecipientType recipientType;

    CardData cardData;
    std::string clientReference;
    std::string retrievalReference;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string bundle;
        std::string description;
    } paymentResponse;

    int getLookupJson(iisys::JSObject& json, Service service) const;
    int getPaymentJson(iisys::JSObject& json, Service service);
    VasResult bundleCheck(const VasResult& bundleCheck);

    unsigned long amount;
    VasResult lookupCheck(const VasResult& lookStatus);
    const char* paymentPath();


};

#endif