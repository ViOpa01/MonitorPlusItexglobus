#ifndef VAS_CASHIN_H
#define VAS_CASHIN_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"
#include "viewmodels/cashioViewModel.h"


struct Cashin : FlowDelegate {

    Cashin(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~Cashin();

protected:
    std::string _title;
    ViceBankingViewModel viewModel;

    int selectBank() const;
    std::string getBeneficiaryAccount(const char *title, const char* prompt);

    VasResult displayLookupInfo() const;
};

#endif