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
#include "nqr.h"

#include "jamb.h"

Jamb::Jamb(const char* title, VasComProxy& proxy)
    : _title(title)
    , viewModel(title, proxy)
{
}

Jamb::~Jamb()
{
}

VasResult Jamb::beginVas()
{
    Service services[] = { JAMB_UTME_PIN, JAMB_DE_PIN };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    const Service service = selectService("JAMB", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        return VasResult(USER_CANCELLATION);
    } else if (viewModel.setService(service).error != NO_ERRORS) {
        return VasResult(VAS_ERROR);
    }

    while (1) {
        std::string email;
        if (getText(email, 0, 30000, "Email", "", UI_DIALOG_TYPE_NONE) < 0) {
            return VasResult(USER_CANCELLATION);
        }

        const std::string confirmationCode = getNumeric(0, 60000, "Confirmation Code", "Enter code", UI_DIALOG_TYPE_NONE);
        if (confirmationCode.empty()) {
            continue;
        }

        std::string message = "\nEmail:\n" + email + "\n" + "Confirmation Code:\n" + confirmationCode; 

        int i = UI_ShowOkCancelMessage(10000, "Please Confirm", message.c_str(), UI_DIALOG_TYPE_NONE);
        if (i != 0) {
            continue;
        }

        if (viewModel.setEmail(email).error != NO_ERRORS) {
            UI_ShowButtonMessage(10000, "Error", "Error setting mail", "", UI_DIALOG_TYPE_WARNING);
            return VasResult(VAS_ERROR);
        } else if (viewModel.setConfirmationCode(confirmationCode).error != NO_ERRORS){
            UI_ShowButtonMessage(10000, "Error", "Error setting confirmation code", "", UI_DIALOG_TYPE_WARNING);
            return VasResult(VAS_ERROR);
        }

        break;
    }

    return VasResult(NO_ERRORS);
}

VasResult Jamb::lookup()
{
    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
    VasResult result = viewModel.lookup();   

    if (result.error != NO_ERRORS) {
        return result;
    }

    std::string message = viewModel.getFirstName() + " " + viewModel.getMiddleName() + " " + viewModel.getSurname()
                            + "\nAmount: " + majorDenomination(viewModel.getAmount()); 

    int i = UI_ShowOkCancelMessage(600000, "Please Confirm", message.c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        result.error = USER_CANCELLATION;
    }

    return result;
}

VasResult Jamb::initiate()
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

    return result;
}

Service Jamb::vasServiceType()
{
    return viewModel.getService();
}

VasResult Jamb::complete()
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

std::map<std::string, std::string> Jamb::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}

void Jamb::statusCheck(const char* title, VasComProxy& comProxy)
{
    Service services[] = { JAMB_UTME_PIN, JAMB_DE_PIN };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    const Service service = selectService("Status Check", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        return;
    }

    const std::string phoneNumber = getPhoneNumber("Phone Number", "");
    if (phoneNumber.empty()) {
        return;
    }

    const std::string confirmationCode = getNumeric(0, 60000, "Confirmation Code", "Enter code", UI_DIALOG_TYPE_NONE);
    if (confirmationCode.empty()) {
        return;
    }

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
    VasResult result = JambViewModel::requery(comProxy, phoneNumber, confirmationCode, service);

    if (result.error != NO_ERRORS) {
        return;
    }

    iisys::JSObject json;
    if (!json.load(result.message)) {
        return;
    }

    JambViewModel viewModel(title, comProxy);
    result = viewModel.processPaymentResponse(json);

    if (result.error != NO_ERRORS) {
        UI_ShowButtonMessage(60000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_NONE);
        return;
    }

    std::map<std::string, std::string> record = viewModel.storageMap(result);
    displayStatusCheck(record);
}

void Jamb::displayStatusCheck(std::map<std::string, std::string>& record)
{
    iisys::JSObject json;
    const std::string name = record[VASDB_BENEFICIARY_NAME];
    const std::string phoneNumber = record[VASDB_BENEFICIARY_PHONE];

    if (!json.load(record[VASDB_SERVICE_DATA])) {
        UI_ShowButtonMessage(60000, "Error", "Service data not found", "OK", UI_DIALOG_TYPE_NONE);
        return;
    }

    const iisys::JSObject& pinObj= json("pin");

    if (!pinObj.isString()) {
        UI_ShowButtonMessage(60000, "Error", "Pin not found", "OK", UI_DIALOG_TYPE_NONE);
        return;
    }

    const std::string pin = pinObj.getString();

    if (name.empty() || phoneNumber.empty() || pin.empty()) {
        UI_ShowButtonMessage(60000, "Error", "", "OK", UI_DIALOG_TYPE_NONE);
        return;
    }
    
    std::string message = name + "\nPhone: " + phoneNumber + "\n\nPIN: " + pin;
    UI_ShowButtonMessage(60000, "Status", message.c_str(), "OK", UI_DIALOG_TYPE_NONE);

}

int Jamb::menu(VasFlow& flow, const char* title, VasComProxy& comProxy)
{
    std::vector<std::string> menu;

    menu.push_back("Buy E-PIN");
    menu.push_back("Status Check");

    const int selection = UI_ShowSelection(30000, title, menu, 0);
    if (selection == 0) {
        Jamb jamb(title, comProxy);
        return flow.start(&jamb);
    } else if (selection == 1) {
        Jamb::statusCheck(title, comProxy);
        return 0;
    }

    return -1;
}
