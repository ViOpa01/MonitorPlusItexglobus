#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <ctype.h>

#include "platform/platform.h"

#include "vasdb.h"

#include "data.h"


Data::Data(const char* title, VasComProxy& proxy)
    : viewModel(title, proxy)
{
}

Data::~Data()
{
}

VasResult Data::beginVas()
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

VasResult Data::lookup()
{
    std::vector<std::string> menuData;
    VasResult result;

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    result = viewModel.lookup();
    if (result.error != NO_ERRORS) {
        return result;
    }

    printf("Look up message : %s\n", result.message.c_str());

    const iisys::JSObject& data = viewModel.getPackages();
    
    if (!data.isArray()) {
        result = VasResult(VAS_ERROR, "Data Packages not found");
        return result;
    }

    printf("Look up message 2\n");
    
    const size_t size = data.size();

    if (size == 0) {
        result = VasResult(VAS_ERROR, "Data Packages not found");
        return result;
    }

    for (size_t i = 0; i < size; ++i) {
        menuData.push_back(data[i]("description").getString() + vasimpl::menuendl() + data[i]("amount").getString() + " Naira");
    }

    printf("Look up message 23\n");

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

VasResult Data::initiate()
{

    while (1) {

        PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
        if (payMethod == PAYMENT_METHOD_UNKNOWN) {
            return VasResult(USER_CANCELLATION);
        } else if (viewModel.setPaymentMethod(payMethod).error != NO_ERRORS) {
            return VasResult(VAS_ERROR);
        }

        std::string phoneNumber = getPhoneNumber("Phone Number", "");
        if (phoneNumber.empty()) {
            continue;
        } else if (viewModel.setPhoneNumber(phoneNumber).error != NO_ERRORS) {
            return VasResult(VAS_ERROR);
        }
        break;
    }

    return VasResult(NO_ERRORS);
}

VasResult Data::complete()
{
    std::string pin;
    VasResult response;

    response.error = getVasPin(pin);
    if (response.error != NO_ERRORS) {
        return response;
    }

    Demo_SplashScreen("Payment In Progress", "www.payvice.com");

    response = viewModel.complete(pin);

    return response;
}

Service Data::vasServiceType()
{
    return viewModel.getService();
}

std::map<std::string, std::string> Data::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);
    

    return record;
}