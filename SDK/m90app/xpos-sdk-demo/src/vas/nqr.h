#ifndef NQR_H
#define NQR_H

#include "vas.h"
#include "platform/platform.h"

VasResult parseVasQr(std::string& productCode, const std::string& response);

VasError presentVasQr(const std::string& qrCode, const std::string& rrn);

iisys::JSObject createNqrJSON(const unsigned long amount, const char* service, const iisys::JSObject& json);

const char* nqrGenerateUrl();
const char* nqrStatusCheckUrl();

iisys::JSObject getNqrPaymentStatusJson(const std::string& productCode, const std::string& clientReference);

#endif
