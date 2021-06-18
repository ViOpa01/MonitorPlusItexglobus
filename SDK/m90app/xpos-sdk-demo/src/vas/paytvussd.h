#ifndef VAS_USSD_PAYTV_H
#define VAS_USSD_PAYTV_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"

#include "viewmodels/paytvUssdViewModel.h"

struct PaytvUssd : FlowDelegate {

    PaytvUssd(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~PaytvUssd();

private:
    VasResult bouquetLookup();
    PaytvUssdViewModel viewModel;
};

#endif
