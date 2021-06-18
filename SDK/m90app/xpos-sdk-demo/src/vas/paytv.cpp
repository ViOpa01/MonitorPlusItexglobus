#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <ctype.h>

#include "platform/platform.h"

#include "vasdb.h"
#include "nqr.h"

#include "paytv.h"

PayTV::PayTV(const char* title, VasComProxy& proxy)
    : viewModel(title, proxy)
{
}

PayTV::~PayTV()
{
}

VasResult PayTV::beginVas()
{
    Service services[] = { DSTV, GOTV, STARTIMES };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    while (1) {
        const Service service = selectService("Services", serviceVector);
        if (service == SERVICE_UNKNOWN) {
            return VasResult(USER_CANCELLATION);
        } else if (viewModel.setService(service).error != NO_ERRORS) {
            return VasResult(VAS_ERROR);
        }

        if (service == STARTIMES) {
            PayTVViewModel::StartimesType type = getStartimesType("Startimes Type");

            if(type == PayTVViewModel::UNKNOWN_TYPE) {
                return VasResult(USER_CANCELLATION);
            } else if(viewModel.setStartimesType(type).error != NO_ERRORS) {
                return VasResult(VAS_ERROR);
            } else if (type == PayTVViewModel::STARTIMES_TOPUP) {
                unsigned long amount = getVasAmount(serviceToString(viewModel.getService()));

                if (!amount) {
                    return VasResult(USER_CANCELLATION);;
                } else if (viewModel.setAmount(amount).error != NO_ERRORS) {
                    return VasResult(VAS_ERROR);
                }

            }
        }

        std::string iuc = getNumeric(0, 60000, "IUC Number", serviceToString(service), UI_DIALOG_TYPE_NONE);

        if (iuc.empty()) {
            return VasResult(USER_CANCELLATION);
        } else if (viewModel.setIUC(iuc).error != NO_ERRORS){
            return VasResult(VAS_ERROR);
        }
        
        break;

    }

    return VasResult(NO_ERRORS);
}

VasResult PayTV::lookup()
{
    VasResult response;
    iisys::JSObject obj;

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");

    response = viewModel.lookup();

    if(response.error != NO_ERRORS) {
        return response;
    }
    
    return displayLookupInfo();
}

VasResult PayTV::initiate()
{
    VasResult result;

    while (1) {
        PaymentMethod payMethod = getPaymentMethod(static_cast<PaymentMethod>(PAY_WITH_CARD | PAY_WITH_NQR | PAY_WITH_CASH));
        if (payMethod == PAYMENT_METHOD_UNKNOWN) {
            result = VasResult(USER_CANCELLATION);
            return result;
        } 

        result = viewModel.setPaymentMethod(payMethod);
        if (result.error != NO_ERRORS) {
            return result;
        } 

        std::string phoneNumber = getPhoneNumber("Phone Number", "");
        if (phoneNumber.empty()) {
            result = VasResult(USER_CANCELLATION);
            return result;
        }

        result = viewModel.setPhoneNumber(phoneNumber);
        if (result.error != NO_ERRORS) {
            return result;
        }

        break;
    }

    if (viewModel.getPaymentMethod() == PAY_WITH_NQR) {
        result = initiateCardlessTransaction();
    }

    return result;
}

VasResult PayTV::initiateCardlessTransaction()
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

VasResult PayTV::complete()
{
    std::string pin;
    VasResult response;

    if (viewModel.getPaymentMethod() == PAY_WITH_CASH) {
        response.error = getVasPin(pin);
        if(response.error != NO_ERRORS) {
            return response;
        }
    }

    Demo_SplashScreen("Payment In Progress", "www.payvice.com");

    response = viewModel.complete(pin);

    return response;
}

std::map<std::string, std::string> PayTV::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}

Service PayTV::vasServiceType()
{
    return DSTV;
}


VasResult PayTV::displayLookupInfo()
{
    std::ostringstream confirmationMessage;

    confirmationMessage << viewModel.lookupResponse.name << std::endl;
    const std::string& accountNo = viewModel.lookupResponse.accountNo;
    if (!accountNo.empty() && viewModel.getIUC() != accountNo) {
        return VasResult(VAS_ERROR, "Account mismatch");
    }
    confirmationMessage << "Accnt: " << viewModel.getIUC() << std::endl;

    if (!viewModel.lookupResponse.bouquet.empty()) {
        confirmationMessage << "Bouquet: " << viewModel.lookupResponse.bouquet << std::endl;
    }

    if (!viewModel.lookupResponse.balance.empty()) {
        confirmationMessage << "Balance: " << viewModel.lookupResponse.balance << std::endl;
    }

    int cancelConfirmation = UI_ShowOkCancelMessage(30000, "Confirm Info", confirmationMessage.str().c_str(), UI_DIALOG_TYPE_NONE);
    if (cancelConfirmation) {
        return VasResult(USER_CANCELLATION);
    } else if (viewModel.getService() == STARTIMES && viewModel.getStartimesTypes() == PayTVViewModel::STARTIMES_TOPUP) {
        return VasResult(NO_ERRORS);
    }

    const iisys::JSObject& bouquets = viewModel.getBouquets();
    
    if (bouquets.isNull() || !bouquets.isArray()) {
        return VasResult(VAS_ERROR, "Bouquets not found");
    }

    const size_t size = bouquets.size();
    std::vector<std::string> menuData;
    std::string cycle;
    int index;
    if (viewModel.getService() == DSTV || viewModel.getService() == GOTV) {
        for (size_t i = 0; i < size; ++i) {
            menuData.push_back(bouquets[i]("name").getString() + vasimpl::menuendl() + bouquets[i]("amount").getString() + " Naira");
        }

        index = UI_ShowSelection(60000, "Bouquets", menuData, 0);
    } else {
        while (1) {
            menuData.clear();
            for (size_t i = 0; i < size; ++i) {
                menuData.push_back(bouquets[i]("name").getString());
            }

            index = UI_ShowSelection(60000, "Bouquets", menuData, 0);
            if (index < 0) {
                return VasResult(USER_CANCELLATION);
            }

            const iisys::JSObject& bouquet = bouquets[index];
            const iisys::JSObject& cycles = bouquet("cycles");

            menuData.clear();
            std::vector<std::string> cyclesVec;
            for (iisys::JSObject::const_iterator it = cycles.begin(); it != cycles.end(); ++it) {
                cyclesVec.push_back(it->first);
                menuData.push_back(it->first + vasimpl::menuendl() + it->second.getString() + " Naira");
            }

            int cycleIndex = UI_ShowSelection(60000, "Cycles", menuData, 0);
            if (cycleIndex < 0) {
                continue;
            }
            cycle = cyclesVec[cycleIndex];
            break;
        }

    }

    if (index < 0) {
        return VasResult(USER_CANCELLATION);
    } else if (viewModel.setSelectedPackage(index, cycle).error != NO_ERRORS) {
        return VasResult(VAS_ERROR);
    }

    return VasResult(NO_ERRORS);
}

PayTVViewModel::StartimesType PayTV::getStartimesType(const char* title)
{
    int selection;
    const char* startimesType[] = { "Bundle", "Topup" };
    std::vector<std::string> menu(startimesType, startimesType + sizeof(startimesType) / sizeof(char*));

    selection = UI_ShowSelection(30000, title, menu, 0);
    switch (selection) {
    case 0:
        return PayTVViewModel::STARTIMES_BUNDLE;
    case 1:
        return PayTVViewModel::STARTIMES_TOPUP;
    default:
        return PayTVViewModel::UNKNOWN_TYPE;
    }
}


