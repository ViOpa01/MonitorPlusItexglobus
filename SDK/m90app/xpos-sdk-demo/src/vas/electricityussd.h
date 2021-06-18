#ifndef VAS_USSD_ELECTRICITY_H
#define VAS_USSD_ELECTRICITY_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"

#include "viewmodels/electricityUssdViewModel.h"


struct ElectricityUssd : FlowDelegate {

    ElectricityUssd(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~ElectricityUssd();
    
private:
    ElectricityUssdViewModel viewModel;
};

#endif
