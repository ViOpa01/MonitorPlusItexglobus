#ifndef VAS_JAMB_VIEW_MODEL_H
#define VAS_JAMB_VIEW_MODEL_H

#include "vas.h"
#include "vascomproxy.h"

#include "jsonwrapper/jsobject.h"


struct JambViewModel {

    JambViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult complete(const std::string& pin);

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setService(Service service);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setEmail(const std::string& email);
    VasResult setConfirmationCode(const std::string& code);
    VasResult setPhoneNumber(const std::string& phoneNumber);

    Service getService() const;
    unsigned long getAmount() const;

    const std::string& getSurname()const;
    const std::string& getMiddleName()const;
    const std::string& getFirstName()const;
    const std::string& getConfirmationCode() const;
    const std::string& getRetrievalReference() const;

    PaymentMethod getPaymentMethod() const;

    static VasResult
    requery(VasComProxy& comProxy, const std::string& phoneNumber, const std::string& confirmationNumber, const Service service);
    
private:
    std::string _title;
    Service service;
    VasComProxy& comProxy;

    std::string email;
    std::string confirmationCode;
    std::string phoneNumber;

    PaymentMethod payMethod;
    CardData cardData;

    std::string retrievalReference;

    struct {
        std::string type;
        std::string product;
        std::string productCode;

        std::string surname;
        std::string firstName;
        std::string middleName;
        unsigned long amount;
    } lookupResponse;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string pin;
    } paymentResponse;

    VasResult lookupCheck(const VasResult& lookupStatus, const int stage);
    VasResult extractFirstStageInfo(const iisys::JSObject& data);
    VasResult extractSecondStageInfo(const iisys::JSObject& data);
    const char* paymentPath(Service service);
    int getPaymentJson(iisys::JSObject& json) const;

    static const char* apiServiceString(Service service);
};

#endif
