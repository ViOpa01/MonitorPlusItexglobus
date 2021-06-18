#include <math.h>
#include <string.h>

#include <string>

#include "platform/platform.h"

#include "vasussd.h"

int vasussd::getUnits()
{
    while (true) {
        const std::string units = getNumeric(0, 60000, "Number of PINs to Buy", "#", UI_DIALOG_TYPE_NONE);
        if (units.empty()) {
            return 0;
        }
        
        const int num = atoi(units.c_str());
        if (num <= 0) {
            return -1;
        } else if (num > 50) {
            UI_ShowButtonMessage(30000, "", "Maximum of 50 allowed", "Okay", UI_DIALOG_TYPE_NONE);
            continue;
        }
        
        return num;
    }
}

VasResult vasussd::lookupCheck(unsigned long& totalAmount, const VasResult& lookupStatus)
{
    iisys::JSObject json;

    totalAmount = 0;

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
        return VasResult(INVALID_JSON);
    }

    const iisys::JSObject& totalAmountObj = data("totalAmount");
    const iisys::JSObject& productCode = data("productCode");

    if (!productCode.isString() || !totalAmountObj.isNumber()) {
        return VasResult(INVALID_JSON);
    }

    totalAmount = (unsigned long) lround(totalAmountObj.getNumber() * 100.0f);

    return VasResult(NO_ERRORS, productCode.getString().c_str());
}

VasResult vasussd::displayLookup(const unsigned long totalAmount, const int units)
{
    VasResult result;
    std::string message;
    

    message = std::string("Total Amount: ") + majorDenomination(totalAmount) + "\n\nNumber of pins: " + vasimpl::to_string(units) + "\nAmount per pin: " + majorDenomination(totalAmount / units);

    int i = UI_ShowOkCancelMessage(60000, "Please confirm", message.c_str(), UI_DIALOG_TYPE_NONE);

    if (i != 0) {
        result.error = USER_CANCELLATION;
        return result;
    }

    result.error = NO_ERRORS;

    return result;
}

int vasussd::getPaymentJson(iisys::JSObject& json, const std::string& productCode, const PaymentMethod payMethod)
{
    json("channel") = vasChannel();
    json("productCode") = productCode;
    json("version") = vasApplicationVersion();
    json("paymentMethod") = apiPaymentMethodString(payMethod);

    return 0;
}

static VasError decodeAndDecrypt(iisys::JSObject& serviceData, const std::string& b64EncKey, const std::string& b64Encpins)
{
    VasError status = VAS_ERROR;
    const int authlen = 16;

    if (b64EncKey.empty() || b64Encpins.empty()) {
        return status;
    }

    std::string encKey = vasimpl::base64_decode((unsigned char *)b64EncKey.c_str(), b64EncKey.length());
    std::string encPins = vasimpl::base64_decode((unsigned char *)b64Encpins.c_str(), b64Encpins.length());

    std::string apiKey = vasApiKey();
    const VasResult plainKey = vasussd::decryptPins((unsigned char*)apiKey.c_str(), encKey, authlen);
    apiKey.clear();

    if (plainKey.error != NO_ERRORS) {
        return status;
    }

    iisys::JSObject json;
    const VasResult pinResult = vasussd::decryptPins((unsigned char*)plainKey.message.c_str(), encPins, authlen);
    if (pinResult.error == NO_ERRORS && json.load(pinResult.message) && json.isArray()) {
        status = NO_ERRORS;
        serviceData("pins") = json;
    }

    return status;
}

VasError vasussd::processUssdPaymentResponse(iisys::JSObject& serviceData, const iisys::JSObject& responseData)
{
    VasError status = VAS_ERROR;
    
    const iisys::JSObject& pins = responseData("pins");
    const iisys::JSObject& hint = responseData("hint");
    const iisys::JSObject& product = responseData("product");
    const iisys::JSObject& description = responseData("description");
    const iisys::JSObject& service = responseData("service");
    const iisys::JSObject& batchNo = responseData("batchNo");
    const iisys::JSObject& amount = responseData("amount");
    const iisys::JSObject& vasAuthKey = responseData("vasAuthKey");

    if (hint.isString()) {
        serviceData("hint") = hint.getString();
    }

    if (product.isString()) {
        serviceData("product") = product.getString();
    }

    if (description.isString()) {
        serviceData("description") = description.getString();
    }

    if (service.isString()) {
        serviceData("service") = service.getString();
    }

    if (batchNo.isString()) {
        serviceData("batchNo") = batchNo.getString();
    }

    if (amount.isNumber()) {
        serviceData("amount") = vasimpl::to_string(lround(amount.getNumber() * 100.0f));
    }

    if (!pins.isString() && !vasAuthKey.isString()) {
        return status;
    }

    status = decodeAndDecrypt(serviceData, vasAuthKey.getString(), pins.getString());

    return status;
}

static unsigned char* subString(size_t start, size_t stop, const unsigned char* src, unsigned char* dst, size_t size)
{
    int count = stop - start;

    if (count <= 0) {
        return NULL;
    } else if ((size_t)count >= --size) {
        count = size;
    }

    memcpy(dst, src + start, count);
    return dst;
}

VasResult vasussd::decryptPins(const unsigned char* key, const std::string& data, int ivlen)
{
    VasResult result;

    size_t outlen = 0;
    unsigned char iv[17] = { 0 };
    unsigned char cipherText[0X4000] = { 0 };

    unsigned char* decodedData = vasimpl::base64_decode((unsigned char*)data.c_str(), data.length(), &outlen);


    if (decodedData == NULL) {
        result.message = "Decoded Data is NULL";
        return result;
    }

    subString(0, ivlen, decodedData, iv, sizeof(iv));
    subString(ivlen, outlen, decodedData, cipherText, sizeof(cipherText));

    free(decodedData);

    result.message = vasimpl::cbc128_decrypt(cipherText, outlen - ivlen, key, iv);
    if (!result.message.empty()) {
        result.error = NO_ERRORS;
    }


    return result;
}

const char* vasussd::pinGenerationLookupPath()
{
    return "/api/v1/vas/pin/lookup";
}

const char* vasussd::pinGenerationPaymentPath()
{
    return "/api/v1/vas/pin/generate/bulk";
}
