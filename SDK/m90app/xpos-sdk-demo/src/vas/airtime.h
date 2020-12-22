#ifndef VAS_AIRTIME_H
#define VAS_AIRTIME_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "viewmodels/airtimeViewModel.h"


struct Airtime : FlowDelegate {

    typedef enum {
        AIRTEL,
        ETISALAT,
        GLO,
        MTN
    } Network;

    Airtime(const char* title, VasComProxy& proxy);
    virtual ~Airtime();

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);


private:
    std::string _title;

    AirtimeViewModel viewModel;
    Service getAirtimeService(Network network) const;
    std::string networkString(Network network) const;
};

#endif
