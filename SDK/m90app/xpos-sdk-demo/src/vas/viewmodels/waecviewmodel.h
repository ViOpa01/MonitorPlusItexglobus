#ifndef VAS_WAEC_VIEW_MODEL_H
#define VAS_WAEC_VIEW_MODEL_H

#include "vas.h"
#include "vascomproxy.h"

#include "jsonwrapper/jsobject.h"

struct WaecViewModel {

    WaecViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult complete(const std::string& pin);

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setService(Service service);
    VasResult setAmount(unsigned long amount);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setEmail(const std::string& email);
    VasResult setLastName(const std::string& surname);
    VasResult setFirstName(const std::string& firstName);
    VasResult setPhoneNumber(const std::string& phoneNumber);

    Service getService() const;

    unsigned long getAmount() const;
    const std::string& getRetrievalReference() const;

    PaymentMethod getPaymentMethod() const;

    static VasResult
    requery(VasComProxy& comProxy, const std::string& phoneNumber, const std::string& confirmationNumber, const Service service);

private:
    std::string _title;
    Service service;
    VasComProxy& comProxy;

    PaymentMethod payMethod;
    CardData cardData;

    std::string retrievalReference;

    struct {
        std::string productCode;
        unsigned long amount;
    } lookupResponse;

    struct {
        std::string email;
        std::string lastName;
        std::string firstName;
        std::string phoneNumber;
    } userData;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string pin;
        std::string token;
    } paymentResponse;

    VasResult lookupCheck(const VasResult& lookupStatus);
    VasResult extractFirstStageInfo(const iisys::JSObject& data);
    const char* paymentPath();
    int getPaymentJson(iisys::JSObject& json) const;

    static const char* apiServiceString(Service service);
};

#endif
