#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "simpio.h"
#include "pfm.h"

#include "vasdb.h"

#include "paytv.h"

PayTV::PayTV(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
{
}

VasStatus PayTV::beginVas()
{
    Service services[] = { DSTV, GOTV, STARTIMES };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    while (1) {
        service = selectService("Services", serviceVector);
        if (service == SERVICE_UNKNOWN) {
            return VasStatus(USER_CANCELLATION);
        }

        iuc = getNumeric(0, 60000, "IUC Number", serviceToString(service), UI_DIALOG_TYPE_NONE);

        if (!iuc.empty()) {
            break;
        }
    }


    return VasStatus(NO_ERRORS);
}

VasStatus PayTV::lookup(const VasStatus&)
{
    VasStatus response;
    iisys::JSObject obj;

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    if (service == DSTV || service == GOTV) {
        obj("iuc") = iuc;
        obj("unit") = serviceToProductString(service);
        response = comProxy.lookup("/vas/multichoice/lookup", &obj);
        return multichoiceLookupCheck(response);
    } else if (service == STARTIMES) {
        Payvice payvice;

        obj("username") = payvice.object(Payvice::USER).getString();
        obj("wallet") = payvice.object(Payvice::WALLETID).getString();
        obj("type") = "default";
        obj("channel") = "POS";
        obj("smartCardCode") = iuc;
        response = comProxy.lookup("/vas/startimes/validation", &obj);
        return startimesLookupCheck(response);
    }
    
    return VasStatus(VAS_ERROR);
}

VasStatus PayTV::initiate(const VasStatus& lookupStatus)
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

Service PayTV::vasServiceType()
{
    return service;
}

VasStatus PayTV::complete(const VasStatus& initiateStatus)
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

std::map<std::string, std::string> PayTV::storageMap(const VasStatus& completionStatus)
{
    std::map<std::string, std::string> record;
    char amountStr[16] = { 0 };


    sprintf(amountStr, "%lu", amount);

    if (service == DSTV || service == GOTV) {
        record[VASDB_PRODUCT] = multichoice.selectedPackage("name").getString(); 
        record[VASDB_BENEFICIARY_NAME] = multichoice.name;
    } else if (service == STARTIMES) {
        record[VASDB_PRODUCT] = startimes.bouquet; 
        record[VASDB_BENEFICIARY_NAME] = startimes.name;
    }
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

    if (cardPurchase.primaryIndex > 0) {
        char primaryIndex[16] = { 0 };
        sprintf(primaryIndex, "%lu", cardPurchase.primaryIndex);
        record[VASDB_CARD_ID] = primaryIndex;
    }

    if (completionStatus.error == NO_ERRORS) {
        record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::APPROVED);
    } else {
        record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::DECLINED);
    }
    record[VASDB_STATUS_MESSAGE] = paymentResponse.message;

    return record;
}

VasStatus PayTV::startimesLookupCheck(const VasStatus& lookupStatus)
{
    iisys::JSObject name, balance, bouquet, productCode;
    std::ostringstream confirmationMessage;

    iisys::JSObject lookupData;
    if (!lookupData.load(lookupStatus.message)) {
        return VasStatus(INVALID_JSON, "Invalid Response");
    }

    VasStatus status = vasErrorCheck(lookupData);
    if (status.error) {
        return status;
    }

    name = lookupData("name");
    bouquet = lookupData("bouquet");
    balance = lookupData("balance");
    productCode = lookupData("productCode");

    if (name.isNull() || bouquet.isNull()) {
        return VasStatus(KEY_NOT_FOUND, "Name or Bouquet not found");
    }

    startimes.name = name.getString();
    startimes.balance = balance.getString();
    startimes.bouquet = bouquet.getString();
    startimes.productCode = productCode.getString();

    confirmationMessage << startimes.name << std::endl << std::endl;
    confirmationMessage << "Bouquet: " << bouquet.getString() << std::endl;
    confirmationMessage << "Balance: " << balance.getString() << std::endl;
    
    while (1) {
        int i = UI_ShowOkCancelMessage(30000, "Confirm Info", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
        if (i != 0) {
            return VasStatus(USER_CANCELLATION);
        }

        amount = getAmount("Startimes");
        if (amount) {
            break;
        }
    }

    return VasStatus(NO_ERRORS);
}

VasStatus PayTV::multichoiceLookupCheck(const VasStatus& lookupStatus)
{
    iisys::JSObject lookupData;
    if (!lookupData.load(lookupStatus.message)) {
        return VasStatus(INVALID_JSON, "Invalid Response");
    }

    VasStatus status = vasErrorCheck(lookupData);
    if (status.error) {
        return status;
    }

    iisys::JSObject name, account;
    std::ostringstream confirmationMessage;

    name = lookupData("name");
    account = lookupData("account");

    if (name.isNull()) {
        return VasStatus(KEY_NOT_FOUND, "Name not found");
    }

    multichoice.name = name.getString();

    confirmationMessage << multichoice.name << std::endl;
    if (!account.isNull() && iuc != account.getString()) {
        return VasStatus(VAS_ERROR, "Account mismatch");
    }

    confirmationMessage << "Account: " << std::endl << iuc;
    
    int i = UI_ShowOkCancelMessage(30000, "Confirm Info", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        return VasStatus(USER_CANCELLATION);
    }

    const iisys::JSObject& data = lookupData("data");
    
    if (data.isNull() || !data.isArray()) {
        return VasStatus(VAS_ERROR, "Data Packages not found");
    }

    const size_t size = data.size();
    std::vector<std::string> menuData;
    for (i = 0; i < size; ++i) {
        menuData.push_back(data[i]("name").getString() + menuendl() + data[i]("amount").getString() + " Naira");
    }

    int selection = UI_ShowSelection(60000, "Data Packages", menuData, 0);

    if (selection < 0) {
        return VasStatus(USER_CANCELLATION);
    }

    multichoice.selectedPackage = data[selection];
    multichoice.unit = lookupData("unit").getString();
    amount = multichoice.selectedPackage("amount").getInt() * 100;
    
    return VasStatus(NO_ERRORS);
}

int PayTV::getPaymentJson(iisys::JSObject& json, Service service)
{
    Payvice payvice;

    if (!loggedIn(payvice)) {
        return -1;
    }

    const std::string username = payvice.object(Payvice::USER).getString();
    const std::string password = payvice.object(Payvice::PASS).getString();
    const std::string walletId = payvice.object(Payvice::WALLETID).getString();


    if (service == DSTV || service == GOTV) {
        json("tid") = getDeviceTerminalId();
        json("user_id") = username;
        json("terminal_id") = walletId;

        json("iuc") = iuc;
        json("unit") = multichoice.unit;
        json("product_code") = multichoice.selectedPackage("product_code").getString();

    } else if (service == STARTIMES) {
        std::string paymentMethodStr = paymentString(payMethod);
        std::transform(paymentMethodStr.begin(), paymentMethodStr.end(), paymentMethodStr.begin(), ::tolower);

        json("wallet") = walletId;
        json("username") = username;
        json("password") = password;
        json("type") = "default";
        json("smartCardCode") = iuc;
        json("customerName") = startimes.name;
        json("productCode") = startimes.productCode;
        json("bouquet") = startimes.bouquet;
        json("amount") = amount;
        json("paymentMethod") = paymentMethodStr;
    }

    json("channel") = "POS";
    json("phone") = phoneNumber;
    
    return 0;
}

VasStatus PayTV::processPaymentResponse(iisys::JSObject& json, Service service)
{
    VasStatus response = vasErrorCheck(json);
    paymentResponse.message = response.message;

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

    return response;
}

const char* PayTV::paymentPath(Service service)
{
    if (service == DSTV || service == GOTV) {
        if (payMethod == PAY_WITH_CARD) {
            return "/vas/card/multichoice/pay";
        } else if (payMethod == PAY_WITH_CASH) {
            return "/vas/multichoice/pay";
        }
    } else if (service == STARTIMES) {
        return "/vas/startimes/payment";
    }
    return "";
}
