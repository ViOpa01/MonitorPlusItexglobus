#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>


#include "simpio.h"
#include "pfm.h"

#include "vasdb.h"

#include "data.h"

Data::Data(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
{
}

VasStatus Data::beginVas()
{
    Service services[] = { AIRTELDATA, ETISALATDATA, GLODATA, MTNDATA };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    service = selectService("Networks", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

VasStatus Data::lookup(const VasStatus&)
{
    VasStatus response;
    iisys::JSObject obj;

    obj("service") = serviceToProductString(service);

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    response = comProxy.lookup("/vas/data/lookup", &obj);
    
    return lookupCheck(response);
}

VasStatus Data::initiate(const VasStatus& lookupStatus)
{

    while (1) {

        payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
        if (payMethod == PAYMENT_METHOD_UNKNOWN) {
            return VasStatus(USER_CANCELLATION);
        }

        phoneNumber = getPhoneNumber("Phone Number", "");
        if (!phoneNumber.empty()) {
            break;
        }
    }

    return VasStatus(NO_ERRORS);
}

Service Data::vasServiceType()
{
    return service;
}

VasStatus Data::complete(const VasStatus& initiateStatus)
{
    std::string pin;
    iisys::JSObject json;
    VasStatus response;

    switch (getPin(pin, "Payvice Pin")) {
    case EMV_CT_PIN_INPUT_ABORT:
        response.error = INPUT_ABORT;
        return response;
    case EMV_CT_PIN_INPUT_TIMEOUT:
        response = INPUT_TIMEOUT_ERROR;
        return response;
    case EMV_CT_PIN_INPUT_OTHER_ERR:
        response = INPUT_ERROR;
        return response;
    default:
        break;
    }

    if (getPaymentJson(json, service) < 0) {
        response.error = VAS_ERROR;
        response.message = "Data Error";
        return response;
    }

    json("pin") = encryptedPin(Payvice(), pin.c_str());
    // json("pfm")("state") = getState();
    json("pfm")("journal")("amount") = amount;
    json("clientReference") = getClientReference();

    Demo_SplashScreen("Payment In Progress", "www.payvice.com");

    if (payMethod == PAY_WITH_CARD) {
        cardPurchase.amount = amount;
        response = comProxy.complete(paymentPath(service), &json, NULL, &cardPurchase);
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

    response = processPaymentResponse(json, service);

    return response;
}

std::map<std::string, std::string> Data::storageMap(const VasStatus& completionStatus)
{
    std::map<std::string, std::string> record;
    char amountStr[16] = { 0 };


    sprintf(amountStr, "%lu", amount);

    record[VASDB_PRODUCT] = lookupResponse.selectedPackage("description").getString(); 
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

    if (completionStatus.error == NO_ERRORS) {
        record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::APPROVED);
    } else {
        record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::DECLINED);
    }
    record[VASDB_STATUS_MESSAGE] = paymentResponse.message;
    // record[VASDB_SERVICE_DATA] = paymentResponse.serviceData;

    return record;
}

VasStatus Data::lookupCheck(const VasStatus& lookupStatus)
{
    iisys::JSObject lookupData;
    if (!lookupData.load(lookupStatus.message)) {
        return VasStatus(INVALID_JSON, "Invalid Response");
    }

    VasStatus status = vasErrorCheck(lookupData);
    if (status.error) {
        return status;
    }

    const iisys::JSObject& data = lookupData("data");
    
    if (data.isNull() || !data.isArray()) {
        return VasStatus(VAS_ERROR, "Data Packages not found");
    }

        // "data": [
        //     {
        //         "type": "MTNDATA",
        //         "code": "104",
        //         "description": "50MB 24Hrs",
        //         "amount": "100",
        //         "value": "50MB",
        //         "duration": "24Hrs"
        //     },
        //     {
        //         "type": "MTNDATA",
        //         "code": "113",
        //         "description": "150MB 24Hrs",
        //         "amount": "200",
        //         "value": "150MB",
        //         "duration": "24Hrs"
        //     },
        // ]

    const size_t size = data.size();
    std::vector<std::string> menuData;
    for (size_t i = 0; i < size; ++i) {
        menuData.push_back(data[i]("description").getString() + '\n' + data[i]("amount").getString() + " Naira");
    }

    int selection = UI_ShowSelection(60000, "Data Packages", menuData, 0);

    if (selection < 0) {
        return VasStatus(USER_CANCELLATION);
    }

    lookupResponse.selectedPackage = data[selection];
    amount = lookupResponse.selectedPackage("amount").getInt() * 100;
    
    return VasStatus(NO_ERRORS);
}

int Data::getPaymentJson(iisys::JSObject& json, Service service)
{
    Payvice payvice;

    if (!loggedIn(payvice)) {
        return -1;
    }

    std::string paymentMethodStr = paymentString(payMethod);
    std::transform(paymentMethodStr.begin(), paymentMethodStr.end(), paymentMethodStr.begin(), ::tolower);

    const std::string username = payvice.object(Payvice::USER).getString();
    const std::string password = payvice.object(Payvice::PASS).getString();
    const std::string walletId = payvice.object(Payvice::WALLETID).getString();


    json("terminal") = getDeviceTerminalId();
    json("user_id") = username;
    json("password") = password;
    json("terminal_id") = walletId;
    json("paymentMethod") = paymentMethodStr;

    json("service") = serviceToProductString(service);
    json("channel") = "POS";

    json("code") = lookupResponse.selectedPackage("code").getString();
    json("amount") = amount / 100;
    json("description") = lookupResponse.selectedPackage("description").getString();

    json("phone") = phoneNumber;

    return 0;
}

VasStatus Data::processPaymentResponse(iisys::JSObject& json, Service service)
{
    VasStatus response = vasErrorCheck(json);
    paymentResponse.message = response.message;

    iisys::JSObject ref = json("ref");
    if (!ref.isNull()) {
        paymentResponse.reference = ref.getString();
    }

    iisys::JSObject date = json("date");
    if (!date.isNull()) {
        paymentResponse.date = date.getString() + ".000";
    }

    iisys::JSObject seq = json("transactionID");
    if (!seq.isNull()) {
        paymentResponse.transactionSeq = seq.getString();
    }

    return response;
}

const char* Data::paymentPath(Service service)
{
    if (payMethod == PAY_WITH_CARD) {
        return "/vas/card/data/subscribe";
    } else if (payMethod == PAY_WITH_CASH) {
        return "/vas/data/subscribe";
    }

    return "";
}

