#ifndef VAS_VASUSSD_H
#define VAS_VASUSSD_H

#include "vas.h"

namespace vasussd {

int getUnits();
VasResult lookupCheck(unsigned long& totalAmount, const VasResult& lookupStatus);
VasResult displayLookup(const unsigned long totalAmount, const int units);
int getPaymentJson(iisys::JSObject& json, const std::string& productCode, const PaymentMethod payMethod);
VasError processUssdPaymentResponse(iisys::JSObject& serviceData, const iisys::JSObject& responseData);
VasResult decryptPins(const unsigned char* key, const std::string& data, int ivlen);

const char* pinGenerationLookupPath();
const char* pinGenerationPaymentPath();

}

#endif
