#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctype.h>

#include "platform/platform.h"

#include "cashout.h"
#include "payvice.h"
#include "nqr.h"

Cashout::Cashout(const char* title, VasComProxy& proxy)
    : _title(title)
    , viewModel(title, proxy)
{
}

Cashout::~Cashout()
{
}

VasResult Cashout::beginVas()
{
    const Service service = WITHDRAWAL;
    VasResult result = viewModel.setService(service);

    if (result.error != NO_ERRORS) {
        return result;
    }

    const unsigned long amount = getVasAmount(serviceToString(service));
    if (amount == 0) {
        result.error = USER_CANCELLATION;
        return result;
    }
    
    result = viewModel.setAmount(amount);
    if (result.error != NO_ERRORS) {
        return result;
    }

    PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_NQR | PAY_WITH_MCASH | PAY_WITH_CGATE));
    if (payMethod == PAYMENT_METHOD_UNKNOWN) {
        result.error = USER_CANCELLATION;
        return result;
    }

    result = viewModel.setPaymentMethod(payMethod);
    if (result.error == NO_ERRORS && viewModel.isPaymentUSSD()) {
        const std::string phoneNumber = getPhoneNumber("Phone Number", "");
        if (phoneNumber.empty()) {
            result = USER_CANCELLATION;
            return result;
        }

        result = viewModel.setPhoneNumber(phoneNumber);
    }

    return result;
}

VasResult Cashout::lookup()
{
    VasResult result;

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    result = viewModel.lookup();
    if (result.error != NO_ERRORS) {
        UI_ShowButtonMessage(30000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
    }

    return result;
}

VasResult Cashout::initiate()
{
    VasResult response(NO_ERRORS);

    if (viewModel.isPaymentUSSD() || viewModel.getPaymentMethod() == PAY_WITH_NQR) {
        response = initiateCardlessTransaction();
    }

    return response;
}

VasResult Cashout::initiateCardlessTransaction()
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

    if (viewModel.isPaymentUSSD()) {
        std::string displayTitle;

        if (viewModel.getPaymentMethod() == PAY_WITH_CGATE) {
            displayTitle = "Continue on Phone";
        } else if (viewModel.getPaymentMethod() == PAY_WITH_MCASH) {
            displayTitle = "Check Your Phone";
        }

        UI_ShowButtonMessage(10 * 60 * 1000, displayTitle.c_str(), result.message.c_str(), "OK", UI_DIALOG_TYPE_CONFIRMATION);
    } else if (viewModel.getPaymentMethod() == PAY_WITH_NQR) {
        result.error = presentVasQr(result.message, viewModel.getRetrievalReference());
        result.message = "";
    }

    return result;
}

Service Cashout::vasServiceType()
{
    return viewModel.getService();
}

VasResult Cashout::complete()
{
    VasResult response;
    std::string pin;

    if (viewModel.getService() != PAY_WITH_CARD) {
        Demo_SplashScreen("Payment In Progress", "www.payvice.com");
    }
    
    response = viewModel.complete(pin);
    
    return response;
}

std::map<std::string, std::string> Cashout::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);
    return record;
}
