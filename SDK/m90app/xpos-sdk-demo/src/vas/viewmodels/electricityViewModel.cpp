#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <ctype.h>
#include <math.h>
#include <iostream>

#include "platform/platform.h"
#include "payvice.h"
#include "virtualtid.h"

#include "electricityViewModel.h"

#include "vasdb.h"
#include "nqr.h"

ElectricityViewModel::ElectricityViewModel(const char* title, VasComProxy& proxy)
    : title(title)
    , comProxy(proxy)
    , service(SERVICE_UNKNOWN)
{
}

VasResult ElectricityViewModel::lookupCheck(const VasResult& lookupStatus)
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

    const iisys::JSObject& name = responseData("name");
    const iisys::JSObject& address = responseData("address");
	const iisys::JSObject& minAmount = responseData("minimumAmount");
	const iisys::JSObject& pCode = responseData("productCode");
	const iisys::JSObject& arrears = responseData("customerArrears");
    

    if (!name.isString()) {
        result = VasResult(KEY_NOT_FOUND, "Name not found");
        return result;
    }

    lookupResponse.name = name.getString();

    if (address.isString()) {
        lookupResponse.address = address.getString();
    }

    lookupResponse.minPayableAmount = minAmount.isString() ? (unsigned long) lround(minAmount.getNumber() * 100.0) : 0;
    lookupResponse.arrears = arrears.isString() ? (unsigned long) lround(arrears.getNumber() * 100.0) : 0;

    if (!pCode.isString()) {
        result.error = VAS_ERROR;
        return result;
    }

    lookupResponse.productCode = pCode.getString();
    

    return VasResult(NO_ERRORS);
}

VasResult ElectricityViewModel::lookup()
{
    VasResult response;
    iisys::JSObject obj;
    
    if (getLookupJson(obj, service) < 0) {
        return VasResult(VAS_ERROR, "Data Error");
    }

    response = comProxy.lookup(lookupPath(), &obj);

    return lookupCheck(response);;
}

VasResult ElectricityViewModel::initiate(const std::string& pin)
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

    json = createNqrJSON(getAmount(), "ELECTRICITY", json);
    response = comProxy.initiate(nqrGenerateUrl(), &json);

    if (response.error != NO_ERRORS) {
        return response;
    }

    response = parseVasQr(lookupResponse.productCode, response.message);

    return response;
}

VasResult ElectricityViewModel::setAmount(unsigned long amount)
{
    this->amount = amount;
    return VasResult(NO_ERRORS);
}

VasResult ElectricityViewModel::complete(const std::string& pin)
{
    iisys::JSObject json;
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
        cardData.refcode = getRefCode(serviceToProductString(service), productString(energyType));
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

    if (energyType == PREPAID_SMARTCARD && response.error == NO_ERRORS) {

        const iisys::JSObject& responseData = json("data");
        const iisys::JSObject& unitValue = responseData("unit_value");
        if (!unitValue.isNull()) {
            userCardInfo.CM_Purchase_Power = unitValue.getNumber();
            
            response = updateSmartCard(&userCardInfo);
        }
    }


    return response;
}

std::map<std::string, std::string> ElectricityViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const std::string amountStr = vasimpl::to_string(this->amount);

    record[VASDB_PRODUCT] = productString(energyType);
    record[VASDB_SERVICE] = serviceToString(service);
    record[VASDB_CATEGORY] = title;
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

VasResult ElectricityViewModel::setService(Service service)
{
    VasResult result;

    switch (service) {
    case IKEJA:
    case EKEDC:
    case EEDC:
    case IBEDC:
    case PHED:
    case AEDC:
    case KEDCO:
        result.error = NO_ERRORS;
        this->service = service;
        break;
    default:
        break;
    }

    return result;
}

VasResult ElectricityViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (payMethod == PAY_WITH_CARD || payMethod == PAY_WITH_CASH || payMethod == PAY_WITH_NQR) {
        result.error = NO_ERRORS;
        this->payMethod = payMethod;
    }   

    return result;
}

VasResult ElectricityViewModel::setPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (!phoneNumber.empty()) {
        result.error = NO_ERRORS;
        this->phoneNumber = phoneNumber;
    }

    return result;
}

VasResult ElectricityViewModel::setAgentPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (!phoneNumber.empty()) {
        result.error = NO_ERRORS;
        agentPhoneNumber = phoneNumber;
    }

    return result;
}

VasResult ElectricityViewModel::setEnergyType(EnergyType energyType)
{   
    VasResult result;

    if (energyType != TYPE_UNKNOWN) {
        result.error = NO_ERRORS;
        this->energyType = energyType;
    }

    return result;
}

VasResult ElectricityViewModel::setMeterNo(const std::string& meterNo)
{
    VasResult result;

    if (!meterNo.empty()) {
        result.error = NO_ERRORS;
        this->meterNo = meterNo;
    }

    return result;
}

Service ElectricityViewModel::getService() const
{
    return service;
}

unsigned long ElectricityViewModel::getAmount() const
{
    return amount;
}

PaymentMethod ElectricityViewModel::getPaymentMethod() const
{
    return payMethod;
}

ElectricityViewModel::EnergyType ElectricityViewModel::getEnergyType() const
{
    return energyType;
}

int ElectricityViewModel::getLookupJson(iisys::JSObject& json, Service service) const
{
    json.clear();

    json("meterNo") = meterNo;
    json("accountType") = getEnergyTypeString(energyType);
    json("service") = apiServiceString(service);
    json("amount") = majorDenomination(amount);
    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();
    if (service == IKEJA) {
        json("customerPhone") = phoneNumber;
        json("agentPhone") = agentPhoneNumber;
    }

    return 0;
}

int ElectricityViewModel::getPaymentJson(iisys::JSObject& json, Service service) const
{
    std::string paymentMethodStr = apiPaymentMethodString(payMethod);


    json("paymentMethod") = paymentMethodStr;
    json("productCode") = lookupResponse.productCode;
	json("service") = apiServiceString(service);
	json("customerPhoneNumber") = phoneNumber;

    // purchase times
    if (energyType == PREPAID_SMARTCARD) {
	    json("purchaseTimes") = purchaseTimes;
	    // json("psamBalance") = psamBalance;   // add psamBalance here 
    }

    return 0;
}

void ElectricityViewModel::addToken(iisys::JSObject& serviceData, const iisys::JSObject& responseData) const
{
    bool isTokenListAllNull = true;
    iisys::JSObject tokenList;
    const iisys::JSObject& tokenTemp = responseData("tokenList");

    if (tokenTemp.isObject()) {
        tokenList = tokenTemp;
    } else if (tokenTemp.isString()) {
        tokenList.load(tokenTemp.getString());
    }

    if (tokenList.isObject()) {
        const char* tokenTags[] = {"token1", "token2", "token3", NULL};
		const char* tokenDescTags[] = {"token1_desc", "token2_desc", "token3_desc", NULL};

		assert(sizeof(tokenTags) == sizeof(tokenDescTags));

        for (size_t i = 0; tokenTags[i]; ++i) {
            const iisys::JSObject& token = tokenList(tokenTags[i]);
            const iisys::JSObject& tokenDesc = tokenList(tokenDescTags[i]);

            if (token.isString() && tokenDesc.isString()) {
                const std::string tokenStr = token.getString();
                const std::string tokenDescStr = tokenDesc.getString();

                if (!tokenStr.empty() && !tokenDescStr.empty()) {
                    isTokenListAllNull = false;
                    serviceData(tokenTags[i]) = tokenStr;
                    serviceData(tokenDescTags[i]) = tokenDescStr;
                    // printf("TokenTag : %s === TotenDesc : %s\n", tokenStr.c_str(), tokenDescStr.c_str());
                }
            }
        }
    }

    if (isTokenListAllNull) {
        const char* tokenTag = "token";
        const iisys::JSObject& token = responseData(tokenTag);
        if (token.isString()) {
            serviceData(tokenTag) = token;
        }
    }

}

VasResult ElectricityViewModel::processPaymentResponse(const iisys::JSObject& json)
{
    const char* keys[] = {"account_type", "type", "tran_id", "client_id", "sgc", "msno", "krn", "ti", "tt", "unit"
		, "sgcti", "meterNo", "tariffCode", "rate", "units", "region", "unit_value", "unit_cost", "vat", "agent"
		, "arrears", "receipt_no", "invoiceNumber", "tariff", "lastTxDate", "collector", "csp", NULL };

    VasResult response = vasResponseCheck(json);

    const iisys::JSObject& responseData = json("data");
    if (!responseData.isObject()) {
        return response;
    }

    getVasTransactionDateTime(paymentResponse.date, responseData);
    getVasTransactionReference(paymentResponse.reference, responseData);
    getVasTransactionSequence(paymentResponse.transactionSeq, responseData);

    iisys::JSObject data;
    iisys::JSObject temp;
    const iisys::JSObject& addr = responseData("address");
    if (addr.isString() && !addr.getString().empty() && lookupResponse.address.empty()) {
        lookupResponse.address = addr.getString();
    }

    for (size_t i = 0; keys[i] != NULL; ++i) {
        temp = responseData(keys[i]);
        if (!temp.isNull() && !temp.getString().empty()) {
            data(keys[i]) = temp;
        }
    }

    this->addToken(data, responseData);

    if (!data.isNull()) {
        paymentResponse.serviceData = data.getString();
    }

    if (payMethod == PAY_WITH_CARD && responseData("reversal").isBool() && responseData("reversal").getBool() == true) {
        comProxy.reverse(cardData);
    }

    return response;
}

const char* ElectricityViewModel::productString(EnergyType type)
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

const char* ElectricityViewModel::paymentPath()
{
    if (getPaymentMethod() == PAY_WITH_NQR) {
        return nqrStatusCheckUrl();
    } else if (service == IKEJA) {
        return "/api/v2/vas/electricity/payment";
    } else {
        return "/api/v1/vas/electricity/payment";
    }
}

const char* ElectricityViewModel::lookupPath()
{
    if (service == IKEJA) {
        return "/api/v2/vas/electricity/validation";
    } else {
        return "/api/v1/vas/electricity/validation";
    }
}

const char* ElectricityViewModel::apiServiceString(Service service) const
{
    switch (service) {
   
    case IKEJA:
        return "ikedc";
    case EKEDC :
        return "ekedc";
    case IBEDC :
        return "ibedc";
    case PHED :
        return "phedc";
    case AEDC :
        return "aedc";
    case KEDCO :
        return "kedco";
    case EEDC :
        return "eedc";
    default:
        return "";
    }
}

const char* ElectricityViewModel::getEnergyTypeString(EnergyType type) const
{
    switch (type) {
	case PREPAID_TOKEN:
		return "prepaid";
	case PREPAID_SMARTCARD:
		return "smartcard";
	case POSTPAID:
		return "postpaid";
    default:
	    return "";
    }
}


int ElectricityViewModel::readSmartCard(La_Card_info * userCardInfo)
{
    unsigned char inData[512] = { 0 };
    unsigned char outData[512] = { 0 };
    int ret = -1;

    memset(&smartCardInFunc, 0x00, sizeof(SmartCardInFunc));
    bindSmartCardApi(&smartCardInFunc);

    smartCardInFunc.removeCustomerCardCb("SMART CARD", "REMOVE CARD!");

    if (smartCardInFunc.detectSmartCardCb("READ CARD", "INSERT CUSTOMER CARD", 3 * 60 * 1000)) 
    {
        return -1;
    }

    memset(userCardInfo, 0x00, sizeof(La_Card_info));    

    ret = readUserCard(userCardInfo, outData, inData, &smartCardInFunc);
    if (ret == 0) {
        purchaseTimes = userCardInfo->CM_Purchase_Times;
    }

    return ret;
    
}

VasResult ElectricityViewModel::updateSmartCard(La_Card_info * userCardInfo)
{
    VasResult result;
    unsigned char psamBalance[16] = { 0 };

    memset(&smartCardInFunc, 0x00, sizeof(SmartCardInFunc));
    bindSmartCardApi(&smartCardInFunc);

    
    smartCardInFunc.removeCustomerCardCb("SMART CARD", "REMOVE CARD!");

    if (smartCardInFunc.detectSmartCardCb("UPDATE CARD", "INSERT CUSTOMER CARD", 3 * 60 * 1000)) 
    {
        result.error = SMART_CARD_DETECT_ERROR;
        result.message = "Error detecting Card";
        return result;
    }

    userCardInfo->CM_Purchase_Times += 1;
    int ret = updateUserCard(psamBalance, userCardInfo, &smartCardInFunc);

    if (ret) {
        result.error = SMART_CARD_UPDATE_ERROR;
        result.message = unistarErrorToString(ret);
        return result;
    }


    //TODO: persist psamBalance

    return VasResult(NO_ERRORS);
}

SmartCardInFunc& ElectricityViewModel::getSmartCardInFunc()
{
    return smartCardInFunc;
}

La_Card_info& ElectricityViewModel::getUserCardInfo()
{
    return userCardInfo;
}

const unsigned char* ElectricityViewModel::getUserCardNo() const
{
    return this->userCardInfo.CM_UserNo;
}

const std::string& ElectricityViewModel::getRetrievalReference() const
{
    return retrievalReference;
}

VasResult ElectricityViewModel::revalidateSmartCard(const iisys::JSObject& data)
{
    
    VasResult result;

    iisys::JSObject response = data("response");

    const iisys::JSObject& purchaseTimesObj = data("purchaseTimes");
    const iisys::JSObject& vasCustomerAccountObj = data("VASCustomerAccount");
    if (!purchaseTimesObj.isNumber() && !purchaseTimesObj.isString() || !vasCustomerAccountObj.isString()) {
        result = KEY_NOT_FOUND;
        return result;
    }

    const int purchaseTimes = purchaseTimesObj.getNumber();
    const std::string vasCustomerAccount = vasCustomerAccountObj.getString();
    
    unsigned char inData[512] = "0004 ";
    unsigned char outData[512] = {'\0'};

    La_Card_info userCardInfo;
    SmartCardInFunc smartCardInFunc;

    memset(&smartCardInFunc, 0x00, sizeof(SmartCardInFunc));
    memset(&userCardInfo, 0x00, sizeof(La_Card_info));

    bindSmartCardApi(&smartCardInFunc);

    smartCardInFunc.removeCustomerCardCb("SMART CARD", "REMOVE CARD!");

    if (smartCardInFunc.detectPsamCB()) {
        result.message = "NO PSMA!";
        return result;
    }

    if (smartCardInFunc.detectSmartCardCb("UPDATE CARD", "INSERT CUSTOMER CARD", 3 * 60 * 1000)) {
        result.message = "Detect Card Error";
        return result;
    }

    if (readUserCard(&userCardInfo, outData, inData, &smartCardInFunc) != 0) {
        return result;
    }

    if (userCardInfo.CM_Purchase_Times == purchaseTimes && strcmp((char*)userCardInfo.CM_UserNo, vasCustomerAccount.c_str()) == 0) {
        unsigned char psamBalance[16] = { 0 };

        smartCardInFunc.removeCustomerCardCb("SMART CARD", "REMOVE CARD!");

        if (smartCardInFunc.detectSmartCardCb("UPDATE CARD", "INSERT CUSTOMER CARD", 3 * 60 * 1000)) {
            result.message = "Detect Card Error";
            return result;
        }

        userCardInfo.CM_Purchase_Power = response("unit_value").getNumber();
        userCardInfo.CM_Purchase_Times += 1;

        int ret = updateUserCard(psamBalance, &userCardInfo, &smartCardInFunc);
        if (ret) {
            result.message = unistarErrorToString(ret);
            return result;
        }

        result.message = "Write Updated";
        result.error = NO_ERRORS;
    } else {
        result.message = "Write Unauthorized";
    }
    
    return result;
}
