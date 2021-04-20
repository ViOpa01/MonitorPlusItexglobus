#include "nqr.h"

#include "vas.h"

#include "platform/simpio.h"

extern "C" {
#include "string.h"
#include "stdio.h"
#include "../sdk_showqr.h"
#include "libapi_xpos/inc/libapi_print.h"
}


VasResult parseVasQr(std::string& productCode, const std::string& response)
{
    VasResult result(VAS_ERROR);
    iisys::JSObject json;

    if (!json.load(response)) {
        return result;
    }

    result = vasResponseCheck(json);

    if (result.error != NO_ERRORS) {
        return result;
    }

    const iisys::JSObject& responseData = json("data");
    if (!responseData.isObject()) {
        result.error = VAS_ERROR;
        result.message = "Data not found";
        return result;
    }

    const iisys::JSObject& qrCodeObj = responseData("qrCode");
    const iisys::JSObject& productCodeObj  = responseData("productCode");

    if (!productCodeObj.isString() || !qrCodeObj.isString()) {
        result.error = VAS_ERROR;
        result.message = "Code(s) not found";
        return result;
    }

    productCode = productCodeObj.getString();

    result.error = NO_ERRORS;
    result.message = qrCodeObj.getString();

    return result;
}

VasError presentVasQr(const std::string& qrCode, const std::string& rrn)
{
    char* qrString = {0};

    qrString = new char[qrCode.length()];

    for (int index = 0; index < qrCode.length(); index++){
        qrString[index] = qrCode[index];
    }

    qrString[qrCode.length()] = '\0';

    if (qrCode.empty() || rrn.empty()) {
        return VAS_ERROR;
    }

    printf("QR : %s", qrString);

    while (1) {
        int completed = 0;
        char displayMessage[128] = "Scan to Pay";

        // TODO: display or print QR and rrn depending on device
        showQrTest(qrString);
        UPrint_MatrixCode(qrString, strlen(qrString), 1, 1);

        if (completed) {
            return NO_ERRORS;
        }

        // int i = UI_Show2ButtonMessage(10000, "Confirmation", UI_DIALOG_TYPE_QUESTION, "Are you sure you want to cancel this transaction?", "No", "Yes");
        int i = UI_ShowOkCancelMessage(10000, "Confirmation", "Are you sure you want to cancel this transaction?", UI_DIALOG_TYPE_CAUTION);
        if (i != 0) {
            // Maybe printout QR before cancelling 
            // UPrint_MatrixCode(qrString, strlen(qrString), 1, 1);
            return USER_CANCELLATION;
        }
    }
}

iisys::JSObject createNqrJSON(const unsigned long amount, const char* service, const iisys::JSObject& json)
{
    iisys::JSObject nqrJson;

    nqrJson("codeType")    = "dynamic";
    nqrJson("channel")     = vasChannel();
    nqrJson("version")     = vasApplicationVersion();
    nqrJson("vasService")  = service;
    nqrJson("amount")      = majorDenomination(amount);
    nqrJson("paymentData") = json;

    return nqrJson;
}

const char* nqrGenerateUrl()
{
    return "/api/v1/vas/nqr/code/generate";
}

const char* nqrStatusCheckUrl()
{
    return "/api/v1/vas/nqr/transaction/status";
}

iisys::JSObject getNqrPaymentStatusJson(const std::string& productCode, const std::string& clientReference)
{
    iisys::JSObject json;

    json("service") = "NQR";
    json("productCode") = productCode;
    json("clientReference") = clientReference;

    return json;
}
