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

#include "smile.h"

Smile::Smile(const char* title, VasComProxy& proxy)
    : viewModel(title, proxy)
{
}

Smile::~Smile()
{
}

VasResult Smile::beginVas()
{
    VasResult result;

    while (1) {
        Service services[] = { SMILETOPUP, SMILEBUNDLE };
        std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

        const Service service = selectService("Smile Services", serviceVector);
        if (service == SERVICE_UNKNOWN) {
            result = VasResult(USER_CANCELLATION);
            return result;
        }

        result = viewModel.setService(service);
        if (result.error != NO_ERRORS) {
            return result;
        }

        const char* optionStr[] = {"Customer ID", "Phone"};
        std::vector<std::string> optionMenu(optionStr, optionStr + sizeof(optionStr) / sizeof(char*));

        int optionSelected = UI_ShowSelection(30000, "Options", optionMenu, 0);
        if (optionSelected < 0) {
            result.error = USER_CANCELLATION;
            return result;
        } else if (optionSelected == 0) {
            const std::string customerID = getNumeric(0, 30000, "Customer ID", "Smile Account No", UI_DIALOG_TYPE_NONE);
            if (customerID.empty()) {
                result = VasResult(USER_CANCELLATION);
                return result;
            }

            result = viewModel.setCustomerID(customerID);
            if(result.error != NO_ERRORS) {
                result = VasResult(result.error, result.message.c_str());
                return result;
            }

        }

        const std::string phoneNumber = getPhoneNumber("Phone Number", "");
        if (phoneNumber.empty()) {
            result.error = USER_CANCELLATION;
            return result;
        }

        result = viewModel.setPhoneNumber(phoneNumber);
        if (result.error != NO_ERRORS) {
            return result;
        }

        break;
    }

    result = VasResult(NO_ERRORS); 
    return result;
}

VasResult Smile::lookup()
{
    VasResult response;
    std::ostringstream confirmationMessage;   
    
    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    response = viewModel.lookup();

    if(response.error != NO_ERRORS) {
        return response;
    }

    confirmationMessage << viewModel.lookupResponse.customerName << std::endl << std::endl;

    int i = UI_ShowOkCancelMessage(30000, "Confirm Info", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        return VasResult(USER_CANCELLATION);
    }

    return VasResult(NO_ERRORS);
}

VasResult Smile::initiate()
{
    VasResult result;

    if (viewModel.getService() == SMILETOPUP) {
        unsigned long amount = getVasAmount("TOP-UP");
        if (!amount) {
            result.error = USER_CANCELLATION;
            return result;
        }

        result = viewModel.setAmount(amount);
        if(result.error != NO_ERRORS) {
            return result;
        }

    } else if (viewModel.getService() == SMILEBUNDLE) {
       
        Demo_SplashScreen("Loading Bundles...", "www.payvice.com");

        result = viewModel.fetchBundles();
        if (result.error != NO_ERRORS) {
            return VasResult(result.error, "Bundle lookup failed");
        }

        const iisys::JSObject& data = viewModel.getBundles();
    
        if (data.isNull() || !data.isArray()) {
            return VasResult(VAS_ERROR, "Data Packages not found");
        }

        const size_t size = data.size();
        std::vector<std::string> menuData;
            for (size_t i = 0; i < size; ++i) {
            std::string amount = data[i]("price").getString();
            std::string validity = data[i]("validity").getString();

            menuData.push_back(data[i]("name").getString() + vasimpl::menuendl() + amount + " Naira, " + validity);
        }

        int selection = UI_ShowSelection(60000, "Bundles", menuData, 0);

        if (selection < 0) {
            return VasResult(USER_CANCELLATION);
        }
        
        viewModel.setSelectedPackageIndex(selection);
    }

    const PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_NQR | PAY_WITH_CASH));
    if (payMethod == PAYMENT_METHOD_UNKNOWN) {
        return VasResult(USER_CANCELLATION);
    }

    result = viewModel.setPaymentMethod(payMethod);
    if (result.error != NO_ERRORS) {
        return result;
    }

    if (viewModel.getPaymentMethod() == PAY_WITH_NQR) {
        result = initiateCardlessTransaction();
    }

    return result;
}

VasResult Smile::initiateCardlessTransaction()
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

VasResult Smile::complete()
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

std::map<std::string, std::string> Smile::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);
    

    return record;
}

Service Smile::vasServiceType()
{
    return viewModel.getService();
}
