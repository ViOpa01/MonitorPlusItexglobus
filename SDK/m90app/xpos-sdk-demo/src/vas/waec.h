#ifndef VAS_WAEC_H
#define VAS_WAEC_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "viewmodels/waecviewmodel.h"

struct Waec : FlowDelegate {

    Waec(const char* title, VasComProxy& proxy);
    virtual ~Waec();

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

private:
    std::string _title;

    VasResult captureWaecData();

    WaecViewModel viewModel;
};

#endif
