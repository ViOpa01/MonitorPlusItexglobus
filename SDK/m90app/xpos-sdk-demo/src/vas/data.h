#ifndef VAS_DATA_H
#define VAS_DATA_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"

#include "viewmodels/dataViewModel.h"


struct Data : FlowDelegate {

    Data(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~Data();
    
protected:
    DataViewModel viewModel;
};

#endif
