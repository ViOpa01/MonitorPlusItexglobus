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

#include "dataViewModel.h"

#include "vasdb.h"
#include "nqr.h"

DataViewModel::DataViewModel(const char* title, VasComProxy& proxy)
    : _title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}

VasResult DataViewModel::lookupCheck(const VasResult& lookupStatus)
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

    printf("lookup ==== 4\n");

    if (data.isNull() || !data.isArray()) {
        return VasResult(VAS_ERROR, "Data Packages not found");
    }

    lookupResponse.packages = data;
    lookupResponse.productCode = pcode.getString();

    printf("Go be dey oo\n");

    
    return VasResult(NO_ERRORS);
}

VasResult DataViewModel::lookup()
{
    VasResult response;
    iisys::JSObject obj;

    if(getLookupJson(obj, service) < 0){
        return VasResult(VAS_ERROR, "Data Error");
    }

    response = comProxy.lookup("/api/v1/vas/data/lookup", &obj);
    
    return lookupCheck(response);
}

int DataViewModel::getLookupJson(iisys::JSObject& json, Service service) const
{
    json.clear();

    json("service") = apiServiceString(service);
    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();

    return 0;
}

const char* DataViewModel::apiServiceString(Service service) const
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

VasResult DataViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH || payMethod == PAY_WITH_NQR) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }   

    return result;
}

VasResult DataViewModel::setPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (!phoneNumber.empty()) {
        result.error = NO_ERRORS;
        this->phoneNumber = phoneNumber;
    }

    return result;
}

int DataViewModel::getPaymentJson(iisys::JSObject& json, Service service)
{
    json.clear();
    json("phone") = phoneNumber;
    json("paymentMethod") = apiPaymentMethodString(payMethod);
	json("service") = apiServiceString(service);
    json("code") = this->selectedPackage("code");
    json("productCode") = lookupResponse.productCode;

    return 0;
}

VasResult DataViewModel::initiate(const std::string& pin)
{
    VasResult response;
    
    if (payMethod != PAY_WITH_NQR) {
        response.error = NO_ERRORS;
        return response;
    }

    iisys::JSObject json;

    if (getPaymentJson(json, service) != 0) {
        response.error = VAS_ERROR;
        response.message = "Data Error";
        return response;
    }
    
    json("clientReference") = clientReference = getClientReference(retrievalReference);
    json("pin") = encryptedPin(Payvice(), pin.c_str());

    json = createNqrJSON(amount, "DATA", json);
    response = comProxy.initiate(nqrGenerateUrl(), &json);

    if (response.error != NO_ERRORS) {
        return response;
    }

    response = parseVasQr(lookupResponse.productCode, response.message);

    return response;
}

VasResult DataViewModel::complete(const std::string& pin)
{
    iisys:: JSObject json;
    VasResult response;

    if (payMethod == PAY_WITH_NQR) {
        json = getNqrPaymentStatusJson(lookupResponse.productCode, clientReference);
    } else {
        if (getPaymentJson(json, service) < 0) {
            response.error = VAS_ERROR;
            response.message = "Data Error";
            return response;
        }

        json("pin") = encryptedPin(Payvice(), pin.c_str());
        json("clientReference") = getClientReference(retrievalReference);
    }

    if (payMethod == PAY_WITH_CARD) {
        cardData = CardData(amount, retrievalReference);
        cardData.refcode = getRefCode(serviceToProductString(service), this->selectedPackage("description").getString());
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

std::map<std::string, std::string> DataViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const std::string amountStr = vasimpl::to_string(this->amount);

    record[VASDB_PRODUCT] = this->selectedPackage("description").getString(); 
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceToString(service);
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY] = phoneNumber;
    record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
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

const char* DataViewModel::paymentPath()
{
    if (getPaymentMethod() == PAY_WITH_NQR) {
        return nqrStatusCheckUrl();
    } else {
        return "/api/v1/vas/data/subscribe";
    }
}


VasResult DataViewModel::processPaymentResponse(const iisys::JSObject& json)
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
    if (!description.isString()) {
        paymentResponse.description = description.getString();
    }

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

Service DataViewModel::getService() const
{
    return service;
}

PaymentMethod DataViewModel::getPaymentMethod() const
{
    return payMethod;
}

const std::string& DataViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

VasResult DataViewModel::setService(Service service)
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

VasResult DataViewModel::setSelectedPackageIndex(int index)
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

const iisys::JSObject& DataViewModel::getPackages() const
{
    return this->lookupResponse.packages;
}
