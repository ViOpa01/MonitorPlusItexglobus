#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctype.h>
#include <math.h>
#include <iostream>

#include "platform/platform.h"
#include "payvice.h"
#include "virtualtid.h"

#include "dataUssdViewModel.h"

#include "vasdb.h"
#include "vasussd.h"

DataUssdViewModel::DataUssdViewModel(const char* title, VasComProxy& proxy)
    : _title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}

VasResult DataUssdViewModel::packageLookupCheck(const VasResult& lookupStatus)
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
    
    const iisys::JSObject& data = responseData("data");
    const iisys::JSObject& pcode = responseData("productCode");

    if (pcode.isNull()) {
        result = VasResult(KEY_NOT_FOUND, "Product Code not found");
        return result;
    }

    if (data.isNull() || !data.isArray()) {
        return VasResult(VAS_ERROR, "Data Packages not found");
    }

    lookupResponse.packages = data;
    lookupResponse.productCode = pcode.getString();

    
    return VasResult(NO_ERRORS);
}

VasResult DataUssdViewModel::packageLookup()
{
    VasResult response;
    iisys::JSObject obj;

    if(getPackageLookupJson(obj) < 0){
        return VasResult(VAS_ERROR, "Data Error");
    }

    response = comProxy.lookup("/api/v1/vas/data/lookup", &obj);
    
    return packageLookupCheck(response);
}

VasResult DataUssdViewModel::lookup()
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

int DataUssdViewModel::getLookupJson(iisys::JSObject& json) const
{
    json.clear();

    json("channel") = vasChannel();
    json("vasService") = "DATA";
    json("version") = vasApplicationVersion();
    json("paymentMethod") = apiPaymentMethodString(this->payMethod);

    iisys::JSObject& data = json("data");
    data("service") = apiServiceString(this->service);
    data("code") = this->selectedPackage("code");
    data("unit") = this->units;

    return 0;
}

int DataUssdViewModel::getPackageLookupJson(iisys::JSObject& json) const
{
    json.clear();

    json("service") = apiServiceString(this->service);
    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();

    return 0;
}

const char* DataUssdViewModel::apiServiceString(Service service) const
{
    switch (service) {

        case ETISALATDATA:
            return "9mobiledata";
        case AIRTELDATA:
            return "airteldata";
        case GLODATA:
            return "glodata";
        case MTNDATA:
            return "mtndata";
        default: 
            return "";
    }
}

VasResult DataUssdViewModel::setUnits(const int units)
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

VasResult DataUssdViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH || payMethod == PAY_WITH_NQR) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }   

    return result;
}

int DataUssdViewModel::getUnits() const
{
    return this->units;
}

VasResult DataUssdViewModel::complete(const std::string& pin)
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

std::map<std::string, std::string> DataUssdViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;

    record[VASDB_PRODUCT] = this->selectedPackage("description").getString(); 
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceToString(service);
    record[VASDB_AMOUNT] = vasimpl::to_string(this->amount);

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
    record[VASDB_SERVICE_DATA] = paymentResponse.serviceData;

    return record;
}

const char* DataUssdViewModel::paymentPath()
{
    return vasussd::pinGenerationPaymentPath();
}


VasResult DataUssdViewModel::processPaymentResponse(const iisys::JSObject& json)
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

Service DataUssdViewModel::getService() const
{
    return service;
}

unsigned long DataUssdViewModel::getAmount() const
{
    return this->amount;
}

PaymentMethod DataUssdViewModel::getPaymentMethod() const
{
    return payMethod;
}

const std::string& DataUssdViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

VasResult DataUssdViewModel::setService(Service service)
{
    VasResult result;

    switch (service) {
    case MTNDATA:
    case GLODATA:
    case AIRTELDATA:
    case ETISALATDATA:
        result.error = NO_ERRORS;
        this->service = service;
        break;
    default:
        break;
    }

    return result;
}

VasResult DataUssdViewModel::setSelectedPackageIndex(int index)
{
    VasResult result;

    if (index < 0 || (size_t)index >= lookupResponse.packages.size()) {
        return result;
    }

    result.error = NO_ERRORS;
    this->selectedPackage = lookupResponse.packages[index];
    this->amount = (unsigned long) lround(this->selectedPackage("amount").getNumber() * 100.0);

    return result;
}

const iisys::JSObject& DataUssdViewModel::getPackages() const
{
    return this->lookupResponse.packages;
}
