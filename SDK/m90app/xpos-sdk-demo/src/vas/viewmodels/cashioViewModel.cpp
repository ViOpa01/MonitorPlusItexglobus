#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <ctype.h>
#include <math.h>

#include "platform/platform.h"

#include "cashio.h"
#include "payvice.h"

std::vector<ViceBankingViewModel::Bank> ViceBankingViewModel::banks;

ViceBankingViewModel::ViceBankingViewModel(const char* title, VasComProxy& proxy)
    : _title(title)
    , service(SERVICE_UNKNOWN)
    , comProxy(proxy)
{
}

VasResult ViceBankingViewModel::lookupCheck(const VasResult& lookupStatus)
{
    iisys::JSObject json;

    if (lookupStatus.error != NO_ERRORS) {
        return VasResult(LOOKUP_ERROR, lookupStatus.message.c_str());
    }

    if (!json.load(lookupStatus.message)) {
        return VasResult(INVALID_JSON, "Invalid Response");
    }

    VasResult errStatus = vasResponseCheck(json);
    if (errStatus.error != NO_ERRORS) {
        return VasResult(errStatus.error, errStatus.message.c_str());
    }

    const iisys::JSObject& data = json("data");
    if (!data.isObject()) {
        return VasResult(errStatus.error, errStatus.message.c_str());
    }

    return extractLookupInfo(data);
}

VasResult ViceBankingViewModel::lookup()
{
    VasResult response;
    iisys::JSObject obj;

    obj("channel") = vasChannel();
    obj("amount") = majorDenomination(amount);
    obj("service") = apiServiceString(getService());

    if (getService() == TRANSFER) {
        obj("accountNo") = beneficiary;
        obj("bankCode") = ViceBankingViewModel::banks[beneficiaryBankIndex].code;
        response = comProxy.lookup("/api/v1/vas/vicebanking/transfer/validation", &obj);
    } else if (getService() == WITHDRAWAL) {
        if (payMethod == PAY_WITH_CARD) {
            response = comProxy.lookup("/api/v1/vas/vicebanking/withdrawal/validation", &obj);
        } else if (payMethod == PAY_WITH_MCASH) {
            response = comProxy.lookup("/api/v1/vas/vicebanking/mcash/withdrawal/validation", &obj);
        } else if (payMethod == PAY_WITH_CGATE) {
            response = comProxy.lookup("/api/v1/vas/vicebanking/cgate/withdrawal/validation", &obj);
        } else {
            return VasResult(VAS_ERROR);
        }
    }

    return lookupCheck(response);
}

VasResult ViceBankingViewModel::initiate(const std::string& pin)
{
    VasResult response;
    
    if (!isPaymentUSSD()) {
        response = NO_ERRORS;
        return response;
    }

    // Initiate ussd transaction
    iisys::JSObject json;

    if (getPaymentJson(json) < 0) {
        response.error = VAS_ERROR;
        response.message = "Data Error";
        return response;
    }

    
    json("clientReference") = clientReference = getClientReference();
    json("pin") = encryptedPin(Payvice(), pin.c_str());

    if (payMethod == PAY_WITH_MCASH) {
        response = comProxy.initiate("/api/v1/vas/vicebanking/mcash/withdrawal/payment", &json);
    } else if (payMethod == PAY_WITH_CGATE) {
        response = comProxy.initiate("/api/v1/vas/vicebanking/cgate/withdrawal/payment", &json);
    }

    if (response.error != NO_ERRORS) {
        return response;
    }

    if (!json.load(response.message)) {
        response.error = INVALID_JSON;
        response.message = "Invalid Response";
        return response;
    }

    response = vasResponseCheck(json);
    if (response.error != NO_ERRORS) {
        return response;
    }

    const iisys::JSObject& data = json("data");

    if (response.error == NO_ERRORS) {
        const iisys::JSObject& productCode = data("productCode");
        const iisys::JSObject& cgateRef = data("referenceCode");

        if (productCode.isNull()) {
            response.error = VAS_ERROR;
            return response;
        } else if (payMethod == PAY_WITH_CGATE && !cgateRef.isNull()) {
            response.message = std::string("Dial *BankUSSDCode*000*") + cgateRef.getString() + "#\n Then Press Enter to Continue";
        } else {
            response.message = "Complete Transaction and Press Enter to Continue";
        }

        lookupResponse.productCode = productCode.getString();
    }

    return response;
}

Service ViceBankingViewModel::getService() const
{
    return service;
}

unsigned long ViceBankingViewModel::getAmount() const
{
    return amount;
}

const std::string& ViceBankingViewModel::getName() const
{
    return lookupResponse.name;
}

const std::string& ViceBankingViewModel::getBeneficiaryAccount() const
{
    return lookupResponse.account;
}

unsigned long ViceBankingViewModel::getAmountSettled() const
{
    return lookupResponse.amountSettled;
}

unsigned long ViceBankingViewModel::getAmountToDebit() const
{
    return lookupResponse.amountToDebit;
}

PaymentMethod ViceBankingViewModel::getPaymentMethod() const
{
    return payMethod;
}

bool ViceBankingViewModel::isPaymentUSSD() const
{
    if (payMethod == PAY_WITH_CGATE || payMethod == PAY_WITH_MCASH) {
        return true;
    }
    return false;
}

VasResult ViceBankingViewModel::setService(Service service)
{
    VasResult result;
    if (service != TRANSFER && service != WITHDRAWAL) {
        return result;
    }

    if (service == TRANSFER) {
        payMethod = PAY_WITH_CASH;
    }

    this->service = service;
    result.error = NO_ERRORS;
    return result;
}

VasResult ViceBankingViewModel::setAmount(unsigned long amount)
{
    this->amount = amount;
    return VasResult(NO_ERRORS);
}

VasResult ViceBankingViewModel::setPaymentMethod(PaymentMethod payMethod)
{
    VasResult result;

    if (getService() == TRANSFER && payMethod != PAY_WITH_CASH) {
        return result;
    } else if (payMethod == PAYMENT_METHOD_UNKNOWN) {
        return result;
    } else if (payMethod != PAY_WITH_CARD && payMethod != PAY_WITH_CGATE && payMethod != PAY_WITH_MCASH) {
        return result;
    }
    
    result.error = NO_ERRORS;
    this->payMethod = payMethod;

    return result;
}

VasResult ViceBankingViewModel::setPhoneNumber(const std::string& phoneNumber)
{
    VasResult result;

    if (!phoneNumber.empty()) {
        result.error = NO_ERRORS;
        this->phoneNumber = phoneNumber;
    }

    return result;
}

VasResult ViceBankingViewModel::setBeneficiary(const std::string& account, const size_t bankIndex)
{
    VasResult result;

    if (!account.empty() && bankIndex < ViceBankingViewModel::banks.size()) {
        beneficiary = account;
        beneficiaryBankIndex = bankIndex;
        result.error = NO_ERRORS;
    }

    return result;
}

VasResult ViceBankingViewModel::setNarration(const std::string& narration)
{
    VasResult result(NO_ERRORS);
    this->narration = narration;
    return result;
}

VasResult ViceBankingViewModel::complete(const std::string& pin)
{
    VasResult response;
    iisys::JSObject json;
    std::string rrn;

    if (isPaymentUSSD()) {
        json("service") = apiServiceString(getService());
        json("productCode") = lookupResponse.productCode;
        json("clientReference") = clientReference;
    } else {
        if (getPaymentJson(json) < 0) {
            response.error = VAS_ERROR;
            response.message = "Data Error";
            return response;
        }
        json("clientReference") = getClientReference(rrn);
        json("pin") = encryptedPin(Payvice(), pin.c_str());
    }


    if (payMethod == PAY_WITH_CARD) {
        cardData = CardData(lookupResponse.amountToDebit, rrn);
        cardData.refcode = getRefCode(serviceToProductString(service));

        if (cardData.amount > lookupResponse.thresholdAmount) {
            const std::string serviceCode = "87001001";     // Test, instead of 87001214, send 87001001 
            const std::string accountNo = "1022665017";     // instead of 0001451023, send 1022643026 (all in F60)
            cardData.upWithdrawal.field60 =
            "upsl-direct~010085C24300148041Meter Number=12." + serviceCode + ".Acct=" + accountNo + ".Phone=" + phoneNumber + "~upsl-withdrawal";
        }

        response = comProxy.complete(paymentPath(service), &json, &cardData);
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

    response = processPaymentResponse(json);

    return response;
}

std::map<std::string, std::string> ViceBankingViewModel::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record;
    const Service service = getService();
    std::string serviceStr(serviceToString(service));
    const std::string amountStr = vasimpl::to_string(this->amount);

    record[VASDB_PRODUCT] = serviceStr; 
    record[VASDB_CATEGORY] = _title;
    record[VASDB_SERVICE] = serviceStr;
    record[VASDB_AMOUNT] = amountStr;

    record[VASDB_BENEFICIARY] = beneficiary;
    record[VASDB_BENEFICIARY_NAME] = lookupResponse.name;
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
    if (service == TRANSFER) {
        record[VASDB_SERVICE_DATA] = std::string("{\"recBank\": \"") + ViceBankingViewModel::banks[beneficiaryBankIndex].name + "\"}";
    }

    return record;
}

const char* ViceBankingViewModel::paymentPath(Service service)
{
    if (service == TRANSFER) {
        return "/api/v1/vas/vicebanking/transfer/payment";
    } else if (service == WITHDRAWAL) {
        if (payMethod == PAY_WITH_CARD) {
            return "/api/v1/vas/vicebanking/withdrawal/payment";
        } else if (payMethod == PAY_WITH_MCASH) {
            return "/api/v1/vas/vicebanking/mcash/withdrawal/complete-payment";
        } else if (payMethod == PAY_WITH_CGATE) {
            return "/api/v1/vas/vicebanking/cgate/withdrawal/complete-payment";
        }
    }

    return "";
}

VasResult ViceBankingViewModel::processPaymentResponse(const iisys::JSObject& json)
{
    VasResult response = vasResponseCheck(json);

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

VasResult ViceBankingViewModel::extractLookupInfo(const iisys::JSObject& data)
{
    const iisys::JSObject& name                 = data("name");
    const iisys::JSObject& amountSettled        = data("amountSettled");
    const iisys::JSObject& amountToDebit        = data("amountToDebit");
    const iisys::JSObject& productCode          = data("productCode");
    const iisys::JSObject& withdrawalThreshold  = data("withdrawalThreshold");
    

    if (!name.isNull()) {
        lookupResponse.name = name.getString();
    }

    if (!productCode.isNull()) {
        lookupResponse.productCode = productCode.getString();
    }

    if (service == TRANSFER) {
        if (lookupResponse.name.empty()) {
            return VasResult(KEY_NOT_FOUND, "Name not found");
        }

        iisys::JSObject element = data("account");
        if (!element.isNull()) {
            lookupResponse.account = element.getString();
        }
    } else if (service == WITHDRAWAL) {
        lookupResponse.amountSettled = (unsigned long) lround(amountSettled.getNumber() * 100.0);
        lookupResponse.amountToDebit = (unsigned long) lround(amountToDebit.getNumber() * 100.0);
        if (withdrawalThreshold.isNumber() || withdrawalThreshold.isString()) {
            lookupResponse.thresholdAmount = (unsigned long) lround(withdrawalThreshold.getNumber() * 100);
        } else {
            lookupResponse.thresholdAmount = 0;
        }
    }

    return VasResult(NO_ERRORS);
}

int ViceBankingViewModel::getPaymentJson(iisys::JSObject& json) const
{
    json("productCode") = lookupResponse.productCode;
    json("paymentMethod") = apiPaymentMethodString(payMethod);
    json("customerPhoneNumber") = phoneNumber;
    json("channel") = vasChannel();
    json("service") = apiServiceString(service);

    if (service == TRANSFER) {
        json("narration") = narration;
    }

    return 0;
}

const char* ViceBankingViewModel::apiServiceString(Service service) const
{
    if (service == TRANSFER) {
        return "transfer";
    } else if (service == WITHDRAWAL) {
        switch (payMethod) {
        case PAY_WITH_CARD:
            return "withdrawal";
        case PAY_WITH_CGATE:
            return "cgatewithdrawal";
        case PAY_WITH_MCASH:
            return "mcashwithdrawal";
        default:
            return "";
        }
    }

    return "";
}

VasError ViceBankingViewModel::fetchBankList(std::vector<ViceBankingViewModel::Bank>& bankVec) const
{
    iisys::JSObject json;

    const VasResult response = comProxy.sendVasRequest("/api/v1/vas/vicebanking/bankcodes");

    if (!json.load(response.message)) {
        return INVALID_JSON;
    }

    const VasResult errStatus = vasResponseCheck(json);
    if (errStatus.error != NO_ERRORS) {
        return errStatus.error;
    }

    const iisys::JSObject& data = json("data");
    if (!data.isObject()) {
        return KEY_NOT_FOUND;
    }

    const iisys::JSObject& banks = data("bankCodes");
    if (!banks.isArray()) {
        return KEY_NOT_FOUND;
    }

    for (size_t i = 0; i < banks.size(); ++i) {
        const iisys::JSObject& bankCode = banks[i]("bankCode");
        const iisys::JSObject& bankName = banks[i]("bankName");

        if (!bankCode.isString() || !bankName.isString()) {
            return TYPE_MISMATCH;
        }

        bankVec.push_back(Bank(bankName.getString(), bankCode.getString()));
    }

    return NO_ERRORS;
}

const std::vector<ViceBankingViewModel::Bank>& ViceBankingViewModel::banklist() const
{
    if (ViceBankingViewModel::banks.empty() && fetchBankList(ViceBankingViewModel::banks) != NO_ERRORS) {
        ViceBankingViewModel::banks.clear();
    }

    return ViceBankingViewModel::banks;
}
