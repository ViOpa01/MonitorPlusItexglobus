#ifndef VAS_VASFLOW_H
#define VAS_VASFLOW_H

#include "jsonwrapper/jsobject.h"

#include <map>
#include <sys/time.h>

#include "vasdb.h"

#include "vas.h"
#include "vascomproxy.h"

#include "payvice.h"
#include "virtualtid.h"
#include "./platform/simpio.h"

extern "C" {
#include "libapi_xpos/inc/libapi_print.h"
}

struct FlowDelegate {
    virtual VasResult beginVas() = 0;
    virtual VasResult lookup() = 0;
    virtual VasResult initiate() = 0;
    virtual VasResult complete() = 0;

    virtual Service vasServiceType() = 0;
    virtual std::map<std::string, std::string> storageMap(const VasResult& completionStatus) = 0;

    virtual ~FlowDelegate() {};
};


struct VasFlow {

    int start(FlowDelegate* delegate) // final
    {
        Payvice payvice;

        if (!loggedIn(payvice) && logIn(payvice) < 0) {
            return -1;
        }

        if (!itexIsMerchant() && !virtualConfigurationIsSet()) {
            Demo_SplashScreen("Configuration...", "www.payvice.com");
            if (resetVirtualConfiguration() < 0) {
                return -1;
            }
        }

        return this->startImplementation(delegate);
    }

    int startImplementation(FlowDelegate* delegate)
    {
        int printStatus = -1;
        if (!delegate) {
            return -1;
        }
        
        VasResult beginStatus = delegate->beginVas();
        if (beginStatus.error) {
            if (beginStatus.error != USER_CANCELLATION) {
                UI_ShowButtonMessage(30000, "", beginStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
            }
            return -1;
        }

        VasResult lookupStatus = delegate->lookup();
        if (lookupStatus.error) {
            if (lookupStatus.error != USER_CANCELLATION) {
                UI_ShowButtonMessage(30000, "", lookupStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
            }
            return -1;
        }

        VasResult initiateStatus = delegate->initiate();
        if (initiateStatus.error) {
            if (initiateStatus.error != USER_CANCELLATION) {
                UI_ShowButtonMessage(30000, "", initiateStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
            }
            return -1;
        }

        
        VasResult completeStatus = delegate->complete();

        std::map<std::string, std::string> record = delegate->storageMap(completeStatus);
        if (record.find(VASDB_DATE) == record.end() || record[VASDB_DATE].empty()) {
            char dateTime[32] = { 0 };
            time_t now = time(NULL);
            strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S.000", localtime(&now));
            record[VASDB_DATE] = dateTime;
        }
        long index = VasDB::saveVasTransaction(record);

        if (completeStatus.error) {
             UI_ShowButtonMessage(3000, record[VASDB_STATUS].c_str(), completeStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        } else {
             UI_ShowButtonMessage(3000, "Approved", completeStatus.message.c_str(), "OK", UI_DIALOG_TYPE_CONFIRMATION);
        }
        
        if (index <= 0) {
            UI_ShowButtonMessage(3000, "Error", "Could not persist record", "OK", UI_DIALOG_TYPE_WARNING);
            return -1;
        }

        VasDB database;
        database.select(record, (unsigned long)index);

        // print
        record["walletId"] = Payvice().object(Payvice::WALLETID).getString();
        printStatus = printVas(record);
        if (printStatus != UPRN_SUCCESS) {

        }

        return 0;
    }
};

template <typename T>
void startVas(T& flow, FlowDelegate* delegate)
{
    flow.start(delegate);
}

#endif
