#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "platform/platform.h"

#include "jsonwrapper/jsobject.h"
#include "nqr.h"
#include "vasdb.h"

#include "waec.h"

Waec::Waec(const char* title, VasComProxy& proxy)
    : _title(title)
    , viewModel(title, proxy)
{
}

Waec::~Waec()
{
}

VasResult Waec::beginVas()
{
    Service services[] = { WAEC_REGISTRATION, WAEC_RESULT_CHECKER };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    const Service service = selectService("Waec", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        return VasResult(USER_CANCELLATION);
    } else if (viewModel.setService(service).error != NO_ERRORS) {
        return VasResult(VAS_ERROR);
    }

    return VasResult(NO_ERRORS);
}

VasResult Waec::lookup()
{
    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
    VasResult result = viewModel.lookup();

    if (result.error != NO_ERRORS) {
        return result;
    }

    std::ostringstream msg;

    msg << std::string(viewModel.getService() == WAEC_REGISTRATION ? "WAEC Registration" : "WAEC Result Checker") << "\n\nAmount: " + majorDenomination(viewModel.getAmount());

    int i = UI_ShowOkCancelMessage(60000, "Please confirm", msg.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        result.error = USER_CANCELLATION;
    }

    return result;
}

VasResult Waec::captureWaecData()
{
    while (1) {
        std::string email;
        std::string firstName;
        std::string lastName;

        if (getText(email, 0, 30000, "Email", "", UI_DIALOG_TYPE_NONE) < 0) {
            return VasResult(USER_CANCELLATION);
        }

        const std::string phoneNumber = getPhoneNumber("Phone Number", "");

        if (phoneNumber.empty()) {
            continue;
        }

        if (viewModel.setEmail(email).error != NO_ERRORS) {
            UI_ShowButtonMessage(10000, "Error", "Error setting mail", "", UI_DIALOG_TYPE_WARNING);
            return VasResult(VAS_ERROR);
        }

        if (viewModel.setPhoneNumber(phoneNumber).error != NO_ERRORS) {
            UI_ShowButtonMessage(10000, "Error", "Error setting Phone number", "", UI_DIALOG_TYPE_WARNING);
            return VasResult(VAS_ERROR);
        }

        if (getText(firstName, 0, 30000, "First Name", "", UI_DIALOG_TYPE_NONE) < 0) {
            return VasResult(USER_CANCELLATION);
        }

        if (viewModel.setFirstName(firstName).error != NO_ERRORS) {
            UI_ShowButtonMessage(10000, "Error", "Error setting first name", "", UI_DIALOG_TYPE_WARNING);
            return VasResult(VAS_ERROR);
        }

        if (getText(lastName, 0, 30000, "Last Name", "", UI_DIALOG_TYPE_NONE) < 0) {
            return VasResult(USER_CANCELLATION);
        }

        if (viewModel.setLastName(lastName).error != NO_ERRORS) {
            UI_ShowButtonMessage(10000, "Error", "Error setting last name", "", UI_DIALOG_TYPE_WARNING);
            return VasResult(VAS_ERROR);
        }

        break;
    }

    return VasResult(NO_ERRORS);
}

VasResult Waec::initiate()
{
    VasResult result;

    result = captureWaecData();
    if (result.error != NO_ERRORS) {
        return result;
    }

    const PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_CASH));
    if (payMethod == PAYMENT_METHOD_UNKNOWN) {
        result = VasResult(USER_CANCELLATION);
        return result;
    }

    result = viewModel.setPaymentMethod(payMethod);
    if (result.error != NO_ERRORS) {
        return result;
    }

    return result;
}

Service Waec::vasServiceType()
{
    return viewModel.getService();
}

VasResult Waec::complete()
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

std::map<std::string, std::string> Waec::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}
