#ifndef VAS_ELECTRICITY_H
#define VAS_ELECTRICITY_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "jsonwrapper/jsobject.h"

#include "viewmodels/electricityViewModel.h"

int electricity(const char* title);

struct Electricity : FlowDelegate {

    Electricity(const char* title, VasComProxy& proxy);

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();

    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);

    virtual ~Electricity();
    
protected:
    ElectricityViewModel viewModel;
    
    ElectricityViewModel::EnergyType getEnergyType(Service service, const char * title);
    std::string getMeterNo(const char* prompt);
    void removeCard(const char* title, const char* desc);
};

#endif
