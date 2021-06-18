#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <ctype.h>

#include "platform/platform.h"

#include "vasdb.h"
#include "vasussd.h"

#include "dataussd.h"


DataUssd::DataUssd(const char* title, VasComProxy& proxy)
    : viewModel(title, proxy)
{
}

DataUssd::~DataUssd()
{
}

VasResult DataUssd::beginVas()
{
    Service services[] = { AIRTELDATA, ETISALATDATA, GLODATA, MTNDATA };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    const Service service = selectService("Networks", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        return VasResult(USER_CANCELLATION);
    } else if (viewModel.setService(service).error != NO_ERRORS) {
        return VasResult(VAS_ERROR);
    }

    return VasResult(NO_ERRORS);
}

VasResult DataUssd::packageLookup()
{
    std::vector<std::string> menuData;
    VasResult result;

    Demo_SplashScreen("Loading Bundles...", "www.payvice.com");

    result = viewModel.packageLookup();
    if (result.error != NO_ERRORS) {
        return result;
    }

    const iisys::JSObject& data = viewModel.getPackages();
    
    if (!data.isArray()) {
        result = VasResult(VAS_ERROR, "Data Packages not found");
        return result;
    }
    
    const size_t size = data.size();

    if (size == 0) {
        result = VasResult(VAS_ERROR, "Data Packages not found");
        return result;
    }

    for (size_t i = 0; i < size; ++i) {
        menuData.push_back(data[i]("description").getString() + vasimpl::menuendl() + data[i]("amount").getString() + " Naira");
    }

    int selection = UI_ShowSelection(60000, "Data Packages", menuData, 0);

    if (selection < 0) {
        result = VasResult(USER_CANCELLATION);
        return result;
    }

    result = viewModel.setSelectedPackageIndex(selection);
    if (result.error != NO_ERRORS) {
        return result;
    }
   
    return VasResult(NO_ERRORS);
}

VasResult DataUssd::lookup()
{
    VasResult result;

    result = packageLookup();
    if (result.error != NO_ERRORS) {
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

    PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
    if (payMethod == PAYMENT_METHOD_UNKNOWN) {
        result.error = USER_CANCELLATION;
        return result;
    } else if (viewModel.setPaymentMethod(payMethod).error != NO_ERRORS) {
        result.error = VAS_ERROR;
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

VasResult DataUssd::initiate()
{
    return VasResult(NO_ERRORS);
}

VasResult DataUssd::complete()
{
    std::string pin;
    VasResult response;

    if (viewModel.getPaymentMethod() == PAY_WITH_CASH) {
        response.error = getVasPin(pin);
        if (response.error != NO_ERRORS) {
            return response;
        }
    }

    Demo_SplashScreen("Payment In Progress", "www.payvice.com");

    response = viewModel.complete(pin);

    return response;
}

Service DataUssd::vasServiceType()
{
    return viewModel.getService();
}

std::map<std::string, std::string> DataUssd::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);
    

    return record;
}