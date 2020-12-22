#ifndef VAS_PAYTV_H
#define VAS_PAYTV_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "viewmodels/paytvViewModel.h"

#include "jsonwrapper/jsobject.h"

struct PayTV : FlowDelegate {

    PayTV(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);
    
    Service vasServiceType();

    PayTVViewModel::StartimesType getStartimesType(const char* title);

    virtual ~PayTV();

    private:
    PayTVViewModel viewModel;
    VasResult displayLookupInfo();

};

#endif
