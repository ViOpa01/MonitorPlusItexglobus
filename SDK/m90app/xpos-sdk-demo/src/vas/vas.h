#ifndef SRC_VAS_H
#define SRC_VAS_H

#include <string>
#include <map>
#include "jsobject.h"

typedef enum {
    ENERGY,
    AIRTIME,
    TV_SUBSCRIPTIONS,
    DATA,
    SMILE,
    CASHIO
} VAS_Menu_T;

typedef enum {
    SERVICE_UNKNOWN,
    IKEJA,
    EKEDC,
    EEDC,
    IBEDC,
    PHED,
    AEDC,
    KEDCO,
    AIRTELVTU,
    AIRTELVOT,
    AIRTELVOS,
    AIRTELDATA,
    ETISALATVTU,
    ETISALATVOT,
    ETISALATVOS,
    ETISALATDATA,
    GLOVTU,
    GLOVOT,
    GLOVOS,
    GLODATA,
    MTNVTU,
    MTNVOT,
    MTNVOS,
    MTNDATA,
    DSTV,
    GOTV,
    SMILETOPUP,
    SMILEBUNDLE,
    STARTIMES,
    WITHDRAWAL,
    TRANSFER
} Service;

typedef enum {
    PAYMENT_METHOD_UNKNOWN,
    PAY_WITH_CARD = 1 << 0,
    PAY_WITH_CASH = 1 << 2,
    PAY_WITH_MCASH = 1 << 3
} PaymentMethod;

typedef enum {
    VAS_ERROR = -1,
    NO_ERRORS,
    USER_CANCELLATION,
    INVALID_JSON,
    TYPE_MISMATCH,
    KEY_NOT_FOUND,
    INPUT_ABORT,
    INPUT_TIMEOUT_ERROR,
    INPUT_ERROR,
    TXN_PENDING,
    STATUS_ERROR,
    TXN_NOT_FOUND,
    CARD_APPROVED,
    NOT_LOGGED_IN
} VasError;

struct VasStatus {
    VasError error;
    std::string message;

    VasStatus(VasError error = VAS_ERROR, const char* message = "")
        : error(error)
        , message(message)
    {
    }
};


int vasTransactionTypes();
int doVasTransaction(VAS_Menu_T menu);

const char* vasMenuString(VAS_Menu_T type);
const char* serviceToString(Service service);
const char* paymentString(PaymentMethod method);
const char* serviceToProductString(Service service);

PaymentMethod getPaymentMethod(const PaymentMethod preferredMethods);

Service selectService(const char * title, std::vector<Service>& services);
VasStatus vasErrorCheck(const iisys::JSObject& data);
VasError requeryVas(iisys::JSObject& transaction, const char * clientRef, const char *serverRef);

int printVas(std::map<std::string, std::string>& record);

std::string getCurrencySymbol();
const char * getDeviceTerminalId();

char menuendl();

#endif
