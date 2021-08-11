#ifndef ELECTRICITY_VIEW_MODEL
#define ELECTRICITY_VIEW_MODEL

#include "vas.h"
#include "vascomproxy.h"
#include "../../CpuCard_Fun.h"
#include "../../unistarwrapper.h"


struct ElectricityViewModel {

    typedef enum {
        TYPE_UNKNOWN,
        GENERIC_ENERGY,
        PREPAID_TOKEN,
        PREPAID_SMARTCARD,
        POSTPAID
    } EnergyType;

    ElectricityViewModel(const char* title, VasComProxy& proxy);

    VasResult lookup();
    VasResult initiate(const std::string& pin);
    VasResult complete(const std::string& pin);
    const char* apiServiceString(Service service) const;
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    VasResult processPaymentResponse(const iisys::JSObject& json);
    
    VasResult setService(Service service);
    VasResult setAmount(unsigned long ammount);
    VasResult setPaymentMethod(PaymentMethod payMethod);
    VasResult setPhoneNumber(const std::string& phoneNumber);
    VasResult setEnergyType(EnergyType energyType);
    VasResult setMeterNo(const std::string& meterNo);

    static VasResult revalidateSmartCard(const iisys::JSObject& data);


    Service getService() const;
    unsigned long getAmount() const;
    PaymentMethod getPaymentMethod() const;
    EnergyType getEnergyType() const;

    const std::string& getRetrievalReference() const;

    struct {
        std::string name;
        std::string address;
        std::string productCode;
        unsigned long minPayableAmount;
        unsigned long arrears;
    } lookupResponse;


    VasResult updateSmartCard(La_Card_info * userCardInfo);
    int readSmartCard(La_Card_info * userCardInfo);
    SmartCardInFunc& getSmartCardInFunc();
    La_Card_info& getUserCardInfo();
    const unsigned char* getUserCardNo() const;

private:
    std::string title;
    VasComProxy& comProxy;
    Service service;

    std::string meterNo;
    std::string phoneNumber;
    PaymentMethod payMethod;
    EnergyType energyType;

    CardData cardData;
    std::string clientReference;
    std::string retrievalReference;

    struct {
        std::string reference;
        std::string transactionSeq;
        std::string date;
        std::string serviceData;
    } paymentResponse;

    std::string getMeterNo(const char* prompt);
    const char* productString(EnergyType type);

    int getLookupJson(iisys::JSObject& json, Service service) const;
    int getPaymentJson(iisys::JSObject& json, Service service) const;

    void addToken(iisys::JSObject& serviceData, const iisys::JSObject& responseData) const;

    unsigned long amount;
    VasResult lookupCheck(const VasResult& lookupStatus);
    const char* paymentPath();
    const char* getEnergyTypeString(EnergyType type) const;

    SmartCardInFunc smartCardInFunc;
    La_Card_info userCardInfo;
    int purchaseTimes;

};


enum ResponseCode
{

};

#endif
