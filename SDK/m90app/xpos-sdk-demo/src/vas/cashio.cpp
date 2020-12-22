#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctype.h>
#include <iomanip>

#include "platform/platform.h"

#include "cashio.h"
#include "payvice.h"

ViceBanking::ViceBanking(const char* title, VasComProxy& proxy)
    : _title(title)
    , viewModel(title, proxy)
{
}

ViceBanking::~ViceBanking()
{
}

VasResult ViceBanking::beginVas()
{
    VasResult result;
    Service services[] = {TRANSFER, WITHDRAWAL};
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    const Service service = selectService(_title.c_str(), serviceVector);
    result = viewModel.setService(service);
    if (result.error != NO_ERRORS) {
        result.error = USER_CANCELLATION;
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

    if (service == TRANSFER) {
        const std::string beneficiaryAccount = getBeneficiaryAccount("Beneficairy Account", "Transfer");
        if (beneficiaryAccount.empty()) {
            result.error = USER_CANCELLATION;
            return result;
        }

        const size_t bankIndex = selectBank();
        if (viewModel.setBeneficiary(beneficiaryAccount, bankIndex).error != NO_ERRORS) {
            return VasResult(USER_CANCELLATION);
        }
    } 

    result.error = NO_ERRORS;

    return result;
}

VasResult ViceBanking::lookup()
{
    VasResult result;

    if (viewModel.getService() == WITHDRAWAL) {
        PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_MCASH | PAY_WITH_CGATE));
        if (payMethod == PAYMENT_METHOD_UNKNOWN) {
            result.error = USER_CANCELLATION;
            return result;
        }

        result = viewModel.setPaymentMethod(payMethod);
        if (result.error) {
            return result;
        }
    }

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    result = viewModel.lookup();
    if (result.error != NO_ERRORS) {
        UI_ShowButtonMessage(30000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
    }

    result = displayLookupInfo();

    return result;
}

VasResult ViceBanking::initiate()
{
    VasResult response;
    
    const std::string phoneNumber = getPhoneNumber("Phone Number", "");
    if (phoneNumber.empty()) {
        response = USER_CANCELLATION;
        return response;
    }

    response = viewModel.setPhoneNumber(phoneNumber);
    if (response.error != NO_ERRORS) {
        UI_ShowButtonMessage(30000, "Error", response.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        return response;
    }

    if (viewModel.getService() == TRANSFER) {
        std::string narration;
        getText(narration, 0, 30000, "Narration", "", UI_DIALOG_TYPE_NONE);
        viewModel.setNarration(narration);
    }

    if (!viewModel.isPaymentUSSD()) {
        response.error = NO_ERRORS;
        return response;
    }

    // initiate USSD transaction 

    std::string pin;

    response.error = getVasPin(pin);
    if (response.error != NO_ERRORS) {
        return response;
    }
    
    Demo_SplashScreen("Initiating Payment...", "www.payvice.com");
    response = viewModel.initiate(pin);
    if (response.error) {
        return response;
    }

    if (response.error == NO_ERRORS) {
        std::string displayTitle;

        if (viewModel.getPaymentMethod() == PAY_WITH_CGATE) {
            displayTitle = "Continue on Phone";
        } else if (viewModel.getPaymentMethod() == PAY_WITH_MCASH) {
            displayTitle = "Check Your Phone";
        }

        UI_ShowButtonMessage(10 * 60 * 1000, displayTitle.c_str(), response.message.c_str(), "OK", UI_DIALOG_TYPE_CONFIRMATION);
    }

    return response;
}

Service ViceBanking::vasServiceType()
{
    return viewModel.getService();
}

VasResult ViceBanking::complete()
{
    VasResult response;
    std::string pin;

    if (!viewModel.isPaymentUSSD()) {
        response.error = getVasPin(pin);
        if (response.error != NO_ERRORS) {
            return response;
        }
    }

    Demo_SplashScreen("Payment In Progress", "www.payvice.com");
    response = viewModel.complete(pin);
    
    return response;
}

std::map<std::string, std::string> ViceBanking::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);
    return record;
}


size_t ViceBanking::selectBank() const
{
    std::vector<std::string> menu;

    Demo_SplashScreen("Loading Banks...", "www.payvice.com");
    const std::vector<ViceBankingViewModel::Bank>& banks = viewModel.banklist();

    if (banks.empty()) {
        return -1;
    }
    
    for (size_t i = 0; i < banks.size(); ++i) {
        menu.push_back(banks[i].name);
    }

    const int selection = UI_ShowSelection(30000, "Banks", menu, 0);

    return static_cast<size_t>(selection);
}

std::string ViceBanking::getBeneficiaryAccount(const char* title, const char* prompt)
{
    char account[16] = { 0 };

    if (getNumeric(account, 10, 10, 0, 30000, title, prompt, UI_DIALOG_TYPE_NONE) < 0) {
        return std::string("");
    }

    return std::string(account);
}

VasResult ViceBanking::displayLookupInfo() const
{
    std::ostringstream confirmationMessage;


    if (viewModel.getService() == TRANSFER) {
        if (viewModel.getName().empty()) {
            return VasResult(KEY_NOT_FOUND, "Name not found");
        }

        confirmationMessage << viewModel.getName() << std::endl;
        confirmationMessage << "NUBAN: " << viewModel.getBeneficiaryAccount() << std::endl;
        confirmationMessage << "Amount: " <<  std::fixed << std::setprecision(2) <<  viewModel.getAmount() / 100.0 << std::endl;

    } else if (viewModel.getService() == WITHDRAWAL) {
        confirmationMessage << "Amount: " <<  std::fixed << std::setprecision(2) <<  viewModel.getAmountToDebit() / 100.0 << std::endl;
        confirmationMessage << "Settlement: " <<  std::fixed << std::setprecision(2) <<  viewModel.getAmountSettled() / 100.0 << std::endl;        
    }

    int i = UI_ShowOkCancelMessage(30000, "Please Confirm", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        return VasResult(USER_CANCELLATION);
    }

    return VasResult(NO_ERRORS);
}
