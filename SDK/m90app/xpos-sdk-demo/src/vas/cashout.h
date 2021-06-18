#ifndef VAS_CASHOUT_H
#define VAS_CASHOUT_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"
#include "viewmodels/cashioViewModel.h"


struct Cashout : FlowDelegate {

    Cashout(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~Cashout();

protected:
    std::string _title;
    ViceBankingViewModel viewModel;

    VasResult initiateCardlessTransaction();
};

#endif
