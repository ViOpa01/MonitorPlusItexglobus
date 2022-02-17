#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <ctype.h>
#include <math.h>

#include "platform/platform.h"

#include "vasdb.h"
#include "payvice.h"

#include "jambviewmodel.h"


JambViewModel::JambViewModel(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
    , payMethod(PAYMENT_METHOD_UNKNOWN)
{
    lookupResponse.amount = 0;
}

VasResult JambViewModel::lookupCheck(const VasResult& lookupStatus, const int stage)
{
    iisys::JSObject json;

    if (lookupStatus.error != NO_ERRORS) {
        return VasResult(LOOKUP_ERROR, lookupStatus.message.c_str());
    }

    if (!json.load(lookupStatus.message)) {
        return VasResult(INVALID_JSON, "Invalid Response");
    }

    VasResult errStatus = vasResponseCheck(json);
    if (errStatus.error != NO_ERRORS) {
        return VasResult(errStatus.error, errStatus.message.c_str());
    }

    const iisys::JSObject& data = json("data");
    if (!data.isObject()) {
        return VasResult(INVALID_JSON);
    }

    if (stage == 1) {
        return extractFirstStageInfo(data);
    } else if (stage == 2) {
        return extractSecondStageInfo(data);
    }
    
    return VasResult(LOOKUP_ERROR, "Stage Error");
}

VasResult JambViewModel::lookup()
{
    iisys::JSObject obj;

    obj("channel") = vasChannel();
    obj("version") = vasApplicationVersion();
    obj("service") = apiServiceString(getService());

    VasResult response = comProxy.lookup("/api/v1/jamb/service/lookup", &obj);

    response = lookupCheck(response, 1);
    if (response.error != NO_ERRORS) {
        response.message = "Error fetching selling price";
    }

    obj("confirmationCode") = getConfirmationCode();
    obj("customerPhoneNumber") = phoneNumber;
    
    response = comProxy.lookup("/api/v1/jamb/candidate/validate", &obj);

    return lookupCheck(response, 2);
}

Service JambViewModel::getService() const
{
    return service;
}

unsigned long JambViewModel::getAmount() const
{
    return lookupResponse.amount;
}

const std::string& JambViewModel::getSurname() const
{
    return lookupResponse.surname;
}

const std::string& JambViewModel::getMiddleName() const
{
    return lookupResponse.middleName;
}

const std::string& JambViewModel::getFirstName() const
{
    return lookupResponse.firstName;
}

const std::string& JambViewModel::getConfirmationCode() const
{
    return confirmationCode;
}

const std::string& JambViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

PaymentMethod JambViewModel::getPaymentMethod() const
{
    return payMethod;
}

VasResult JambViewModel::setService(Service service)
{
    VasResult result;

    if (service != JAMB_UTME_PIN && service != JAMB_DE_PIN) {
        return result;
    }

    this->service = service;
    result.error = NO_ERRORS;
    return result;
}

VasResult JambViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod != PAY_WITH_CARD && payMethod != PAY_WITH_CASH) {
        return result;
    }
    
    this->payMethod = payMethod;

    result.error = NO_ERRORS;
    return result;
}

VasResult JambViewModel::setEmail(const std::string& email)
{
    VasResult result;

    if (email.empty()) {
        return result;
    }

    this->email = email;

    result.error = NO_ERRORS;
    return result;
}

VasResult JambViewModel::setPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (phoneNumber.empty()) {
        return result;
    }

    this->phoneNumber = phoneNumber;
    result.error = NO_ERRORS;
    return result;
}

VasResult JambViewModel::setConfirmationCode(const std::string& code)
{
    VasResult result;

    if (code.empty()) {
        return result;
    }

    this->confirmationCode = code;
    result.error = NO_ERRORS;
    return result;
}

VasResult JambViewModel::complete(const std::string& pin)
{
    VasResult response;
    iisys::JSObject json;

    if (getPaymentJson(json) < 0) {
        response.error = VAS_ERROR;
        response.message = "Data Error";
        return response;
    }

    json("clientReference") = getClientReference(retrievalReference);
    json("pin") = encryptedPin(Payvice(), pin.c_str());

    if (payMethod == PAY_WITH_CARD) {
        cardData = CardData(lookupResponse.amount, retrievalReference);
        cardData.refcode = getRefCode(serviceToProductString(service));
        response = comProxy.complete(paymentPath(service), &json, &cardData);
    } else {
        response = comProxy.complete(paymentPath(service), &json);
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

std::map<std::string, std::string> JambViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const Service service = getService();
    std::string serviceStr(serviceToString(service));
    const std::string amountStr = vasimpl::to_string(lookupResponse.amount);

    record[VASDB_PRODUCT] = serviceStr; 
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceStr;
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY] = getConfirmationCode();
    record[VASDB_BENEFICIARY_NAME] = lookupResponse.firstName + " " + lookupResponse.middleName + " " + lookupResponse.surname;
    record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
    record[VASDB_PAYMENT_METHOD] = paymentString(payMethod);

    record[VASDB_REF] = paymentResponse.reference;
    record[VASDB_DATE] = paymentResponse.date;
    record[VASDB_TRANS_SEQ] = paymentResponse.transactionSeq;

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
    record[VASDB_SERVICE_DATA] = std::string("{\"pin\": \"") + paymentResponse.pin + std::string("\",\"email\": \"") + this->email + "\"}";

    return record;
}

const char* JambViewModel::paymentPath(Service service)
{
    return "/api/v1/jamb/pin/purchase";
}

VasResult JambViewModel::processPaymentResponse(const iisys::JSObject& json)
{
    VasResult response = vasResponseCheck(json);

    const iisys::JSObject& responseData = json("data");
    if (!responseData.isObject()) {
        return response;
    }

    getVasTransactionDateTime(paymentResponse.date, responseData);
    getVasTransactionReference(paymentResponse.reference, responseData);
    getVasTransactionSequence(paymentResponse.transactionSeq, responseData);

    const iisys::JSObject& detail = responseData("detail");

    if (detail.isObject()) {
        const iisys::JSObject& pin         = detail("PIN");
        const iisys::JSObject& surname     = detail("Surname");
        const iisys::JSObject& firstName   = detail("FirstName");
        const iisys::JSObject& middleName  = detail("MiddleName");
        const iisys::JSObject& amount      = detail("Amount");
        const iisys::JSObject& phoneNumberObj = detail("GSMNo");

        if (pin.isString()) {
            paymentResponse.pin = pin.getString();
        }

        if (lookupResponse.surname.empty() && surname.isString()) {
            lookupResponse.surname = surname.getString();
        }

        if (lookupResponse.firstName.empty() && firstName.isString()) {
            lookupResponse.firstName = firstName.getString();
        }

        if (lookupResponse.middleName.empty() && middleName.isString()) {
            lookupResponse.middleName = middleName.getString();
        }

        if (phoneNumber.empty() && phoneNumberObj.isString()) {
            phoneNumber = phoneNumberObj.getString();
        }
    }

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

VasResult JambViewModel::extractFirstStageInfo(const iisys::JSObject& data)
{
    const iisys::JSObject& jambdata             = data("data");
    const iisys::JSObject& productCode          = data("productCode");

    if (!jambdata.isObject() || !productCode.isString()) {
        return VasResult(TYPE_MISMATCH);
    }

    const iisys::JSObject& type                 = jambdata("type");
    const iisys::JSObject& product              = jambdata("product");
    const iisys::JSObject& sellingPrice         = jambdata("sellingPrice");
    
    if (!type.isString() || !product.isString()) {
        return VasResult(TYPE_MISMATCH);
    }

    lookupResponse.type = type.getString();
    lookupResponse.product = product.getString();
    lookupResponse.productCode = productCode.getString();
    if (lookupResponse.amount == 0 && sellingPrice.isString()) {
        lookupResponse.amount = (unsigned long) lround(sellingPrice.getNumber() * 100.0);
    }

    return VasResult(NO_ERRORS);
}
    
VasResult JambViewModel::extractSecondStageInfo(const iisys::JSObject& data)
{
    const iisys::JSObject& detail             = data("detail");

    if (!detail.isObject()) {
        return VasResult(TYPE_MISMATCH);
    }

    const iisys::JSObject& surname            = detail("Surname");
    const iisys::JSObject& firstName          = detail("FirstName");
    const iisys::JSObject& middleName         = detail("MiddleName");
    const iisys::JSObject& phoneNo            = detail("GSMNo");

    if (!surname.isString() || !firstName.isString() || !middleName.isString() || !phoneNo.isString()) {
        return VasResult(TYPE_MISMATCH);
    }

    lookupResponse.surname = surname.getString();
    lookupResponse.firstName = firstName.getString();
    lookupResponse.middleName = middleName.getString();
    phoneNumber = phoneNo.getString();

    return VasResult(NO_ERRORS);
}

int JambViewModel::getPaymentJson(iisys::JSObject& json) const
{
    json("productCode") = lookupResponse.productCode;
    json("paymentMethod") = apiPaymentMethodString(payMethod);
    json("customerPhoneNumber") = phoneNumber;
    json("confirmationCode") = getConfirmationCode();
    json("email") = this->email;
    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();
    json("service") = apiServiceString(service);

    return 0;
}

const char* JambViewModel::apiServiceString(Service service)
{
    if (service == JAMB_UTME_PIN) {
        return "jambutme";
    } else if (service == JAMB_DE_PIN) {
        return "jambde";
    }
    
    return "";
}

VasResult JambViewModel::requery(VasComProxy& comProxy, const std::string& phoneNumber, const std::string& confirmationCode, const Service service)
{
    VasResult result;

    if (service != JAMB_UTME_PIN && service != JAMB_DE_PIN) {
        return result;
    }

    iisys::JSObject json;

    json("service") = apiServiceString(service);
    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();
    json("confirmationCode") = confirmationCode;
    json("customerPhoneNumber") = phoneNumber;

    result = comProxy.sendVasRequest("/api/v1/jamb/transaction/requery", &json);

    return result;
}
