#ifndef VAS_CASHIO_VIEW_MODEL_H
#define VAS_CASHIO_VIEW_MODEL_H

#include "vas.h"
#include "vascomproxy.h"

#include "jsonwrapper/jsobject.h"


struct ViceBankingViewModel {

    struct Bank {
        std::string name;
        std::string code;
        Bank(const std::string& _name, const std::string& _code)
        : name(_name), code(_code)
        {
        }
    };

    ViceBankingViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult initiate(const std::string& pin);
    VasResult complete(const std::string& pin);

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);

    VasResult setService(Service service);
    VasResult setAmount(unsigned long ammount);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setPhoneNumber(const std::string& phoneNumber);
    VasResult setBeneficiary(const std::string& account, const int bankIndex);
    VasResult setNarration(const std::string& narration);

    Service getService() const;
    unsigned long getAmount() const;

    const std::string& getName()const;
    const std::string& getBeneficiaryAccount() const;
    unsigned long getAmountSettled() const;
    unsigned long getAmountToDebit() const;
    const std::string& getRetrievalReference() const;

    PaymentMethod getPaymentMethod() const;
    const std::vector<Bank>& banklist() const;

    bool isPaymentUSSD() const;

    
private:
    std::string _title;
    Service service;
    VasComProxy& comProxy;

    unsigned long amount;
    std::string beneficiary;
    int beneficiaryBankIndex;

    std::string phoneNumber;

    PaymentMethod payMethod;
    CardData cardData;

    std::string narration;
    std::string clientReference;
    std::string retrievalReference;

    static std::vector<Bank> banks;

    struct {
        std::string name;
        std::string account;
        std::string productCode;
        unsigned long amountSettled;
        unsigned long amountToDebit;
        unsigned long thresholdAmount;
    } lookupResponse;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
    } paymentResponse;

    VasResult lookupCheck(const VasResult& lookupStatus);
    VasResult extractLookupInfo(const iisys::JSObject& data);
    const char* paymentPath(Service service);
    int getPaymentJson(iisys::JSObject& json) const;
    VasError fetchBankList(std::vector<Bank>& bankVec) const;

    const char* apiServiceString(Service service) const;
};

#endif
