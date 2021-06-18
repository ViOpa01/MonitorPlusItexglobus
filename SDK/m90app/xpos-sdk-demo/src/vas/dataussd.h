#ifndef VAS_DATA_USSD_H
#define VAS_DATA_USSD_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"

#include "viewmodels/dataUssdViewModel.h"


struct DataUssd : FlowDelegate {

    DataUssd(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~DataUssd();
    
private:
    VasResult packageLookup();
    DataUssdViewModel viewModel;

};

#endif
