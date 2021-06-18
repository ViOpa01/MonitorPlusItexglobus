#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <ctype.h>
#include <math.h>

#include "platform/platform.h"
#include "payvice.h"
#include "jsonwrapper/jsobject.h"
#include "virtualtid.h"

#include "vasdb.h"

#include "smileViewModel.h"
#include "nqr.h"

SmileViewModel::SmileViewModel(const char* title, VasComProxy& proxy)
    : _title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}

VasResult SmileViewModel::lookup()
{
    VasResult response;
    iisys::JSObject obj;

    if(getLookupJson(obj, service) < 0) {
        return VasResult(VAS_ERROR, "Data Error");
    }

    response = comProxy.lookup("/api/v1/vas/internet/validation", &obj);

    return lookupCheck(response);
}

VasResult SmileViewModel::fetchBundles()
{
    iisys::JSObject obj;

    if (getService() != SMILEBUNDLE) {
        return VasResult(VAS_ERROR);
    }

    if (getLookupJson(obj, service) < 0) {
        return VasResult(VAS_ERROR, "Data Error");
    }

    VasResult response = comProxy.lookup("/api/v1/vas/internet/bundles", &obj);
    if (response.error) {
        return VasResult(response.error, "Bundle lookup failed");
    }

    return bundleCheck(response);
}

VasResult SmileViewModel::bundleCheck(const VasResult& bundleCheck)
{
    iisys::JSObject lookupData;
    VasResult result;

    if (!lookupData.load(bundleCheck.message)) {
        return VasResult(INVALID_JSON, "Invalid Response");
    }

    result = vasResponseCheck(lookupData);
    if (result.error != NO_ERRORS) {
        return result;
    }

    const iisys::JSObject& responseData = lookupData("data");
    const iisys::JSObject& data = responseData("bundles");
    
    if (!data.isArray()) {
        return VasResult(VAS_ERROR, "Bundles not found");
    }

    bundles = data;
    
    return VasResult(NO_ERRORS);
}

int SmileViewModel::getLookupJson(iisys::JSObject& json, Service service) const
{
    json.clear();
    json("service") = apiServiceString(service);
    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();
    json("type") = recipientTypeString(this->recipientType);

    if (recipientType == PHONE_NO) {
        json("account") = this->phoneNumber;
    } else if (recipientType == CUSTOMER_ACC_NO) {
        json("account") = this->customerID;
    } else {
        return -1;
    }

    return 0;
}

VasResult SmileViewModel::lookupCheck(const VasResult& lookStatus)
{
    iisys::JSObject response;
    VasResult result;

    if (lookStatus.error != NO_ERRORS) {
        return VasResult(LOOKUP_ERROR, lookStatus.message.c_str());
    }

    if(!response.load(lookStatus.message)) {
        return VasResult(INVALID_JSON, "Invalid Response");
    }

    result = vasResponseCheck(response);
    if (result.error != NO_ERRORS) {
        return result;
    }

    const iisys::JSObject& responseData = response("data");
    const iisys::JSObject& customerName = responseData("customerName");
    const iisys::JSObject& pcode = responseData("productCode");

    if (!pcode.isString()) {
        result = VasResult(KEY_NOT_FOUND, "Product Code not found");
        return result;
    }

    lookupResponse.productCode = pcode.getString();

    if (!customerName.isString()) {
        result = VasResult(KEY_NOT_FOUND, "Customer name not found");
        return result;
    }

    lookupResponse.customerName = customerName.getString();
    
    return VasResult(NO_ERRORS);
}

VasResult SmileViewModel::initiate(const std::string& pin)
{
    VasResult result;
    
    if (payMethod != PAY_WITH_NQR) {
        result.error = NO_ERRORS;
        return result;
    }

    iisys::JSObject json;

    if (getPaymentJson(json, service) != 0) {
        result.error = VAS_ERROR;
        result.message = "Data Error";
        return result;
    }
    
    json("clientReference") = clientReference = getClientReference(retrievalReference);
    json("pin") = encryptedPin(Payvice(), pin.c_str());

    json = createNqrJSON(getAmount(), "CABLE", json);
    result = comProxy.initiate(nqrGenerateUrl(), &json);

    if (result.error != NO_ERRORS) {
        return result;
    }

    result = parseVasQr(lookupResponse.productCode, result.message);

    return result;
}

VasResult SmileViewModel::complete(const std::string& pin)
{
    iisys::JSObject json;
    VasResult response;

    if (payMethod == PAY_WITH_NQR) {
        json = getNqrPaymentStatusJson(lookupResponse.productCode, clientReference);
    } else {
        if(getPaymentJson(json, service) < 0) {
            response.error = VAS_ERROR;
            response.message = "Data Error";
            return response;
        }

        json("pin") = encryptedPin(Payvice(), pin.c_str());
        json("clientReference") = getClientReference(retrievalReference);
    }


     if (payMethod == PAY_WITH_CARD) {
        cardData = CardData(amount, retrievalReference);
        cardData.refcode = getRefCode(serviceToProductString(service));
        response = comProxy.complete(paymentPath(), &json, &cardData);
    } else {
        response = comProxy.complete(paymentPath(), &json);
    }
    
    if (response.error) {
        return response;
    }

    if (!json.load(response.message)) {
        response.error = INVALID_JSON;
        response.message = "Invalid Response";
        return response;
    }

    response = processPaymentResponse(json);

    return response;
}

int SmileViewModel::getPaymentJson(iisys::JSObject& json, Service service)
{
    std::string paymentMethodStr = apiPaymentMethodString(this->payMethod);

    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();
    json("type") = apiCompleteServiceType(service);
    json("account") = customerID;
    json("phone") = phoneNumber;
    json("productCode") = lookupResponse.productCode;
    json("amount") = majorDenomination(amount);
    json("paymentMethod") = paymentMethodStr;
    json("service") = apiServiceString(service);
    if(service == SMILEBUNDLE) {
        json("code") = this->selectedPackage("code");
    }

    return 0;
}

VasResult SmileViewModel::processPaymentResponse(const iisys::JSObject& json)
{
    VasResult response = vasResponseCheck(json);

    const iisys::JSObject& responseData = json("data");
    if (!responseData.isObject()) {
        return response;
    }

    getVasTransactionDateTime(paymentResponse.date, responseData);
    getVasTransactionReference(paymentResponse.reference, responseData);
    getVasTransactionSequence(paymentResponse.transactionSeq, responseData);

    const iisys::JSObject& description = responseData("description");
    if (description.isString()) {
        paymentResponse.description = description.getString();
    }

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

std::map<std::string, std::string> SmileViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const std::string amountStr = vasimpl::to_string(this->amount);

    if (service == SMILEBUNDLE) {
        const iisys::JSObject& packageName = this->selectedPackage("name");
        if (!packageName.isNull()) {
            record[VASDB_PRODUCT] = packageName.getString(); 
        }
    } else if (service == SMILETOPUP) {
        record[VASDB_PRODUCT] = serviceToProductString(service); 
    }
    
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceToString(service);
    record[VASDB_AMOUNT] = amountStr;


    record[VASDB_BENEFICIARY_NAME] = lookupResponse.customerName;
    if (!customerID.empty()) {
        record[VASDB_BENEFICIARY] = customerID;
    }

    if (!phoneNumber.empty()) {
        record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
    }

    record[VASDB_PAYMENT_METHOD] = paymentString(payMethod);

    record[VASDB_REF] = paymentResponse.reference;
    record[VASDB_DATE] = paymentResponse.date;
    if (!paymentResponse.transactionSeq.empty()) {
        record[VASDB_TRANS_SEQ] = paymentResponse.transactionSeq;
    }

    if (payMethod == PAY_WITH_CARD) {
        if (cardData.primaryIndex > 0) {
            record[VASDB_CARD_ID] = vasimpl::to_string(cardData.primaryIndex);
        }

        if (vasimpl::getDeviceTerminalId() != cardData.transactionTid) {
            record[VASDB_VIRTUAL_TID] = cardData.transactionTid;
        }
    }

    record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::vasErrToTrxStatus(completionStatus.error));

    record[VASDB_STATUS_MESSAGE] = completionStatus.message;

    return record;
}

std::string SmileViewModel::getPhoneNumber()
{
    return phoneNumber;
}

VasResult SmileViewModel::setPhoneNumber(std::string phoneNumber)
{
    this->phoneNumber = phoneNumber;
    if (this->customerID.empty()) {
        this->recipientType = SmileViewModel::PHONE_NO;
    }
    return VasResult(NO_ERRORS);
}

VasResult SmileViewModel::setRecepientType(RecipientType recepientType)
{
    this->recipientType = recepientType;
    return VasResult(NO_ERRORS);
}

VasResult SmileViewModel::setService(Service service)
{
    VasResult result;

    switch(service) {
        case SMILEBUNDLE:
        case SMILETOPUP:
            result.error = NO_ERRORS;
            this->service = service;
            break;
        default:
            break;
    }

    return result;
}

Service SmileViewModel::getService() const
{
    return service;
}

VasResult SmileViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH || payMethod == PAY_WITH_NQR) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }   

    return result;
}

PaymentMethod SmileViewModel::getPaymentMethod() const
{
    return payMethod;
}

const std::string& SmileViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

const char* SmileViewModel::apiServiceString(Service service) const
{
    (void) service;
    return "smile";
}

const char* SmileViewModel::recipientTypeString(RecipientType recipient) const
{
    switch (recipient) {
        case PHONE_NO:
            return "phone";
        case CUSTOMER_ACC_NO:
            return "account";    
        default:
            return "";
    }
}

const char* SmileViewModel::apiCompleteServiceType(Service service) const
{
    switch (service)
    {
        case SMILETOPUP:
            return "topup";
        case SMILEBUNDLE:
            return "subscription";    
        default:
            return "";
    }
}

std::string SmileViewModel::getCustomerID()
{
    return customerID;
}

const iisys::JSObject& SmileViewModel::getBundles() const
{
    return bundles;
}

VasResult SmileViewModel::setCustomerID(std::string customerID)
{
    this->customerID = customerID;
    this->recipientType = SmileViewModel::CUSTOMER_ACC_NO;
    return VasResult(NO_ERRORS);
}

unsigned long SmileViewModel::getAmount() const
{
    return amount;
}

VasResult SmileViewModel::setAmount(unsigned long amount)
{    
    this->amount = amount;
    return VasResult(NO_ERRORS);
}

const char* SmileViewModel::paymentPath()
{
    if (getPaymentMethod() == PAY_WITH_NQR) {
        return nqrStatusCheckUrl();
    } else {
        return "/api/v1/vas/internet/subscription";
    }
}

VasResult SmileViewModel::setSelectedPackageIndex(int index)
{
    VasResult result;

    if (getService() != SMILEBUNDLE || index < 0 || (size_t)index >= bundles.size()) {
        return result;
    }

    result.error = NO_ERRORS;
    this->selectedPackage = bundles[index];
    this->amount = (unsigned long) lround(this->selectedPackage("price").getNumber() * 100.0);

    return result;
}
