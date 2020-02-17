#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <algorithm>

#include "simpio.h"
#include "pfm.h"
#include "cashio.h"

ViceBanking::ViceBanking(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
{
}

VasStatus ViceBanking::beginVas()
{
    Service services[] = {TRANSFER, WITHDRAWAL};
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    service = selectService(_title.c_str(), serviceVector);
    if (service == SERVICE_UNKNOWN) {
        return VasStatus(USER_CANCELLATION);
    }

    amount = getAmount(serviceToString(service));
    if (amount <= 0) {
        return VasStatus(USER_CANCELLATION);
    }

    if (service == TRANSFER) {
        beneficiary = beneficiaryAccount("Enter Beneficairy Account", "Transfer");
        if (beneficiary.empty()) {
            return VasStatus(USER_CANCELLATION);
        }

        Bank_t bankSelection = selectBank();
        if (bankSelection == BANKUNKNOWN) {
            return VasStatus(USER_CANCELLATION);
        }
        Bank bank = bankCode(bankSelection);
        vendorBankCode = bank.code;
        bankName = bank.name;
    } 

    return VasStatus(NO_ERRORS);
}

VasStatus ViceBanking::lookupCheck(const VasStatus& lookupStatus)
{
    iisys::JSObject data;
    iisys::JSObject status;

    if (!data.load(lookupStatus.message)) {
        return VasStatus(INVALID_JSON, "Invalid Response");
    }

    VasStatus errStatus = vasErrorCheck(data);
    if (errStatus.error) {
        return VasStatus(errStatus.error, errStatus.message.c_str());
    }

    status = data("status");
    if (status.isNull() || status.getInt() != 1) {
        iisys::JSObject msg =  data("message");
        return VasStatus(STATUS_ERROR, msg.isNull() ? "Status Error" : msg.getString().c_str());
    }

    return displayLookupInfo(data);
}

VasStatus ViceBanking::lookup(const VasStatus& beginStatus)
{
    VasStatus response;
    iisys::JSObject obj;
    
    obj("amount") = amount;
    obj("beneficiary") = beneficiary;
    obj("vendorBankCode") = vendorBankCode;
    obj("type") = "default";

    injectPayviceCredentials(obj);

    if (service == TRANSFER) {
        payMethod = PAY_WITH_CASH;
        Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
        response = comProxy.lookup("/vas/vice-banking/transfer/lookup", &obj, NULL /*&customHeaders*/);
    } else if (service == WITHDRAWAL) {
        payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_MCASH));
        Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
        if (payMethod == PAY_WITH_CARD) {
            response = comProxy.lookup("/vas/vice-banking/withdrawal/lookup", &obj);
        } else if (payMethod == PAY_WITH_MCASH) {
            response = comProxy.lookup("/vas/mcash-banking/withdrawal/lookup", &obj);
        } else {
            return VasStatus(USER_CANCELLATION);
        }
    }

    return lookupCheck(response);
}

VasStatus ViceBanking::initiate(const VasStatus& lookupStatus)
{
    phoneNumber = getPhoneNumber("Phone Number", "");
    if (phoneNumber.empty()) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

Service ViceBanking::vasServiceType()
{
    return service;
}

VasStatus ViceBanking::complete(const VasStatus& initiateStatus)
{
    std::string pin;
    iisys::JSObject json;
    VasStatus response;

    KeyChain keys;
    std::map<std::string, std::string> customHeaders;

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
    
    cashIOVasToken(json.dump().c_str(), NULL, &keys);
    customHeaders["ITEX-Nonce"] = keys.nonce;
    customHeaders["ITEX-Signature"] = keys.signature;


    Demo_SplashScreen("Payment In Progress", "www.payvice.com");

    if (payMethod == PAY_WITH_CARD) {
        cardPurchase.amount = amount;
        response = comProxy.complete(paymentPath(service), &json, &customHeaders, &cardPurchase);
    } else {
        response = comProxy.complete(paymentPath(service), &json, &customHeaders);
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

std::map<std::string, std::string> ViceBanking::storageMap(const VasStatus& completionStatus)
{
    std::map<std::string, std::string> record;
    std::string serviceStr(serviceToString(service));
    char amountStr[16] = { 0 };


    sprintf(amountStr, "%lu", amount);

    record[VASDB_PRODUCT] = serviceStr; 
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceStr;
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY_NAME] = lookupResponse.name;
    record[VASDB_BENEFICIARY] = beneficiary;
    record[VASDB_BENEFICIARY_PHONE] = phoneNumber;
    record[VASDB_PAYMENT_METHOD] = paymentString(payMethod);

    record[VASDB_REF] = paymentResponse.reference;
    record[VASDB_DATE] = paymentResponse.date;
    record[VASDB_TRANS_SEQ] = paymentResponse.transactionSeq;

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

    record[VASDB_SERVICE_DATA] = std::string("{\"recBank\": \"") + bankName + "\"}";

    return record;
}

ViceBanking::Bank ViceBanking::bankCode(Bank_t bank)
{
    switch (bank) {
    case GTB:
        return Bank("GTB", "058152052");
    case ACCESS:
        return Bank("ACCESS BANK PLC", "044150149");
    case CITI:
        return Bank("CITI BANK", "023150005");
    case DIAMOND:
        return Bank("DIAMOND BANK PLC", "063150162");
    case ECOBANK:
        return Bank("ECOBANK NIGERIA", "050150311");
    case ENTERPRISE:
        return Bank("ENTERPRISE BANK", "084150015");
    case ETB:
        return Bank("ETB", "040150101");
    case FCMB:
        return Bank("FCMB PLC", "214150018");
    case FIDELITY:
        return Bank("FIDELITY BANK PLC", "070150003");
    case FIRSTBANK:
        return Bank("FIRST BANK PLC", "011152303");
    case HERITAGE:
        return Bank("HERITAGE BANK", "030159992");
    case JAIZ:
        return Bank("JAIZ BANK", "301080020");
    case KEYSTONE:
        return Bank("KEYSTONE BANK PLC", "082150017");
    case MAINSTREET:
        return Bank("MAIN STREET BANK", "014150030");
    case NIB:
        return Bank("NIB", "023150005");
    case POLARIS:
        return Bank("POLARIS BANK PLC", "076151006");
    case STANBIC:
        return Bank("STANBIC IBTC BANK", "221159522");
    case STANDARDCHARTERED:
        return Bank("STANDARD CHARTERED", "068150057");
    case STERLING:
        return Bank("STERLING BANK PLC", "232150029");
    case UBA:
        return Bank("UBA PLC", "033154282");
    case UNION:
        return Bank("UNION BANK PLC", "032156825");
    case UNITY:
        return Bank("UNITY BANK PLC", "215082334");
    case WEMA:
        return Bank("WEMA BANK PLC", "035150103");
    case ZENITH:
        return Bank("ZENITH BANK PLC", "057150013");
    default:
        return Bank("", "");
    }
}

ViceBanking::Bank_t ViceBanking::selectBank()
{
    int selection;
    std::vector<std::string> menu;

    for (size_t i = 0; i < ViceBanking::BANK_TOTAL; ++i) {
        menu.push_back(bankCode(static_cast<Bank_t>(i)).name);
    }

    selection = UI_ShowSelection(30000, "Banks", menu, 0);

    return static_cast<ViceBanking::Bank_t>(selection);
}

std::string ViceBanking::beneficiaryAccount(const char* title, const char* prompt)
{
    char account[16] = { 0 };

    if (getNumeric(account, 10, 10, 0, 30000, title, prompt, UI_DIALOG_TYPE_NONE) < 0) {
        return std::string("");
    }

    return std::string(account);
}

const char* ViceBanking::paymentPath(Service service)
{
    if (service == TRANSFER) {
        return "/vas/vice-banking/transfer/payment";
    } else if (service == WITHDRAWAL) {
        if (payMethod == PAY_WITH_CARD) {
            return "/vas/vice-banking/withdrawal/payment";
        } else if (payMethod == PAY_WITH_MCASH) {
            return "/vas/mcash-banking/withdrawal/payment";
        }
    }

    return "";
}

VasStatus ViceBanking::processPaymentResponse(iisys::JSObject& json, Service service)
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

VasStatus ViceBanking::displayLookupInfo(const iisys::JSObject& data)
{
    std::ostringstream confirmationMessage;

    iisys::JSObject name          = data("beneficiaryName");
    iisys::JSObject convenience   = data("convenienceFee");
    iisys::JSObject amountSettled = data("amountSettled");
    iisys::JSObject amountToDebit = data("amountToDebit");
    iisys::JSObject productCode   = data("productCode");

    if (!name.isNull()) {
        lookupResponse.name = name.getString();
    }

    if (!productCode.isNull()) {
        lookupResponse.productCode = productCode.getString();
    }

    lookupResponse.convenience   = convenience.getInt();
    lookupResponse.amountSettled = amountSettled.getInt();
    lookupResponse.amountToDebit = amountToDebit.getInt();

    if (service == TRANSFER) {
        if (lookupResponse.name.empty()) {
            return VasStatus(KEY_NOT_FOUND, "Name not found");
        }

        confirmationMessage << lookupResponse.name << std::endl;

        iisys::JSObject element = data("account");
        if (!element.isNull()) {
            confirmationMessage << "NUBAN: " << element.getString() << std::endl;
        }

        confirmationMessage << "Amount: " << amount / 100.0f << std::endl;
        confirmationMessage << "Convenience: " << lookupResponse.convenience / 100.0f << std::endl;
    } else if (service == WITHDRAWAL) {
        confirmationMessage << "Amount: " << lookupResponse.amountToDebit / 100.0f << std::endl;
        confirmationMessage << "Convenience: " << lookupResponse.convenience / 100.0f << std::endl;        
    }

    int i = UI_ShowOkCancelMessage(30000, "Please Confirm", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

int ViceBanking::getPaymentJson(iisys::JSObject& json, Service service)
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
    json("username") = username;
    json("password") = password;
    json("wallet") = walletId;
    json("paymentMethod") = paymentMethodStr;

    json("type") = "default";
    json("channel") = "POS";
    json("amount") = amount;
    json("service") = serviceToProductString(service);

    json("phone") = phoneNumber;

    if (service == TRANSFER) {
        json("beneficiary") = beneficiary;
        json("vendorBankCode") = vendorBankCode;
    }

    json("productCode") = lookupResponse.productCode;

    return 0;
}
