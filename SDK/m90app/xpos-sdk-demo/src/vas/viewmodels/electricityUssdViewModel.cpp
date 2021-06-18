#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <ctype.h>
#include <math.h>
#include <iostream>

#include "platform/platform.h"
#include "payvice.h"
#include "virtualtid.h"

#include "electricityUssdViewModel.h"

#include "vasdb.h"
#include "vasussd.h"

ElectricityUssdViewModel::ElectricityUssdViewModel(const char* title, VasComProxy& proxy)
    : title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}

VasResult ElectricityUssdViewModel::lookup()
{
    VasResult result;
    iisys::JSObject json;
    
    if (getLookupJson(json) < 0) {
        return VasResult(VAS_ERROR, "Data Error");
    }

    result = comProxy.lookup(vasussd::pinGenerationLookupPath(), &json);

    unsigned long totalAmount = 0;
    result = vasussd::lookupCheck(totalAmount, result);
    if (result.error != NO_ERRORS) {
        return result;
    } else if (totalAmount == 0) {
        result.error = VAS_ERROR;
        result.message.clear();
        return result;
    }

    this->amount = totalAmount;
    this->productCode = result.message;

    result.error = NO_ERRORS;
    result.message.clear();

    return result;
}

VasResult ElectricityUssdViewModel::setAmount(unsigned long amount)
{
    this->amount = amount;
    return VasResult(NO_ERRORS);
}

VasResult ElectricityUssdViewModel::complete(const std::string& pin)
{
    iisys::JSObject json;
    VasResult response;

    if (vasussd::getPaymentJson(json, this->productCode, this->payMethod) < 0) {
        response.error = VAS_ERROR;
        response.message = "Data Error";
        return response;
    }

    json("pin") = encryptedPin(Payvice(), pin.c_str());
    json("clientReference") = getClientReference(retrievalReference);

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

std::map<std::string, std::string> ElectricityUssdViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;

    record[VASDB_SERVICE] = serviceToString(ELECTRICITY_USSD);
    record[VASDB_PRODUCT] = serviceToString(service);
    record[VASDB_CATEGORY] = title;
    record[VASDB_AMOUNT] = vasimpl::to_string(this->amount);
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
    record[VASDB_SERVICE_DATA] = paymentResponse.serviceData;


    return record;
}

VasResult ElectricityUssdViewModel::setUnits(const int units)
{
    VasResult result;

    if (units <= 0) {
        result = VasResult(VAS_ERROR, "Invalid number of units");
        return result;
    }

    result.error = NO_ERRORS;
    this->units = units;

    return result;
}

VasResult ElectricityUssdViewModel::setService(Service service)
{
    VasResult result;

    switch (service) {
    case IKEJA:
    case EKEDC:
    case EEDC:
    case IBEDC:
    case PHED:
    case AEDC:
    case KEDCO:
        result.error = NO_ERRORS;
        this->service = service;
        break;
    default:
        break;
    }

    return result;
}

VasResult ElectricityUssdViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }   

    return result;
}

int ElectricityUssdViewModel::getUnits() const
{
    return this->units;
}

Service ElectricityUssdViewModel::getService() const
{
    return service;
}

unsigned long ElectricityUssdViewModel::getAmount() const
{
    return amount;
}

PaymentMethod ElectricityUssdViewModel::getPaymentMethod() const
{
    return payMethod;
}

int ElectricityUssdViewModel::getLookupJson(iisys::JSObject& json) const
{
    json.clear();

    json("channel") = vasChannel();
    json("vasService") = "ELECTRICITY";
    json("version") = vasApplicationVersion();
    json("paymentMethod") = apiPaymentMethodString(this->payMethod);

    iisys::JSObject& data = json("data");

    data("amount") = majorDenomination(this->amount);
    data("service") = apiServiceString(this->service);
    data("unit") = this->units;

    json("version") = vasApplicationVersion();

    return 0;
}

VasResult ElectricityUssdViewModel::processPaymentResponse(const iisys::JSObject& json)
{
    VasResult response = vasResponseCheck(json);

    const iisys::JSObject& responseData = json("data");
    if (!responseData.isObject()) {
        return response;
    }

    getVasTransactionDateTime(paymentResponse.date, responseData);
    getVasTransactionReference(paymentResponse.reference, responseData);
    getVasTransactionSequence(paymentResponse.transactionSeq, responseData);

    iisys::JSObject serviceData;
    const VasError pinStatus = vasussd::processUssdPaymentResponse(serviceData, responseData);
    if (pinStatus == NO_ERRORS && serviceData.isObject()) {
        serviceData("units") = vasimpl::to_string(getUnits());
        paymentResponse.serviceData = serviceData.getString();
    }

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

const char* ElectricityUssdViewModel::paymentPath()
{
    return vasussd::pinGenerationPaymentPath();
}

const char* ElectricityUssdViewModel::apiServiceString(Service service) const
{
    switch (service) {
   
    case IKEJA:
        return "ikedc";
    case EKEDC :
        return "ekedc";
    case IBEDC :
        return "ibedc";
    case PHED :
        return "phedc";
    case AEDC :
        return "aedc";
    case KEDCO :
        return "kedco";
    case EEDC :
        return "eedc";
    default:
        return "";
    }
}

const std::string& ElectricityUssdViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

