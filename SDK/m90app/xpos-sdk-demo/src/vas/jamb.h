#ifndef VAS_JAMB_H
#define VAS_JAMB_H

#include "vas.h"
#include "vascomproxy.h"
#include "vasflow.h"

#include "viewmodels/jambviewmodel.h"

struct Jamb : FlowDelegate {

    Jamb(const char* title, VasComProxy& proxy);
    virtual ~Jamb();

    VasResult beginVas();
    VasResult lookup();
    VasResult initiate();
    VasResult complete();

    Service vasServiceType();
    std::map<std::string, std::string> storageMap(const VasResult& completionStatus);
    static void statusCheck(const char* title, VasComProxy& comProxy);

    static int menu(VasFlow& flow, const char* title, VasComProxy& comProxy);

private:
    std::string _title;

    JambViewModel viewModel;

    static void displayStatusCheck(std::map<std::string, std::string>& record);
};

#endif
