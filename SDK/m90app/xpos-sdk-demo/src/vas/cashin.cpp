#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctype.h>

#include "platform/platform.h"

#include "cashin.h"
#include "payvice.h"

Cashin::Cashin(const char* title, VasComProxy& proxy)
    : _title(title)
    , viewModel(title, proxy)
{
}

Cashin::~Cashin()
{
}

VasResult Cashin::beginVas()
{
    const Service service = TRANSFER;
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

    const std::string beneficiaryAccount = getBeneficiaryAccount("Transfer", "Beneficairy Account");
    if (beneficiaryAccount.empty()) {
        result.error = USER_CANCELLATION;
        return result;
    }

    const int bankIndex = selectBank();
    if (viewModel.setBeneficiary(beneficiaryAccount, bankIndex).error != NO_ERRORS) {
        result.error = USER_CANCELLATION;
        return result;
    }

    result.error = NO_ERRORS;

    return result;
}

VasResult Cashin::lookup()
{
    VasResult result;

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    result = viewModel.lookup();
    if (result.error != NO_ERRORS) {
        UI_ShowButtonMessage(30000, "Error", result.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
    } else {
        result = displayLookupInfo();
    }

    return result;
}

VasResult Cashin::initiate()
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

    std::string narration;
    getText(narration, 0, 30000, "Narration", "", UI_DIALOG_TYPE_NONE);
    viewModel.setNarration(narration);

    return response;
}

Service Cashin::vasServiceType()
{
    return viewModel.getService();
}

VasResult Cashin::complete()
{
    VasResult response;
    std::string pin;

    response.error = getVasPin(pin);
    if (response.error != NO_ERRORS) {
        return response;
    }

    Demo_SplashScreen("Payment In Progress", "www.payvice.com");
    response = viewModel.complete(pin);
    
    return response;
}

std::map<std::string, std::string> Cashin::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);
    return record;
}

int Cashin::selectBank() const
{
    std::vector<std::string> menu;

    Demo_SplashScreen("Loading Banks...", "www.payvice.com");
    const std::vector<ViceBankingViewModel::Bank>& banks = viewModel.banklist();

    if (banks.empty()) {
         UI_ShowButtonMessage(30000, "Error", "Load error", "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }
    
    for (size_t i = 0; i < banks.size(); ++i) {
        menu.push_back(banks[i].name);
    }

    return UI_ShowSelection(30000, "Banks", menu, 0);
}

std::string Cashin::getBeneficiaryAccount(const char* title, const char* prompt)
{
    char account[16] = { 0 };

    if (getNumeric(account, 10, 10, 0, 30000, title, prompt, UI_DIALOG_TYPE_NONE) < 0) {
        return std::string("");
    }

    return std::string(account);
}

VasResult Cashin::displayLookupInfo() const
{
    std::ostringstream confirmationMessage;

    if (viewModel.getName().empty()) {
        return VasResult(KEY_NOT_FOUND, "Name not found");
    }

    confirmationMessage << viewModel.getName() << std::endl;
    confirmationMessage << "NUBAN: " << viewModel.getBeneficiaryAccount() << std::endl;
    confirmationMessage << "Amount: " <<  majorDenomination(viewModel.getAmount()) << std::endl;

    int i = UI_ShowOkCancelMessage(30000, "Please Confirm", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (i != 0) {
        return VasResult(USER_CANCELLATION);
    }

    return VasResult(NO_ERRORS);
}
