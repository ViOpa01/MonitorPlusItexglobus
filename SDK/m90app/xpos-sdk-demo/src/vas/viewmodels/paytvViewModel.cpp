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

#include "paytvViewModel.h"

#include "vasdb.h"

PayTVViewModel::PayTVViewModel(const char* title, VasComProxy& proxy)
    : _title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}

VasResult PayTVViewModel::lookup()
{
    VasResult response;
    iisys::JSObject obj;

    if (getLookupJson(obj, service) < 0) {
        return VasResult(VAS_ERROR, "Data Error");
    }

    response = comProxy.lookup("/api/v1/vas/cabletv/validation", &obj);
    
    
    return lookupCheck(response);
}

VasResult PayTVViewModel::complete(const std::string& pin)
{
    iisys::JSObject json;
    VasResult response;
    std::string rrn;

    if (getPaymentJson(json, service) < 0) {
        response.error = VAS_ERROR;
        response.message = "Data Error";
        return response;
    }

    json("pin") = encryptedPin(Payvice(), pin.c_str());
    json("clientReference") = getClientReference(rrn);

    if (payMethod == PAY_WITH_CARD) {
        cardData = CardData(amount, rrn);
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

    response = processPaymentResponse(json, service);

    return response;
}

VasResult PayTVViewModel::lookupCheck(const VasResult& lookupStatus)
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

    const iisys::JSObject& name = responseData("name");
    const iisys::JSObject& unit = responseData("unit");
    const iisys::JSObject& bouquet = responseData("bouquet");
    const iisys::JSObject& balance = responseData("balance");
    const iisys::JSObject& account = responseData("account");
    const iisys::JSObject& pCode = responseData("productCode");
    const iisys::JSObject& bundles = responseData("bouquets");

    if (!name.isString()) {
        return VasResult(KEY_NOT_FOUND, "Name not found");
    } else if (!pCode.isString()) {
        return VasResult(KEY_NOT_FOUND, "Product code not found");
    } else if (!bundles.isArray()) {
        result = VasResult(VAS_ERROR, "Data Packages not found");
        return result;
    }

    lookupResponse.name = name.getString();

    if (account.isString()) {
        lookupResponse.accountNo = account.getString();
    }

    if (unit.isString()) {
        lookupResponse.unit = unit.getString();
    }

    if (bouquet.isString()) {
        lookupResponse.bouquet = bouquet.getString();
    } 

    if (balance.isString()) {
        lookupResponse.balance = balance.getString();
    } 

    lookupResponse.productCode = pCode.getString();
    
    if (bundles.size() == 0) {
        result = VasResult(VAS_ERROR, "Bouquets empty");
        return result;
    }

    bouquets = bundles;
    result = VasResult(NO_ERRORS); 
    return result;
}

std::map<std::string, std::string> PayTVViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const std::string amountStr = vasimpl::to_string(this->amount);
    
    record[VASDB_PRODUCT] = this->selectedPackage("name").getString(); 
    record[VASDB_BENEFICIARY_NAME] = lookupResponse.name;

    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceToString(service);
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY] = iuc;
    record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
    record[VASDB_PAYMENT_METHOD] = paymentString(payMethod);

    record[VASDB_REF] = paymentResponse.reference;
    record[VASDB_DATE] = paymentResponse.date;
    if (!paymentResponse.transactionSeq.empty()) {
        record[VASDB_TRANS_SEQ] = paymentResponse.transactionSeq;
    }

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

    return record;
}

const iisys::JSObject& PayTVViewModel::getBouquets() const
{
    return bouquets;
}

Service PayTVViewModel::getService() const
{
    return service;
}

int PayTVViewModel::getLookupJson(iisys::JSObject& json, Service service) const
{
    json.clear();

    if (service == GOTV || service == DSTV) {
        json("account") = iuc;
    } else if (service == STARTIMES) {
        json("smartCardCode") = iuc;
        if (this->startimestype == STARTIMES_TOPUP){
            json("amount") = majorDenomination(amount);
        }
    }
    json("type") = apiServiceType(service);
    json("channel") = vasChannel();
    json("service") = apiServiceString(service);

    return 0;
}

int PayTVViewModel::getPaymentJson(iisys::JSObject& json, Service service) const
{

    if (service == DSTV || service == GOTV) {
        json("iuc") = iuc;
        json("code") = selectedPackage("code");
        json("unit") = lookupResponse.unit;
    } else if (service == STARTIMES) {
        if (this->startimestype == STARTIMES_BUNDLE) {
            json("bouquet") = selectedPackage("name");
            json("cycle") = this->cycle;
        } else if (this->startimestype == STARTIMES_TOPUP) {
            json("amount") = majorDenomination(amount);
        }
    }
    
    json("productCode") = lookupResponse.productCode;
    json("channel") = vasChannel();
    json("phone") = phoneNumber;
    json("service") = apiServiceString(service);
    json("paymentMethod") = apiPaymentMethodString(payMethod);
    
    return 0;
}

const char* PayTVViewModel::apiServiceString(Service service) const
{
    switch (service) {   
    case DSTV:
    case GOTV :
        return "multichoice";
    case STARTIMES :
        return "startimes";
    default:
        return "";
    }
}

const char* PayTVViewModel::apiServiceType(Service service) const
{
    switch (service) {
        case DSTV:
            return "DSTV";
        case GOTV:
            return "GOTV";  
        case STARTIMES:
            if (this->startimestype == STARTIMES_BUNDLE) {
                return "default";
            } else if (this->startimestype == STARTIMES_TOPUP) {
                return "topup";
            } else {
                return "";
            }
        default:
            return "";
    }
}

unsigned long PayTVViewModel::getAmount() const
{
    return amount;
}

VasResult PayTVViewModel::setAmount(unsigned long amount)
{
    VasResult result;

    if (getService() == STARTIMES && this->startimestype == STARTIMES_TOPUP) {
        this->amount = amount;
        result = VasResult(NO_ERRORS);
    }

    return result;
}

std::string PayTVViewModel::getIUC() const
{
    return iuc;
}

VasResult PayTVViewModel::setIUC(const std::string& iuc)
{
    this->iuc = iuc;
    return VasResult(NO_ERRORS);
}

VasResult PayTVViewModel::setSelectedPackage(int selectedPackageIndex, const std::string& cycle)
{
    VasResult result;

    if (selectedPackageIndex < 0 || (size_t)selectedPackageIndex >= bouquets.size()) {
        return result;
    }

    result.error = NO_ERRORS;
    this->selectedPackage = bouquets[selectedPackageIndex];

    if (getService() == STARTIMES && startimestype == STARTIMES_BUNDLE) {
        const iisys::JSObject& cycles = this->selectedPackage("cycles");
        this->cycle = cycle;
        this->amount = (unsigned long) lround(cycles(cycle).getNumber() * 100.0);
    } else {
        this->amount = (unsigned long) lround(this->selectedPackage("amount").getNumber() * 100.0);
    }

    return result;
}

VasResult PayTVViewModel::setStartimesType(StartimesType startimestype)
{   
    VasResult result;

    if (startimestype != UNKNOWN_TYPE) {
        result.error = NO_ERRORS;
        this->startimestype = startimestype;
    }

    return result;
}

PayTVViewModel::StartimesType PayTVViewModel::getStartimesTypes() const
{
    return startimestype;
}

const char* PayTVViewModel::paymentPath()
{
    return "/api/v1/vas/cabletv/subscription";
}

VasResult PayTVViewModel::processPaymentResponse(const iisys::JSObject& json, Service service)
{
    VasResult response = vasResponseCheck(json);

    this->service = service;

    const iisys::JSObject& responseData = json("data");
    if (!responseData.isObject()) {
        return response;
    }

    getVasTransactionDateTime(paymentResponse.date, responseData);
    getVasTransactionReference(paymentResponse.reference, responseData);
    getVasTransactionSequence(paymentResponse.transactionSeq, responseData);

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

VasResult PayTVViewModel::setPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (!phoneNumber.empty()) {
        result.error = NO_ERRORS;
        this->phoneNumber = phoneNumber;
    }

    return result;
}

VasResult PayTVViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }   

    return result;
}

VasResult PayTVViewModel::setService(Service service)
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

const char* PayTVViewModel::serviceTypeString(StartimesType type)
{
	switch (type) {
	case STARTIMES_BUNDLE:
		return "default";
	case STARTIMES_TOPUP:
		return "topup";
    default:
	    return "";
    }
}

