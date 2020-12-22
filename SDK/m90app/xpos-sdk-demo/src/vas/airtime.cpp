#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <ctype.h>

#include "platform/platform.h"

#include "jsonwrapper/jsobject.h"
#include "vasdb.h"

#include "airtime.h"

Airtime::Airtime(const char* title, VasComProxy& proxy)
    : _title(title)
    , viewModel(title, proxy)
{
}

Airtime::~Airtime()
{
}

VasResult Airtime::beginVas()
{
    Network networks[] = { AIRTEL, ETISALAT, GLO, MTN };
    std::vector<std::string> menu;

    for (size_t i = 0; i < sizeof(networks) / sizeof(Network); ++i) {
        menu.push_back(networkString(networks[i]));
    }

    while (1) {
        int selection = UI_ShowSelection(30000, "Networks", menu, 0);
        if (selection < 0) {
            return VasResult(USER_CANCELLATION);
        }

        const Service service = getAirtimeService(networks[selection]);
        if (service == SERVICE_UNKNOWN) {
            continue;
        } else if (viewModel.setService(service).error != NO_ERRORS) {
            return VasResult(VAS_ERROR);
        }

        break;
    }

    return VasResult(NO_ERRORS);
}

VasResult Airtime::lookup()
{
    return VasResult(NO_ERRORS);   
}

Service Airtime::getAirtimeService(Network network) const
{
    switch (network) {
    case ETISALAT:
        return ETISALATVTU;
    case AIRTEL: {
        Service services[] = { AIRTELVTU, AIRTELVOT };
        std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

        return selectService(networkString(network).c_str(), serviceVector);
    }
    case GLO: {
        Service services[] = { GLOVTU, GLOVOS, GLOVOT };
        std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

        return selectService(networkString(network).c_str(), serviceVector);
    }
    case MTN:
        return MTNVTU;
    }

    return SERVICE_UNKNOWN;
}

VasResult Airtime::initiate()
{

    while (1) {
        VasResult result;
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

        const PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
        if (payMethod == PAYMENT_METHOD_UNKNOWN) {
            return VasResult(USER_CANCELLATION);
        }

        result = viewModel.setPaymentMethod(payMethod);
        if (result.error != NO_ERRORS) {
            return result;
        } 

        const std::string phoneNumber = getPhoneNumber("Phone Number", "");
        if (phoneNumber.empty()) {
            return VasResult(USER_CANCELLATION);
        }

        result = viewModel.setPhoneNumber(phoneNumber);
        if (result.error != NO_ERRORS) {
            return result;
        }

        break;
    }

    return VasResult(NO_ERRORS);
}

Service Airtime::vasServiceType()
{
    return viewModel.getService();
}

VasResult Airtime::complete()
{
    std::string pin;
    VasResult response;


    response.error = getVasPin(pin);
    if (response.error != NO_ERRORS) {
        return response;
    }

    Demo_SplashScreen("Please Wait", "www.payvice.com");

    response = viewModel.complete(pin);

    return response;
}

std::map<std::string, std::string> Airtime::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}

std::string Airtime::networkString(Network network) const
{
    switch (network) {
    case AIRTEL:
        return std::string("Airtel");
    case ETISALAT:
        return std::string("Etisalat");
    case GLO:
        return std::string("Glo");
    case MTN:
        return std::string("MTN");
    default:
        return std::string("");
    }
}
