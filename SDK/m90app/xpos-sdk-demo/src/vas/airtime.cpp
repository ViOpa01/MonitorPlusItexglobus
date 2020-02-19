#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

extern "C" {
#include "util.h"
}

#include "simpio.h"
#include "pfm.h"

#include "vasdb.h"
#include "airtime.h"

Airtime::Airtime(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
{
}

VasStatus Airtime::beginVas()
{
    Network networks[] = { AIRTEL, ETISALAT, GLO, MTN };
    std::vector<std::string> menu;

    for (size_t i = 0; i < sizeof(networks) / sizeof(Network); ++i) {
        menu.push_back(networkString(networks[i]));
    }

    int selection = UI_ShowSelection(30000, "Networks", menu, 0);
    if (selection < 0) {
        return VasStatus(USER_CANCELLATION);
    }

    service = getAirtimeService(networks[selection]);
    if (service == SERVICE_UNKNOWN) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

VasStatus Airtime::lookup(const VasStatus&)
{
    return VasStatus(NO_ERRORS);   
}

Service Airtime::getAirtimeService(Network network)
{

    switch (network) {
    case ETISALAT:
        return ETISALATVTU;
    case AIRTEL: {
        Service services[] = { AIRTELVTU, AIRTELVOT };
        std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

        return selectService(networkString(network).c_str(), serviceVector);
    }
    case GLO: {
        Service services[] = { GLOVTU, GLOVOS, GLOVOT };
        std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

        return selectService(networkString(network).c_str(), serviceVector);
    }
    case MTN:
        return MTNVTU;
    }

    return SERVICE_UNKNOWN;
}

VasStatus Airtime::initiate(const VasStatus& lookupStatus)
{

    while (1) {
        amount = getAmount(serviceToString(service));
        if (amount <= 0) {
            return VasStatus(USER_CANCELLATION);
        } else if (amount < 5000) {
            char message[128] = { 0 };
            sprintf(message, "Least Payable amount is %.2f", 50.0);
            UI_ShowButtonMessage(30000, "Please Try Again", message, "OK", UI_DIALOG_TYPE_WARNING);
            continue;
        }

        payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
        if (payMethod == PAYMENT_METHOD_UNKNOWN) {
            return VasStatus(USER_CANCELLATION);
        }

        phoneNumber = getPhoneNumber("Phone Number", "");
        if (phoneNumber.empty()) {
            continue;
        }

        break;
    }

    return VasStatus(NO_ERRORS);
}

Service Airtime::vasServiceType()
{
    return service;
}

VasStatus Airtime::complete(const VasStatus& initiateStatus)
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
    
    json("pfm")("state") = getState();
    
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

std::map<std::string, std::string> Airtime::storageMap(const VasStatus& completionStatus)
{
    std::map<std::string, std::string> record;
    std::string serviceStr(serviceToString(service));
    char amountStr[16] = { 0 };


    sprintf(amountStr, "%lu", amount);

    record[VASDB_PRODUCT] = serviceStr; 
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceStr;
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY] = phoneNumber;
    record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
    record[VASDB_PAYMENT_METHOD] = paymentString(payMethod);

    record[VASDB_REF] = paymentResponse.reference;
    record[VASDB_DATE] = paymentResponse.date;
    record[VASDB_TRANS_SEQ] = paymentResponse.transactionSeq;

    if (payMethod == PAY_WITH_CARD) {
        if(cardPurchase.primaryIndex > 0) {
            char primaryIndex[16] = { 0 };
            sprintf(primaryIndex, "%lu", cardPurchase.primaryIndex);
            record[VASDB_CARD_ID] = primaryIndex;
        }
        
        if (!itexIsMerchant()) {
            record[VASDB_VIRTUAL_TID] = Payvice().object(Payvice::VIRTUAL)(Payvice::TID).getString();
        }
    }

    if (completionStatus.error == NO_ERRORS) {
        record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::APPROVED);
    } else {
        record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::DECLINED);
    }
    record[VASDB_STATUS_MESSAGE] = paymentResponse.message;
    record[VASDB_SERVICE_DATA] = paymentResponse.serviceData;

    return record;
}

int Airtime::getPaymentJson(iisys::JSObject& json, Service service)
{
    Payvice payvice;

    if (!loggedIn(payvice)) {
        return -1;
    }

    std::string paymentMethodStr = paymentString(payMethod);
    std::transform(paymentMethodStr.begin(), paymentMethodStr.end(), paymentMethodStr.begin(), ::tolower);

    const std::string username = payvice.object(Payvice::USER).getString();
    const std::string walletId = payvice.object(Payvice::WALLETID).getString();


    json("terminal") = getDeviceTerminalId();
    json("terminal_id") = walletId;
    json("user_id") = username;
    json("paymentMethod") = paymentMethodStr;

    json("channel") = "POS";
    json("amount") = amount / 100;
    json("service") = serviceToProductString(service);

    json("phone") = phoneNumber;

    if (payMethod == PAY_WITH_CARD && !itexIsMerchant()) {
        json("virtualTid") = payvice.object(Payvice::VIRTUAL)(Payvice::TID);
    }

    return 0;
}

VasStatus Airtime::processPaymentResponse(iisys::JSObject& json, Service service)
{
    VasStatus response = vasErrorCheck(json);
    paymentResponse.message = response.message;

    this->service = service;

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

    iisys::JSObject data = json("pin_data");
    if (!data.isNull())
        paymentResponse.serviceData = data.getString();

    return response;
}

const char* Airtime::paymentPath(Service service)
{
    if (payMethod == PAY_WITH_CARD) {
        return "/vas/card/vtu/purchase";
    } else if (payMethod == PAY_WITH_CASH) {
        return "/vas/vtu/purchase";
    }

    return "";
}

std::string Airtime::networkString(Network network)
{
    switch (network) {
    case AIRTEL:
        return std::string("Airtel");
    case ETISALAT:
        return std::string("Etisalat");
    case GLO:
        return std::string("Glo");
    case MTN:
        return std::string("MTN");
    default:
        return std::string("");
    }
}