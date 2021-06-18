#ifndef VAS_AIRTIME_USSD_H
#define VAS_AIRTIME_USSD_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "viewmodels/airtimeUssdViewModel.h"


struct AirtimeUssd : FlowDelegate {


    AirtimeUssd(const char* title, VasComProxy& proxy);
    virtual ~AirtimeUssd();

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);


private:
    std::string _title;

    AirtimeUssdViewModel viewModel;
};

#endif
