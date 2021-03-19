#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctype.h>
#include <iostream>

#include "platform/platform.h"
#include "payvice.h"
#include "virtualtid.h"

#include "airtimeViewModel.h"

#include "vasdb.h"
#include "vas.h"

AirtimeViewModel::AirtimeViewModel(const char* title, VasComProxy& proxy)
    : title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}


VasResult AirtimeViewModel::setAmount(unsigned long amount)
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


const char* AirtimeViewModel::paymentPath() const
{
    return "/api/v1/vas/vtu/purchase";
}

int AirtimeViewModel::getPaymentJson(iisys::JSObject& json) const
{
    std::string paymentMethodStr = paymentString(this->payMethod);
    std::transform(paymentMethodStr.begin(), paymentMethodStr.end(), paymentMethodStr.begin(), ::tolower);

    json.clear();
    json("phone") = this->phoneNumber;
    json("amount") = majorDenomination(this->amount);
    json("paymentMethod") = paymentMethodStr;
	json("channel") = vasChannel();
	json("version") = vasApplicationVersion();
	json("service") = apiServiceString(this->service);

    return 0;
}

VasResult AirtimeViewModel::processPaymentResponse(const iisys::JSObject& json)
{
    VasResult response = vasResponseCheck(json);
 
    const iisys::JSObject& responseData = json("data");
    if (!responseData.isObject()) {
        return response;
    }

    getVasTransactionDateTime(paymentResponse.date, responseData);
    getVasTransactionReference(paymentResponse.reference, responseData);
    getVasTransactionSequence(paymentResponse.transactionSeq, responseData);

    const iisys::JSObject& pinData = responseData("pin_data");
    if (pinData.isObject()) {
        paymentResponse.serviceData = pinData.getString();
    }

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

std::map<std::string, std::string> AirtimeViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    std::string serviceStr(serviceToString(service));
    const std::string amountStr = vasimpl::to_string(this->amount);

    record[VASDB_PRODUCT] = serviceStr; 
    record[VASDB_CATEGORY] = title;
    record[VASDB_SERVICE] = serviceStr;
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY] = phoneNumber;
    record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
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
    record[VASDB_SERVICE_DATA] = paymentResponse.serviceData;

    return record;
}

VasResult AirtimeViewModel::complete(const std::string& pin)
{
    iisys::JSObject json;
    VasResult response;
    std::string rrn;

    if (getPaymentJson(json) < 0) {
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
    } else if (payMethod == PAY_WITH_CASH) {
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

VasResult AirtimeViewModel::setService(Service service)
{
    VasResult result;

    switch (service) {
    case ETISALATVTU:
    case AIRTELVTU:
    case AIRTELVOT:
    case AIRTELVOS:
    case GLOVTU:
    case GLOVOS:
    case GLOVOT:
    case MTNVTU:
        result.error = NO_ERRORS;
        this->service = service;
        break;
    default:
        break;
    }

    return result;
}

VasResult AirtimeViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }   

    return result;
}

VasResult AirtimeViewModel::setPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (!phoneNumber.empty()) {
        result.error = NO_ERRORS;
        this->phoneNumber = phoneNumber;
    }

    return result;
}

Service AirtimeViewModel::getService() const
{
    return service;
}

std::string AirtimeViewModel::apiServiceString(Service service) const
{
    switch (service) {
    case MTNVTU:
        return "mtnvtu";
    case ETISALATVTU:
        return "9mobilevtu";
    case GLOVTU:
        return "glovtu";
    case GLOVOT:
        return "glovot";
    case GLOVOS:
        return "glovos";
    case AIRTELVTU:
        return "airtelvtu";
    case AIRTELVOT:
    case AIRTELVOS:
        return "airtelpin";    
    default:
        return "";
    }
}
