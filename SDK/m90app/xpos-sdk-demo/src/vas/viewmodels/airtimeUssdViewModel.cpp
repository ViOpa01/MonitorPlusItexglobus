#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "payvice.h"
#include "platform/platform.h"
#include "virtualtid.h"

#include "airtimeUssdViewModel.h"

#include "nqr.h"
#include "vas.h"
#include "vasdb.h"
#include "vasussd.h"

AirtimeUssdViewModel::AirtimeUssdViewModel(const char* title, VasComProxy& proxy)
    : title(title)
    , comProxy(proxy)
    , service(AIRTIME_USSD)
    , units(0)
{
}

VasResult AirtimeUssdViewModel::lookup()
{
    VasResult result;
    iisys::JSObject json;

    if (getLookupJson(json) != 0) {
        return result;
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

VasResult AirtimeUssdViewModel::complete(const std::string& pin)
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

VasResult AirtimeUssdViewModel::setAmount(unsigned long amount)
{
    VasResult result;
    if (amount < 5000) {
        char message[128] = { 0 };
        sprintf(message, "Least Payable amount is %.2f", 50.0);
        result.message = message;
        return result;
    }

    this->amount = amount;
    return VasResult(NO_ERRORS);
}

const char* AirtimeUssdViewModel::paymentPath() const
{
    return vasussd::pinGenerationPaymentPath();
}

int AirtimeUssdViewModel::getLookupJson(iisys::JSObject& json) const
{
    json("channel") = vasChannel();
    json("vasService") = "AIRTIMEPIN";
    json("version") = vasApplicationVersion();
    json("paymentMethod") = apiPaymentMethodString(this->payMethod);

    iisys::JSObject& data = json("data");
    data("amount") = majorDenomination(this->amount);
    data("unit") = this->units;

    return 0;
}

VasResult AirtimeUssdViewModel::processPaymentResponse(const iisys::JSObject& json)
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

std::map<std::string, std::string> AirtimeUssdViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    std::string serviceStr(serviceToString(service));

    record[VASDB_PRODUCT] = serviceStr;
    record[VASDB_CATEGORY] = title;
    record[VASDB_SERVICE] = serviceStr;
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

VasResult AirtimeUssdViewModel::setUnits(const int units)
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

VasResult AirtimeUssdViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }

    return result;
}

int AirtimeUssdViewModel::getUnits() const
{
    return this->units;
}

Service AirtimeUssdViewModel::getService() const
{
    return service;
}

unsigned long AirtimeUssdViewModel::getAmount() const
{
    return this->amount;
}

PaymentMethod AirtimeUssdViewModel::getPaymentMethod() const
{
    return payMethod;
}

const std::string& AirtimeUssdViewModel::getRetrievalReference() const
{
    return retrievalReference;
}
