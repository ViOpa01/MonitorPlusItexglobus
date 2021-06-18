#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "platform/platform.h"

#include "jsonwrapper/jsobject.h"
#include "vasdb.h"
#include "vasussd.h"

#include "airtimeussd.h"

AirtimeUssd::AirtimeUssd(const char* title, VasComProxy& proxy)
    : _title(title)
    , viewModel(title, proxy)
{
}

AirtimeUssd::~AirtimeUssd()
{
}

VasResult AirtimeUssd::beginVas()
{
    VasResult result;

    while (1) {
        unsigned long amount = getVasAmount(serviceToString(viewModel.getService()));
        if (!amount) {
            result = VasResult(USER_CANCELLATION);
            return result;
        }

        result = viewModel.setAmount(amount);
        if (result.error != NO_ERRORS) {
            UI_ShowButtonMessage(30000, "Please Try Again", result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
            continue;
        }

        const int units = vasussd::getUnits();
        if (units == 0) {
            result.error = USER_CANCELLATION;
            return result;
        }

        result = viewModel.setUnits(units);
        if (result.error != NO_ERRORS) {
            return result;
        }

        result.error = NO_ERRORS;
        break;
    }

    return result;
}

VasResult AirtimeUssd::lookup()
{
    VasResult result;

    const PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
    if (payMethod == PAYMENT_METHOD_UNKNOWN) {
        result = VasResult(USER_CANCELLATION);
        return result;
    }

    result = viewModel.setPaymentMethod(payMethod);
    if (result.error != NO_ERRORS) {
        return result;
    }

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
    result = viewModel.lookup();
    if (result.error != NO_ERRORS) {
        return result;
    }

    result = vasussd::displayLookup(viewModel.getAmount(), viewModel.getUnits());

    return result;
}

VasResult AirtimeUssd::initiate()
{
    return VasResult(NO_ERRORS);
}

Service AirtimeUssd::vasServiceType()
{
    return viewModel.getService();
}

VasResult AirtimeUssd::complete()
{
    std::string pin;
    VasResult response;

    if (viewModel.getPaymentMethod() == PAY_WITH_CASH) {
        response.error = getVasPin(pin);
        if (response.error != NO_ERRORS) {
            return response;
        }
    }

    Demo_SplashScreen("Please Wait", "www.payvice.com");

    response = viewModel.complete(pin);

    return response;
}

std::map<std::string, std::string> AirtimeUssd::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}
