#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <ctype.h>

#include "platform/platform.h"

#include "vasdb.h"

#include "paytvussd.h"
#include "vasussd.h"

PaytvUssd::PaytvUssd(const char* title, VasComProxy& proxy)
    : viewModel(title, proxy)
{
}

PaytvUssd::~PaytvUssd()
{
}

VasResult PaytvUssd::beginVas()
{
    VasResult result;
    Service services[] = { DSTV, GOTV, STARTIMES };
    std::vector<Service> serviceVector(services, services + sizeof(services) / sizeof(Service));

    const Service service = selectService("Services", serviceVector);
    if (service == SERVICE_UNKNOWN) {
        result = VasResult(USER_CANCELLATION); 
        return result;
    } else if (viewModel.setService(service).error != NO_ERRORS) {
        result = VasResult(VAS_ERROR);
        return result;
    }

    return VasResult(NO_ERRORS);
}

VasResult PaytvUssd::bouquetLookup()
{
    VasResult result;

    Demo_SplashScreen("Loading Bouquets", "www.payvice.com");

    result = viewModel.bouquetLookup();
    if (result.error != NO_ERRORS) {
        return result;
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

VasResult PaytvUssd::lookup()
{
    VasResult result;

    result = bouquetLookup();
    if (result.error != NO_ERRORS) {
        return result;
    }

    const int units = vasussd::getUnits();
    if (units == 0) {
        result.error = USER_CANCELLATION;
        return result;
    }

    result = viewModel.setUnits(units);
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

    Demo_SplashScreen("Lookup In Progress", "www.payvice.com");
    result = viewModel.lookup();
    if (result.error != NO_ERRORS) {
        return result;
    }

    result = vasussd::displayLookup(viewModel.getAmount(), viewModel.getUnits());

    return result;
}

VasResult PaytvUssd::initiate()
{
    return VasResult(NO_ERRORS);
}

VasResult PaytvUssd::complete()
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

Service PaytvUssd::vasServiceType()
{
    return viewModel.getService();
}

std::map<std::string, std::string> PaytvUssd::storageMap(const VasResult& completionStatus)
{
    std::map<std::string, std::string> record = viewModel.storageMap(completionStatus);

    return record;
}
