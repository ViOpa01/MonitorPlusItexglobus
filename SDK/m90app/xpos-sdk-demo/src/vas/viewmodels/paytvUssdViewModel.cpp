#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "payvice.h"
#include "platform/platform.h"
#include "virtualtid.h"

#include "paytvUssdViewModel.h"

#include "vasdb.h"
#include "vasussd.h"

PaytvUssdViewModel::PaytvUssdViewModel(const char* title, VasComProxy& proxy)
    : title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}

VasResult PaytvUssdViewModel::lookup()
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

VasResult PaytvUssdViewModel::bouquetLookup()
{
    VasResult response;
    iisys::JSObject obj;
    std::string endpoint;

    if (this->service == STARTIMES) {
        endpoint = "/api/v1/vas/pin/service/list/STARTIMES";
    } else {
        endpoint = std::string("/api/v1/vas/pin/service/list/") + apiServiceType(this->service);
    }

    response = comProxy.lookup(endpoint.c_str(), NULL);

    return bouquetLookupCheck(response);
}

VasResult PaytvUssdViewModel::bouquetLookupCheck(const VasResult& lookupStatus)
{
    iisys::JSObject response;
    VasResult result;

    if (lookupStatus.error != NO_ERRORS) {
        result = VasResult(LOOKUP_ERROR, lookupStatus.message.c_str());
        return result;
    }

    if (!response.load(lookupStatus.message)) {
        result = VasResult(INVALID_JSON, "Invalid Response");
        return result;
    }

    result = vasResponseCheck(response);

    if (result.error != NO_ERRORS) {
        return result;
    }

    const iisys::JSObject& responseData = response("data");
    if (!responseData.isObject()) {
        return result;
    }

    const iisys::JSObject& bundles = responseData("packages");

    if (!bundles.isArray()) {
        result = VasResult(VAS_ERROR, "Data Packages not found");
        return result;
    }

    if (bundles.size() == 0) {
        result = VasResult(VAS_ERROR, "Bouquets empty");
        return result;
    }

    bouquets = bundles;
    result = VasResult(NO_ERRORS);
    return result;
}

VasResult PaytvUssdViewModel::setAmount(unsigned long amount)
{
    this->amount = amount;
    return VasResult(NO_ERRORS);
}

VasResult PaytvUssdViewModel::complete(const std::string& pin)
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

std::map<std::string, std::string> PaytvUssdViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const iisys::JSObject& name = selectedPackage("name");
    std::string product = serviceToString(service);
    
    if (name.isString()) {
        product = product + " " + name.getString();
    }

    record[VASDB_SERVICE] = serviceToString(PAYTV_USSD);
    record[VASDB_PRODUCT] = product;
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

VasResult PaytvUssdViewModel::setUnits(const int units)
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

VasResult PaytvUssdViewModel::setService(Service service)
{
    VasResult result;

    switch (service) {
    case DSTV:
    case GOTV:
    case STARTIMES:
        result.error = NO_ERRORS;
        this->service = service;
        break;
    default:
        break;
    }

    return result;
}

VasResult PaytvUssdViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }

    return result;
}

VasResult PaytvUssdViewModel::setSelectedPackage(int selectedPackageIndex, const std::string& cycle)
{
    VasResult result;

    if (selectedPackageIndex < 0 || (size_t)selectedPackageIndex >= bouquets.size()) {
        return result;
    }

    result.error = NO_ERRORS;
    this->selectedPackage = bouquets[selectedPackageIndex];

    if (getService() == STARTIMES) {
        const iisys::JSObject& cycles = this->selectedPackage("cycles");
        this->cycle = cycle;
        this->amount = (unsigned long)lround(cycles(cycle).getNumber() * 100.0);
    } else {
        this->amount = (unsigned long)lround(this->selectedPackage("amount").getNumber() * 100.0);
    }

    return result;
}

int PaytvUssdViewModel::getUnits() const
{
    return this->units;
}

Service PaytvUssdViewModel::getService() const
{
    return service;
}

unsigned long PaytvUssdViewModel::getAmount() const
{
    return amount;
}

PaymentMethod PaytvUssdViewModel::getPaymentMethod() const
{
    return payMethod;
}

const iisys::JSObject& PaytvUssdViewModel::getBouquets() const
{
    return bouquets;
}

int PaytvUssdViewModel::getLookupJson(iisys::JSObject& json) const
{
    json.clear();

    json("channel") = vasChannel();
    json("vasService") = "CABLE";
    json("version") = vasApplicationVersion();
    json("paymentMethod") = apiPaymentMethodString(this->payMethod);

    iisys::JSObject& data = json("data");

    data("service") = apiServiceString(this->service);
    data("type") = apiServiceType(this->service);

    if (this->service == STARTIMES) {
        data("bouquet") = selectedPackage("name");
        data("cycle") = cycle;
    } else if (this->service == DSTV) {
        data("code") = selectedPackage("product_code");
    } else if (this->service == GOTV) {
        // TODO change line for live, na so we see am o

        // data("code") = selectedPackage("product_code"); // live
        data("code") = selectedPackage("code"); // test
    }

    data("unit") = this->units;
    json("version") = vasApplicationVersion();

    return 0;
}

VasResult PaytvUssdViewModel::processPaymentResponse(const iisys::JSObject& json)
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

const char* PaytvUssdViewModel::paymentPath()
{
    return vasussd::pinGenerationPaymentPath();
}

const char* PaytvUssdViewModel::apiServiceString(Service service) const
{
    switch (service) {
    case DSTV:
    case GOTV:
        return "multichoice";
    case STARTIMES:
        return "startimes";
    default:
        return "";
    }
}

const std::string& PaytvUssdViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

const char* PaytvUssdViewModel::apiServiceType(Service service) const
{
    switch (service) {
    case DSTV:
        return "DSTV";
    case GOTV:
        return "GOTV";
    case STARTIMES:
        return "default";
    default:
        return "";
    }
}
