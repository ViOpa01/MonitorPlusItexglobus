#ifndef VAS_SMILE_H
#define VAS_SMILE_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"

#include "viewmodels/smileViewModel.h"

struct Smile : FlowDelegate {

    Smile(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~Smile();

    
protected:
    SmileViewModel viewModel;
    VasResult initiateCardlessTransaction();

};

#endif
