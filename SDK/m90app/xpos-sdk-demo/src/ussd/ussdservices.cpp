#include "ussdservices.h"

const char* ussdServiceToString(USSDService service)
{
    switch (service) {
    case CGATE_PURCHASE:
        return "C'Gate Purchase";
    case CGATE_CASHOUT:
        return "C'Gate Cashout";
    case MCASH_PURCHASE:
        return "mCash Purchasse";
    case PAYATTITUDE_PURCHASE:
        return "PayAttitude";
    default:
        return "Unknown";
    }

}


