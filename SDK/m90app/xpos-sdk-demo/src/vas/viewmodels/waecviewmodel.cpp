#include <algorithm>
#include <ctype.h>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "platform/platform.h"

#include "payvice.h"
#include "vasdb.h"

#include "waecviewmodel.h"

WaecViewModel::WaecViewModel(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
    , payMethod(PAYMENT_METHOD_UNKNOWN)
{
    lookupResponse.amount = 0;
}

VasResult WaecViewModel::lookupCheck(const VasResult& lookupStatus)
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

    return extractFirstStageInfo(data);
}

VasResult WaecViewModel::lookup()
{
    iisys::JSObject obj;

    obj("version") = vasApplicationVersion();
    obj("channel") = vasChannel();
    obj("service") = apiServiceString(getService());

    VasResult response = comProxy.lookup("/api/v1/waec/token/lookup", &obj);

    return lookupCheck(response);
}

VasResult WaecViewModel::setService(Service service)
{
    VasResult result;

    if (service != WAEC_REGISTRATION && service != WAEC_RESULT_CHECKER) {
        return result;
    }

    this->service = service;
    result.error = NO_ERRORS;
    return result;
}

VasResult WaecViewModel::setAmount(unsigned long amount)
{
    VasResult result;
    if (amount < 5000) {
        char message[128] = { 0 };
        sprintf(message, "Least Payable amount is %.2f", 50.0);
        result.message = message;
        return result;
    }

    lookupResponse.amount = amount;
    return VasResult(NO_ERRORS);
}

VasResult WaecViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod != PAY_WITH_CARD && payMethod != PAY_WITH_CASH) {
        return result;
    }

    this->payMethod = payMethod;

    result.error = NO_ERRORS;
    return result;
}

VasResult WaecViewModel::setEmail(const std::string& email)
{
    VasResult result;

    if (email.empty()) {
        return result;
    }

    userData.email = email;

    result.error = NO_ERRORS;
    return result;
}

VasResult WaecViewModel::setLastName(const std::string& lastName)
{
    VasResult result;

    if (lastName.empty()) {
        return result;
    }

    userData.lastName = lastName;

    result.error = NO_ERRORS;
    return result;
}

VasResult WaecViewModel::setFirstName(const std::string& firstName)
{
    VasResult result;

    if (firstName.empty()) {
        return result;
    }

    userData.firstName = firstName;

    result.error = NO_ERRORS;
    return result;
}

VasResult WaecViewModel::setPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (phoneNumber.empty()) {
        return result;
    }

    userData.phoneNumber = phoneNumber;

    result.error = NO_ERRORS;
    return result;
}

Service WaecViewModel::getService() const
{
    return service;
}

unsigned long WaecViewModel::getAmount() const
{
    return lookupResponse.amount;
}

const std::string& WaecViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

PaymentMethod WaecViewModel::getPaymentMethod() const
{
    return payMethod;
}

VasResult WaecViewModel::complete(const std::string& pin)
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

std::map<std::string, std::string> WaecViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const Service service = getService();
    std::string serviceStr(serviceToString(service));
    const std::string amountStr = vasimpl::to_string(lookupResponse.amount);

    record[VASDB_PRODUCT] = serviceStr;
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceStr;
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY_NAME] = userData.firstName + " " + userData.lastName;
    record[VASDB_BENEFICIARY_PHONE] = userData.phoneNumber;
    record[VASDB_PAYMENT_METHOD] = paymentString(payMethod);

    record[VASDB_REF] = paymentResponse.reference;
    record[VASDB_DATE] = paymentResponse.date;
    record[VASDB_TRANS_SEQ] = paymentResponse.transactionSeq;

    if (payMethod == PAY_WITH_CARD) {
        if (cardData.primaryIndex > 0) {
            char primaryIndex[16] = { 0 };
            sprintf(primaryIndex, "%lu", cardData.primaryIndex);
            record[VASDB_CARD_ID] = primaryIndex;
        }

        if (vasimpl::getDeviceTerminalId() != cardData.transactionTid) {
            record[VASDB_VIRTUAL_TID] = cardData.transactionTid;
        }
    }

    record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::vasErrToTrxStatus(completionStatus.error));

    record[VASDB_STATUS_MESSAGE] = completionStatus.message;
    record[VASDB_SERVICE_DATA] = (service == WAEC_REGISTRATION ? (std::string("{\"token\": \"") + paymentResponse.token) : (std::string("{\"pin\": \"") + paymentResponse.pin)) + std::string("\",\"email\": \"") + userData.email + "\"}";

    return record;
}

const char* WaecViewModel::paymentPath()
{
    return "/api/v1/waec/token/purchase";
}

VasResult WaecViewModel::processPaymentResponse(const iisys::JSObject& json)
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
        if (service == WAEC_REGISTRATION) {
            const iisys::JSObject& tokenArray = detail("Token");
            if (tokenArray.isArray()) {
                const iisys::JSObject& token = tokenArray[0];
                if (token.isString()) {
                    paymentResponse.token = token.getString();
                }
            }
        } else if (service == WAEC_RESULT_CHECKER) {
            const iisys::JSObject& pin = detail("Pin");
            if (pin.isString()) {
                paymentResponse.pin = pin.getString();
            }
        }
    }

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

VasResult WaecViewModel::extractFirstStageInfo(const iisys::JSObject& data)
{
    const iisys::JSObject& Waecdata = data("data");
    const iisys::JSObject& productCode = data("productCode");

    if (!Waecdata.isObject() || !productCode.isString()) {
        return VasResult(TYPE_MISMATCH);
    }

    const iisys::JSObject& amount = Waecdata("sellingPrice");

    if (!amount.isString() && !amount.isNumber()) {
        return VasResult(TYPE_MISMATCH);
    }

    lookupResponse.amount = (unsigned long) lround(amount.getNumber() * 100.0f);
    lookupResponse.productCode = productCode.getString();

    return VasResult(NO_ERRORS);
}

int WaecViewModel::getPaymentJson(iisys::JSObject& json) const
{
    json("email") = userData.email;
    json("phone") = userData.phoneNumber;
    json("customerName") = userData.firstName + " " + userData.lastName;
    json("productCode") = lookupResponse.productCode;
    json("version") = vasApplicationVersion();
    json("paymentMethod") = apiPaymentMethodString(payMethod);
    json("channel") = vasChannel();
    json("service") = apiServiceString(service);

    return 0;
}

const char* WaecViewModel::apiServiceString(Service service)
{
    if (service == WAEC_REGISTRATION) {
        return "waecregistration";
    } else if (service == WAEC_RESULT_CHECKER) {
        return "waecresult";
    }

    return "";
}
