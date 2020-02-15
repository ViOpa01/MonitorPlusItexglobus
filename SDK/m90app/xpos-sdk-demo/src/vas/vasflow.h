#ifndef VAS_VASFLOW_H
#define VAS_VASFLOW_H



#include <map>
#include <sys/time.h>

#include "EmvDB.h"
// #include "EmvDBUtil.h"
#include "vasdb.h"

#include "vas.h"
#include "vascomproxy.h"
#include "jsobject.h"
#include "util.h"

#include "payvice.h"
#include "virtualtid.h"

extern "C" {
#include "libapi_xpos/inc/libapi_print.h"
}

struct FlowDelegate {
    virtual VasStatus beginVas() = 0;
    virtual VasStatus lookup(const VasStatus& beginStatus) = 0;
    virtual VasStatus initiate(const VasStatus& lookupStatus) = 0;
    virtual VasStatus complete(const VasStatus& initiationStatus) = 0;

    virtual Service vasServiceType() = 0;
    virtual std::map<std::string, std::string> storageMap(const VasStatus& completionStatus) = 0;
};


template <typename T>
struct VasFlow_T {

    int start(FlowDelegate* delegate) // final
    {
        Payvice payvice;

        if (!loggedIn(payvice) && logIn(payvice) < 0) {
            return -1;
        }

        if (!virtualConfigurationIsSet()) {
            Demo_SplashScreen("Configuration...", "www.payvice.com");
            if (resetVirtualConfiguration() < 0) {
                return -1;
            }
        }


        return static_cast<T*>(this)->startImplementation(delegate);
    }

    int startImplementation(FlowDelegate* delegate)
    {
        if (!delegate) {
            return -1;
        }
        
        VasStatus beginStatus = delegate->beginVas();
        if (beginStatus.error) {
            if (beginStatus.error != USER_CANCELLATION) {
                UI_ShowButtonMessage(30000, "", beginStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
            }
            return -1;
        }

        VasStatus lookupStatus = delegate->lookup(beginStatus);
        if (lookupStatus.error) {
            if (lookupStatus.error != USER_CANCELLATION) {
                UI_ShowButtonMessage(30000, "", lookupStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
            }
            return -1;
        }

        VasStatus initiateStatus = delegate->initiate(lookupStatus);
        if (initiateStatus.error) {
            if (initiateStatus.error != USER_CANCELLATION) {
                UI_ShowButtonMessage(30000, "", initiateStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
            }
            return -1;
        }

        
        VasStatus completeStatus = delegate->complete(initiateStatus);

        std::map<std::string, std::string> record = delegate->storageMap(completeStatus);
        if (record.find(VASDB_DATE) == record.end() || record[VASDB_DATE].empty()) {
            char dateTime[32] = { 0 };
            formattedDateTime(dateTime, sizeof(dateTime));
            record[VASDB_DATE] = std::string(dateTime).append(".000");
        }
        long index = VasDB::saveVasTransaction(record);

        if (completeStatus.error) {
            printf("Vas Delined Message\n");
             UI_ShowButtonMessage(3000, "Declined", completeStatus.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        } else {
             UI_ShowButtonMessage(3000, "Approved", completeStatus.message.c_str(), "OK", UI_DIALOG_TYPE_CONFIRMATION);
        }

        if (index <= 0) {
            UI_ShowButtonMessage(3000, "Error", "Could not persist record", "OK", UI_DIALOG_TYPE_WARNING);
            return -1;
        }

        VasDB database;
        database.select(record, (unsigned long)index);

       
        record["walletId"] = Payvice().object(Payvice::WALLETID).getString();
        int printStatus = printVas(record);
        if (printStatus != UPRN_SUCCESS) {

        }

        return 0;
    }

private:
    VasFlow_T()
    {
    }
    friend T;
};

template <typename T>
void startVas(T& base, FlowDelegate* delegate)
{
    base.start(delegate);
}

struct VasFlow : VasFlow_T<VasFlow> {
};

#endif
