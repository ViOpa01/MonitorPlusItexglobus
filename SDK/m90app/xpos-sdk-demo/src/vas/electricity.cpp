#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>



#include "simpio.h"
#include "pfm.h"
#include "vasdb.h"
#include "electricity.h"

extern "C" {
#include "../merchant.h"
}

Electricity::Electricity(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
{
}

VasStatus Electricity::beginVas()
{
    Service services[] = { IKEJA, EKEDC, EEDC, IBEDC, PHED, AEDC, KEDCO };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    service = selectService("Distri. Companies", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        return VasStatus(USER_CANCELLATION);
    }

    energyType = getEnergyType(service, serviceToString(service));
    if (energyType == TYPE_UNKNOWN) {
        return VasStatus(USER_CANCELLATION);
    }

    meterNo = getMeterNo(serviceToString(service));
    if (meterNo.empty()) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

Electricity::EnergyType Electricity::getEnergyType(Service service, const char* title)
{
    int selection;
    const char* energyTypes[] = { "Token", "Postpaid" };
    std::vector<std::string> menu(energyTypes, energyTypes + sizeof(energyTypes) / sizeof(char*));

    if (service == EKEDC) {
        return GENERIC_ENERGY;
    }

    selection = UI_ShowSelection(30000, title, menu, 0);
    switch (selection) {
    case 0:
        return PREPAID_TOKEN;
    case 1:
        return POSTPAID;
    default:
        return TYPE_UNKNOWN;
    }
}

VasStatus Electricity::lookupCheck(const VasStatus& lookupStatus)
{
    iisys::JSObject data;
    std::ostringstream confirmationMessage;

    if (lookupStatus.error) {
        return VasStatus(LOOKUP_ERROR, lookupStatus.message.c_str());
    }

    if (!data.load(lookupStatus.message)) {
        return VasStatus(INVALID_JSON, "Invalid Response");
    }

    VasStatus status = vasErrorCheck(data);
    if (status.error) {
        return status;
    }

    iisys::JSObject name, address, minAmount, pCode;

    name = data("name");
    address = data("address");
	minAmount = data("minimumPayableAmount");
	pCode = data("productCode");

    if (name.isNull()) {
        return VasStatus(KEY_NOT_FOUND, "Name not found");
    }

    lookupResponse.name = name.getString();

    confirmationMessage << lookupResponse.name << std::endl << std::endl;
    if (!address.isNull()) {
        confirmationMessage << "Addr:" << std::endl << address.getString();
        lookupResponse.address = address.getString();
    }

    lookupResponse.minPayableAmount = !minAmount.isNull() ? (unsigned long) (minAmount.getNumber() * 100.0) : 0;

    lookupResponse.productCode = !pCode.isNull() ? pCode.getString() : "";
    
    int i = UI_ShowOkCancelMessage(30000, "Confirm Info", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        return VasStatus(USER_CANCELLATION);
    }

    return VasStatus(NO_ERRORS);
}

VasStatus Electricity::lookup(const VasStatus& beginStatus)
{
    VasStatus response;
    iisys::JSObject obj;
    
    if (getLookupJson(obj, service) < 0) {
        return VasStatus(VAS_ERROR, "Data Error");
    }

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    if (service == EKEDC) {
        response = comProxy.lookup("/ekedc/vas/validate", &obj);
    } else if (service == IKEJA) {
        response = comProxy.lookup("/vas/ie/validate", &obj);
    } else if (service == EEDC) {
        response = comProxy.lookup("/vas/eedc/validation", &obj);
    } else if (service == IBEDC) {
        response = comProxy.lookup("/vas/ibedc/validation", &obj);
    } else if (service == PHED) {
        response = comProxy.lookup("/vas/phed/validation", &obj);
    } else if (service == AEDC) {
        response = comProxy.lookup("/vas/abuja/validation", &obj);
    } else if (service == KEDCO) {
        response = comProxy.lookup("/vas/kedco/validation", &obj);
    }

    return lookupCheck(response);
}

VasStatus Electricity::initiate(const VasStatus& lookupStatus)
{

    while (1) {
        amount = getAmount(serviceToString(service));
        if (amount <= 0) {
            return VasStatus(USER_CANCELLATION);
        } else if (amount < lookupResponse.minPayableAmount) {
            char message[128] = { 0 };
            sprintf(message, "Amount must exceed %.2f", lookupResponse.minPayableAmount / 100.0);
            UI_ShowButtonMessage(30000, "Increase Amount", message, "OK", UI_DIALOG_TYPE_WARNING);
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

Service Electricity::vasServiceType()
{
    return service;
}

VasStatus Electricity::complete(const VasStatus& initiateStatus)
{
    std::string pin;
    iisys::JSObject json;
    VasStatus response;


    switch (getPin(pin, "Payvice Pin")) {
    case EMV_CT_PIN_INPUT_ABORT:
        response.error = INPUT_ABORT;
        return response;
    case EMV_CT_PIN_INPUT_TIMEOUT:
        response.error = INPUT_TIMEOUT_ERROR;
        return response;
    case EMV_CT_PIN_INPUT_OTHER_ERR:
        response.error = INPUT_ERROR;
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

std::map<std::string, std::string> Electricity::storageMap(const VasStatus& completionStatus)
{
    std::map<std::string, std::string> record;
    char amountStr[16] = { 0 };

    sprintf(amountStr, "%lu", amount);

    record[VASDB_PRODUCT] = productString(energyType);
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceToString(service);
    record[VASDB_AMOUNT] = amountStr;
    record[VASDB_BENEFICIARY] = meterNo;
    record[VASDB_BENEFICIARY_NAME] = lookupResponse.name;
    record[VASDB_BENEFICIARY_ADDR] = lookupResponse.address;
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
            record[VASDB_VIRTUAL_TID] = cardPurchase.purchaseTid;;
        }
    }

    record[VASDB_STATUS] = VasDB::trxStatusString(VasDB::vasErrToTrxStatus(completionStatus.error));

    record[VASDB_STATUS_MESSAGE] = paymentResponse.message;
    record[VASDB_SERVICE_DATA] = paymentResponse.serviceData;


    return record;
}

const char* Electricity::payloadServiceTypeString(EnergyType energyType)
{
    switch (energyType) {
    case PREPAID_TOKEN:
    case PREPAID_SMARTCARD:
        return "vend";
    case POSTPAID:
        return "pay";
    default:
        return "";
    }
}

int Electricity::getLookupJson(iisys::JSObject& json, Service service)
{
    Payvice payvice;
    char serviceType[16] = { 0 };
    char request[4096] = { 0 };
    char tid[16] = { 0 };

    if (!loggedIn(payvice)) {
        return -1;
    }

    const std::string& username = payvice.object(Payvice::USER).getString();
    const std::string& password = payvice.object(Payvice::PASS).getString();
    const std::string& walletId = payvice.object(Payvice::WALLETID).getString();


    strncpy(tid, getDeviceTerminalId(), sizeof(tid) - 1);

    strcpy(serviceType, payloadServiceTypeString(energyType));

    if (service == EKEDC) {
        sprintf(request, "{\"terminal\": \"%s\", \"terminal_id\": \"%s\", \"user_id\": \"%s\", \"password\": \"%s\", \"meter\": \"%s\", \"account\": \"%s\", \"type\": \"getcus\", \"service_type\": \"%s\"}"
        , tid, walletId.c_str(), username.c_str(), password.c_str(), meterNo.c_str(), "", serviceType);
    } else if (service == IKEJA) {
        int useMeterNo = (energyType == PREPAID_TOKEN) ? 1 : 0;
        sprintf(request, "{\"terminal\": \"%s\", \"terminal_id\": \"%s\", \"user_id\": \"%s\", \"password\": \"%s\", \"meter\": \"%s\", \"account\": \"%s\", \"type\": \"getcus\", \"service_type\": \"%s\"}"
        , tid, walletId.c_str(), username.c_str(), password.c_str(), (useMeterNo) ? meterNo.c_str() : "", (!useMeterNo) ? meterNo.c_str() : "", serviceType);
    } else if (service == EEDC) {
        sprintf(request, "{\"terminalId\": \"%s\", \"wallet\": \"%s\", \"username\": \"%s\", \"type\": \"%s\", \"channel\": \"POS\", \"account\": \"%s\"}"
        , tid, walletId.c_str(), username.c_str(), (energyType == PREPAID_TOKEN) ? "prepaid" : "postpaid", meterNo.c_str());
    } else if (service == IBEDC) {
        sprintf(request, "{\"wallet\": \"%s\", \"username\": \"%s\", \"type\": \"%s\", \"channel\": \"POS\", \"account\": \"%s\"}"
        , walletId.c_str(), username.c_str(), (energyType == PREPAID_TOKEN) ? "prepaid" : "postpaid", meterNo.c_str());
    } else if (service == PHED) {
        sprintf(request, "{\"terminalId\": \"%s\", \"wallet\": \"%s\", \"username\": \"%s\", \"type\": \"%s\", \"channel\": \"POS\", \"account\": \"%s\"}"
        , tid, walletId.c_str(), username.c_str(), (energyType == PREPAID_TOKEN) ? "prepaid" : "postpaid", meterNo.c_str());
    } else if (service == AEDC) {
        sprintf(request, "{\"wallet\": \"%s\", \"username\": \"%s\", \"terminalId\": \"%s\", \"requestType\": \"0\", \"meterType\": \"%s\", \"meterNo\": \"%s\", \"channel\": \"POS\"}"
        , walletId.c_str(), username.c_str(), tid, abujaMeterType(energyType), meterNo.c_str());
    } else if (service == KEDCO) {
        sprintf(request, "{\"terminalId\": \"%s\", \"wallet\": \"%s\", \"username\": \"%s\", \"type\": \"%s\", \"channel\": \"POS\", \"account\": \"%s\"}"
        , tid, walletId.c_str(), username.c_str(), (energyType == PREPAID_TOKEN) ? "prepaid" : "postpaid", meterNo.c_str());
    }

    if (!json.load(request)) {
        return -1;
    }

    return 0;
}

int Electricity::getPaymentJson(iisys::JSObject& json, Service service)
{
    Payvice payvice;
    char tid[16] = { 0 };

    if (!loggedIn(payvice)) {
        return -1;
    }

    std::string paymentMethodStr = paymentString(payMethod);
    std::transform(paymentMethodStr.begin(), paymentMethodStr.end(), paymentMethodStr.begin(), ::tolower);

    const std::string username = payvice.object(Payvice::USER).getString();
    const std::string password = payvice.object(Payvice::PASS).getString();
    const std::string walletId = payvice.object(Payvice::WALLETID).getString();
    strncpy(tid, getDeviceTerminalId(), sizeof(tid) - 1);

    json("terminal") = tid;;
	if (service == EKEDC || service == IKEJA) {
		json("terminal_id") = walletId;
		json("user_id") = username;

		json("meter") = (energyType == PREPAID_TOKEN || service == EKEDC) ? meterNo : "";
		if (energyType == POSTPAID || energyType == PREPAID_SMARTCARD) {
			json("account") = meterNo;
		} else {
			json("account") = "";
		}
		json("type") = service == IKEJA ? "getcus" : paymentMethodStr;
		json("service_type") = payloadServiceTypeString(energyType);
        json("paymentMethod") = paymentMethodStr;
	} else if (service == EEDC || service == IBEDC || service == PHED || service == KEDCO) {
		json("wallet") = walletId;
		json("username") = username;
		json("account") = meterNo;
		json("type") = (energyType == PREPAID_TOKEN) ? "prepaid" : "postpaid";
		json("productCode") = lookupResponse.productCode;
		json("customerName") = lookupResponse.name;
		json("paymentMethod") = paymentMethodStr;
	} else if (service == AEDC) {
		json("wallet") = walletId;
		json("username") = username;
		json("terminalId") = tid;
		json("requestType") = "1";
		json("meterType") = abujaMeterType(energyType);
		json("meterNo") = meterNo;
		json("productCode") = lookupResponse.productCode;
		json("customerName") = lookupResponse.name;
		json("paymentMethod") = paymentMethodStr;
	}

	json("channel") = "POS";

    if (service == EEDC || service == IBEDC || service == PHED || service == AEDC || service == KEDCO) {
		json("amount") = amount;
	} else {
        json("amount") = amount / 100;
	}
    

    json("phone") = phoneNumber;
    // json("pin") = encPin;
	json("password") = password;

    if (payMethod == PAY_WITH_CARD && !itexIsMerchant()) {
        json("virtualTID") = payvice.object(Payvice::VIRTUAL)(Payvice::TID); 
    }

    return 0;
}

VasStatus Electricity::processPaymentResponse(iisys::JSObject& json, Service service)
{
    const char* keys[] = {"account_type", "type", "tran_id", "client_id", "sgc", "msno", "krn", "ti", "tt", "unit"
		, "sgcti", "accountNo", "tariffCode", "rate", "units", "region", "token", "unit_value", "unit_cost", "vat", "agent"
		, "arrears", "receipt_no", "invoiceNumber", "tariff", "lastTxDate", "collector", "csp", NULL };

    VasStatus response = vasErrorCheck(json);
    paymentResponse.message = response.message;

    this->service = service;

    iisys::JSObject ref = json("ref");
    if (!ref.isNull()) {
        paymentResponse.reference = ref.getString();
    } else {
        ref = json("reference");
        if(!ref.isNull()) {
            paymentResponse.reference = ref.getString();
        }
    }


    iisys::JSObject date = json("date");
    if (!date.isNull()) {
        paymentResponse.date = date.getString() + ".000";
    }

    iisys::JSObject seq = json("transactionID");
    if (!seq.isNull()) {
        paymentResponse.transactionSeq = seq.getString();
    }

    iisys::JSObject data;
    iisys::JSObject temp;
    
    temp = json("address");
    if (!temp.isNull() && !temp.getString().empty()) {
        lookupResponse.address = temp.getString();
    }

    for (size_t i = 0; keys[i] != NULL; ++i) {
        temp = json(keys[i]);
        if (!temp.isNull() && !temp.getString().empty()) {
            data(keys[i]) = temp;
        }
    }

    if (!data.isNull())
        paymentResponse.serviceData = data.getString();

    if (payMethod == PAY_WITH_CARD && json("reversal").isBool() && json("reversal").getBool() == true) {
        comProxy.reverse(cardPurchase);
    }

    return response;
}

std::string Electricity::getMeterNo(const char* title)
{
    char number[16] = { 0 };
    std::string prompt;

    if (energyType == PREPAID_TOKEN) {
        prompt = "Meter No";
    } else if (energyType == POSTPAID) {
        prompt = "Account No";
    }

    if (getNumeric(number, 6, sizeof(number) - 1, 0, 30000, prompt.c_str(), title, UI_DIALOG_TYPE_NONE) < 0) {
        return std::string("");
    }

    return std::string(number);
}

const char* Electricity::abujaMeterType(EnergyType type)
{
	switch (type) {
	case PREPAID_TOKEN:
		return "0";
	case PREPAID_SMARTCARD:
		return "1";
	case POSTPAID:
		return "2";
    default:
	    return "-1";
    }
}

const char* Electricity::productString(EnergyType type)
{
	switch (type) {
	case PREPAID_TOKEN:
		return "PREPAID TOKEN";
	case PREPAID_SMARTCARD:
		return "PREPAID SMARTCARD";
	case POSTPAID:
		return "POSTPAID";
    default:
	    return "";
    }
}

const char* Electricity::paymentPath(Service service)
{
    if (payMethod == PAY_WITH_CARD) {
        switch (service) {
        case IKEJA:
            return "/vas/card/ie/purchase";
        case EKEDC:
            return "/ekedc/vas/payment/process";
        case EEDC:
            return "/vas/eedc/payment";
        case IBEDC:
            return "/vas/ibedc/payment";
        case PHED:
            return "/vas/phed/payment";
        case AEDC:
            return "/vas/abuja/payment";
        case KEDCO:
            return "/vas/kedco/payment";
        default:
            return "";
        }
    } else if (payMethod == PAY_WITH_CASH) {
        switch (service) {
        case IKEJA:
            return "/vas/ie/purchase";
        case EKEDC:
            return "/ekedc/vas/payment/process";
        case EEDC:
            return "/vas/eedc/payment";
        case IBEDC:
            return "/vas/ibedc/payment";
        case PHED:
            return "/vas/phed/payment";
        case AEDC:
            return "/vas/abuja/payment";
        case KEDCO:
            return "/vas/kedco/payment";
        default:
            return "";
        }
    }

    return "";
}
