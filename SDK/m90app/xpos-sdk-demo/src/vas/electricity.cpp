#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctype.h>

#include "platform/platform.h"

#include "vasdb.h"

#include "electricity.h"
#include "nqr.h"

Electricity::Electricity(const char* title, VasComProxy& proxy)
    : viewModel(title, proxy)
{
}

Electricity::~Electricity()
{
}

VasResult Electricity::beginVas()
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

    ElectricityViewModel::EnergyType energyType = getEnergyType(viewModel.getService(), serviceToString(service));
    if (energyType == ElectricityViewModel::TYPE_UNKNOWN) {
        result = VasResult(USER_CANCELLATION);
        return result;
    } else if (viewModel.setEnergyType(energyType).error != NO_ERRORS) {
        result = VasResult(VAS_ERROR);
        return result;
    }

    std::string meterNo = getMeterNo(serviceToString(service));
    if (meterNo.empty()) {
        result = VasResult(USER_CANCELLATION);
        return result;
    } else if (viewModel.setMeterNo(meterNo).error != NO_ERRORS) {
        result = VasResult(VAS_ERROR);
        return result;
    }

    result = VasResult(NO_ERRORS);

    return result;
}

ElectricityViewModel::EnergyType Electricity::getEnergyType(Service service, const char* title)
{
    int selection;
    const char* energyTypes[] = { "Token", "Postpaid"};
    std::vector<std::string> menu(energyTypes, energyTypes + sizeof(energyTypes) / sizeof(char*));

    (void) service;     // Will later need this for IE smartcard
    if (service == IKEJA) {
        menu.push_back("SmartCard");
    }

    selection = UI_ShowSelection(30000, title, menu, 0);
    switch (selection) {
    case 0:
        return ElectricityViewModel::PREPAID_TOKEN;
    case 1:
        return ElectricityViewModel::POSTPAID;
    case 2:
        return ElectricityViewModel::PREPAID_SMARTCARD;
    default:
        return ElectricityViewModel::TYPE_UNKNOWN;
    }
}

VasResult Electricity::lookup()
{
    VasResult response;
    std::ostringstream confirmationMessage;   

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
    response = viewModel.lookup();
    
    if (response.error != NO_ERRORS) {
        return response;
    }

    confirmationMessage << viewModel.lookupResponse.name << std::endl << std::endl;
    if (!viewModel.lookupResponse.address.empty()) {
        confirmationMessage << "Addr:" << std::endl << viewModel.lookupResponse.address;
    }

    int i = UI_ShowOkCancelMessage(30000, "Confirm Info", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        return VasResult(USER_CANCELLATION);
    }

    return VasResult(NO_ERRORS);
}

VasResult Electricity::initiate()
{
    VasResult result;
    PaymentMethod payMethod;

    while (1) {
        payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_NQR | PAY_WITH_CASH));
        if (payMethod == PAYMENT_METHOD_UNKNOWN) {
            result.error = USER_CANCELLATION;
            return result;
        } else if (viewModel.setPaymentMethod(payMethod).error != NO_ERRORS) {
            result.error = VAS_ERROR;
            return result;
        }

        std::string phoneNumber = getPhoneNumber("Phone Number", "");
        if (phoneNumber.empty()) {
            continue;
        } else if (viewModel.setPhoneNumber(phoneNumber).error != NO_ERRORS) {
            result.error = VAS_ERROR;
            return result;
        }

        break;
    }

    if (payMethod == PAY_WITH_NQR) {
        result = initiateCardlessTransaction();
    } else {
        result.error = NO_ERRORS;
    }

    return result;
}

VasResult Electricity::initiateCardlessTransaction()
{
    VasResult result;
    std::string pin;

    result.error = getVasPin(pin);
    if (result.error != NO_ERRORS) {
        return result;
    }
        
    Demo_SplashScreen("Initiating Payment...", "www.payvice.com");
    result = viewModel.initiate(pin);
    if (result.error) {
        return result;
    }

    result.error = presentVasQr(result.message, viewModel.getRetrievalReference());
    result.message = "";

    return result;
}

VasResult Electricity::complete()
{
    std::string pin;
    VasResult response;

    if (viewModel.getPaymentMethod() != PAY_WITH_NQR) {
        response.error = getVasPin(pin);
        if (response.error != NO_ERRORS) {
            return response;
        }
    }

    Demo_SplashScreen("Payment In Progress", "www.payvice.com");

    response = viewModel.complete(pin);

    if(viewModel.getEnergyType() == ElectricityViewModel::PREPAID_SMARTCARD) {

        if (response.error == NO_ERRORS) {
            UI_ShowOkCancelMessage(2000, "SMART CARD", "APPROVED", UI_DIALOG_TYPE_NONE);
        } else {
            std::string title = response.error == SMART_CARD_DETECT_ERROR ? "Card Error" : "Update Error";
            UI_ShowOkCancelMessage(30000, title.c_str(), response.message.c_str(), UI_DIALOG_TYPE_NONE);
        }

        removeCard("SMART CARD", "REMOVE CARD!");
    }
    
    return response;
}

Service Electricity::vasServiceType()
{
    return viewModel.getService();
}

std::map<std::string, std::string> Electricity::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}

std::string Electricity::getMeterNo(const char* title)
{
    char number[16] = { 0 };
    std::string prompt;

    if (viewModel.getEnergyType() == ElectricityViewModel::PREPAID_TOKEN) {
        prompt = "Meter No";
    } else if (viewModel.getEnergyType() == ElectricityViewModel::POSTPAID) {
        prompt = "Account No";
    }

    if (viewModel.getEnergyType() == ElectricityViewModel::PREPAID_SMARTCARD) {

        int result = viewModel.readSmartCard(&viewModel.getUserCardInfo());
        std::string smartCardNo;

        if (result) {
            UI_ShowOkCancelMessage(30000, "Read Error", unistarErrorToString(result), UI_DIALOG_TYPE_NONE);
        } else {
            smartCardNo = (const char*)viewModel.getUserCardNo();
        }

        removeCard("SMART CARD", "REMOVE CARD!");
        return smartCardNo;

    } else if (getNumeric(number, 6, sizeof(number) - 1, 0, 30000, title, prompt.c_str(), UI_DIALOG_TYPE_NONE) < 0) {
        return std::string("");
    }

    return std::string(number);
}

void Electricity::removeCard(const char* title, const char* desc)
{
    viewModel.getSmartCardInFunc().removeCustomerCardCb(title, desc);
}

