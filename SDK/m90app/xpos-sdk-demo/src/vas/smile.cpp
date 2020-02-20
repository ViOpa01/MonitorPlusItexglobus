#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>


#include "simpio.h"
#include "pfm.h"

#include "vasdb.h"

#include "smile.h"

Smile::Smile(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
{
}

VasStatus Smile::beginVas()
{
    while (1) {
        Service services[] = { SMILETOPUP, SMILEBUNDLE };
        std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

        service = selectService("Smile Services", serviceVector);
        if (service == SERVICE_UNKNOWN) {
            return VasStatus(USER_CANCELLATION);
        }

        const char* optionStr[] = {"Customer ID", "Phone"};
        std::vector<std::string> optionMenu(optionStr, optionStr + sizeof(optionStr) / sizeof(char*));

        int optionSelected = UI_ShowSelection(30000, "Options", optionMenu, 0);
        if (optionSelected == 0) {
            customerID = getNumeric(0, 30000, "Customer ID", "Smile Account No", UI_DIALOG_TYPE_NONE);
        } else if (optionSelected == 1) {
            phoneNumber = getPhoneNumber("Phone Number", "");
        }

        if (customerID.empty() && phoneNumber.empty()) {
            continue;
        }

        break;
    }

    return VasStatus(NO_ERRORS);
}

VasStatus Smile::lookup(const VasStatus&)
{
    VasStatus response;
    iisys::JSObject obj;
    Payvice payvice;

    obj("username") = payvice.object(Payvice::USER).getString();
    obj("wallet") = payvice.object(Payvice::WALLETID).getString();
    obj("password") = payvice.object(Payvice::PASS).getString();
    obj("pin") = payvice.object(Payvice::PASS).getString();

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    if (!phoneNumber.empty()) {
        obj("account") = phoneNumber;
        response = comProxy.lookup("/smile/validate/phone", &obj);
    } else if (!customerID.empty()) {
        obj("account") = customerID;
        response = comProxy.lookup("/smile/validate", &obj);
    } else {
        return VasStatus(USER_CANCELLATION);
    }

    if (!obj.load(response.message)) {
        return VasStatus(INVALID_JSON, "Invalid Response");
    }

    return displayLookupInfo(obj);
}

VasStatus Smile::initiate(const VasStatus& lookupStatus)
{
    if (service == SMILETOPUP) {
        amount = getAmount("TOP-UP");
        if (amount <= 0) {
            return VasStatus(USER_CANCELLATION);
        }
    } else {
        iisys::JSObject obj;
        Payvice payvice;

        obj("username") = payvice.object(Payvice::USER).getString();
        obj("password") = payvice.object(Payvice::PASS).getString();
        obj("wallet") = payvice.object(Payvice::WALLETID).getString();

        Demo_SplashScreen("Loading Bundles...", "www.payvice.com");

        VasStatus response = comProxy.lookup("/smile/get-bundles", &obj);
        if (response.error) {
            return VasStatus(response.error, "Bundle lookup failed");
        }

        VasStatus check = bundleCheck(response);
        if (check.error) {
            return VasStatus(check.error, check.message.c_str());
        }
    }

    payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
    if (payMethod == PAYMENT_METHOD_UNKNOWN) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

Service Smile::vasServiceType()
{
    return service;
}

VasStatus Smile::complete(const VasStatus& initiateStatus)
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

std::map<std::string, std::string> Smile::storageMap(const VasStatus& completionStatus)
{
    std::map<std::string, std::string> record;
    char amountStr[16] = { 0 };


    sprintf(amountStr, "%lu", amount);

    if (service == SMILEBUNDLE) {
        record[VASDB_PRODUCT] = lookupResponse.selectedPackage("name").getString(); 
    } else if (service == SMILETOPUP) {
        record[VASDB_PRODUCT] = serviceToProductString(service); 
    }
    
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceToString(service);
    record[VASDB_AMOUNT] = amountStr;


    record[VASDB_BENEFICIARY_NAME] = lookupResponse.name;
    if (!customerID.empty()) {
        record[VASDB_BENEFICIARY] = customerID;
    }

    if (!phoneNumber.empty()) {
        record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
    }

    record[VASDB_PAYMENT_METHOD] = paymentString(payMethod);

    record[VASDB_REF] = paymentResponse.reference;
    record[VASDB_DATE] = paymentResponse.date;
    if (!paymentResponse.transactionSeq.empty()) {
        record[VASDB_TRANS_SEQ] = paymentResponse.transactionSeq;
    }

    if (payMethod == PAY_WITH_CARD) {
        if(cardPurchase.primaryIndex > 0) {
            char primaryIndex[16] = { 0 };
            sprintf(primaryIndex, "%lu", cardPurchase.primaryIndex);
            record[VASDB_CARD_ID] = primaryIndex;
        }
        
        if (!itexIsMerchant()) {
            record[VASDB_VIRTUAL_TID] = cardPurchase.purchaseTid;;
        }
    }

    record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::vasErrToTrxStatus(completionStatus.error));

    record[VASDB_STATUS_MESSAGE] = paymentResponse.message;

    return record;
}

VasStatus Smile::displayLookupInfo(const iisys::JSObject& data)
{
    iisys::JSObject name = data("customerName");

    iisys::JSObject status = data("status");
    if (status.isNull() || status.getInt() != 1) {
        iisys::JSObject msg =  data("message");
        return VasStatus(STATUS_ERROR, msg.isNull() ? "Status Error" : msg.getString().c_str());
    } else if (name.isNull()) {
        return VasStatus(KEY_NOT_FOUND, "Customer Name Not Found");
    }

    lookupResponse.name = name.getString();

    int i = UI_ShowOkCancelMessage(30000, "Please Confirm", lookupResponse.name.c_str(), UI_DIALOG_TYPE_CONFIRMATION);
    if (i != 0) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

VasStatus Smile::bundleCheck(const VasStatus& bundleCheck)
{
    iisys::JSObject lookupData;
    if (!lookupData.load(bundleCheck.message)) {
        return VasStatus(INVALID_JSON, "Invalid Response");
    }

    const iisys::JSObject& data = lookupData("bundles");
    
    if (data.isNull() || !data.isArray()) {
        return VasStatus(VAS_ERROR, "Bundles not found");
    }

    const size_t size = data.size();
    std::vector<std::string> menuData;
    for (size_t i = 0; i < size; ++i) {
        std::string amount = data[i]("price").getString();
        std::string validity = data[i]("validity").getString();

        formatAmount(amount);
        menuData.push_back(data[i]("name").getString() + menuendl() + amount + " Naira, " + validity);
    }

    int selection = UI_ShowSelection(60000, "Bundles", menuData, 0);

    if (selection < 0) {
        return VasStatus(USER_CANCELLATION);
    }

    lookupResponse.selectedPackage = data[selection];
    amount = lookupResponse.selectedPackage("price").getInt();
    
    return VasStatus(NO_ERRORS);
}

int Smile::getPaymentJson(iisys::JSObject& json, Service service)
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

    if (!phoneNumber.empty()) {
        json("account") = phoneNumber;
    } else if (!customerID.empty()) {
        json("account") = customerID;
    }

    json("code") = lookupResponse.selectedPackage("code").getString();
    if (service == SMILEBUNDLE) {
        json("price") = lookupResponse.selectedPackage("price").getString();
    } else if (service == SMILETOPUP) {
        char amountStr[16] = { 0 };
        snprintf(amountStr, sizeof(amountStr), "%lu", amount);
        json("amount") = amountStr;
    }

    json("username") = username;
    json("password") = password;
    json("wallet") = walletId;
    json("method") = paymentMethodStr;

    json("channel") = "POS";

    json("phone") = phoneNumber;

    if (payMethod == PAY_WITH_CARD && !itexIsMerchant()) {
        json("virtualTid") = cardPurchase.purchaseTid;
    }

    return 0;
}

VasStatus Smile::processPaymentResponse(iisys::JSObject& json, Service service)
{
    VasStatus response;
    iisys::JSObject status = json("status");
    iisys::JSObject msg =  json("message");

    this->service = service;

    if (status.isNull() || status.getInt() != 1) {
        response = VasStatus(STATUS_ERROR, msg.isNull() ? "Status Error" : msg.getString().c_str());
        // return response;
    }

    response.error = NO_ERRORS;
    if (!msg.isNull()) {
        paymentResponse.message = msg.getString();
        response.message = msg.getString();
    }

    iisys::JSObject ref = json("reference");
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

    if (payMethod == PAY_WITH_CARD && json("reversal").isBool() && json("reversal").getBool() == true) {
        comProxy.reverse(cardPurchase);
    }

    return response;
}

const char* Smile::paymentPath(Service service)
{
    if (service == SMILEBUNDLE) {
        return "/smile/buy-bundle";
    } else if (service == SMILETOPUP) {
        if (!phoneNumber.empty()) {
            return "/smile/top-up/phone";
        } else if (!customerID.empty()) {
            return "/smile/top-up";
        }
    }

    return "";
}

