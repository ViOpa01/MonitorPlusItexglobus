#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctype.h>

#include "platform/platform.h"

#include "vasdb.h"

#include "electricityussd.h"
#include "vasussd.h"

ElectricityUssd::ElectricityUssd(const char* title, VasComProxy& proxy)
    : viewModel(title, proxy)
{
}

ElectricityUssd::~ElectricityUssd()
{
}

VasResult ElectricityUssd::beginVas()
{
    VasResult result;
    Service services[] = { IKEJA, EKEDC, EEDC, IBEDC, PHED, AEDC, KEDCO };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    const Service service = selectService("Distri. Companies", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        result = VasResult(USER_CANCELLATION); 
        return result;
    } else if (viewModel.setService(service).error != NO_ERRORS) {
        result = VasResult(VAS_ERROR);
        return result;
    }

    unsigned long amount = getVasAmount(serviceToString(viewModel.getService()));
    if (!amount) {
        result = VasResult(USER_CANCELLATION);
        return result;
    }
    result = viewModel.setAmount(amount);
    if (result.error != NO_ERRORS) {
        UI_ShowButtonMessage(30000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        return result;
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


    result = VasResult(NO_ERRORS);

    return result;
}

VasResult ElectricityUssd::lookup()
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

VasResult ElectricityUssd::initiate()
{
    return VasResult(NO_ERRORS);
}

VasResult ElectricityUssd::complete()
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

Service ElectricityUssd::vasServiceType()
{
    return viewModel.getService();
}

std::map<std::string, std::string> ElectricityUssd::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}
