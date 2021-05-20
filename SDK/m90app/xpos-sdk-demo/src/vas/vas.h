#ifndef SRC_VAS_H
#define SRC_VAS_H

#include <string>
#include <map>
#include <vector>
#include "vasmenu.h"
#include "jsonwrapper/jsobject.h"


#define VAS_PENDING_RESPONSE_CODE "25"
#define VAS_IN_PROGRESS_RESPONSE_CODE "28"
#define VAS_TXN_NOT_FOUND_RESPONSE_CODE "29"

#define VAS_EXCEPTION_RESPONSE_CODE "91"
#define VAS_STATUS_UNKNOWN_RESPONSE_CODE "92"
#define VAS_SYSTEM_ERROR_RESPONSE_CODE "99"

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
    TRANSFER,
    JAMB_UTME_PIN,
    JAMB_DE_PIN
} Service;

typedef enum {
    PAYMENT_METHOD_UNKNOWN,
    PAY_WITH_CARD  = 1 << 0,
    PAY_WITH_CASH  = 1 << 2,
    PAY_WITH_MCASH = 1 << 3,
    PAY_WITH_CGATE = 1 << 4,
    PAY_WITH_NQR   = 1 << 5
} PaymentMethod;

typedef enum {
    VAS_ERROR = -1,
    NO_ERRORS,
    USER_CANCELLATION,
    INVALID_JSON,
    EMPTY_VAS_RESPONSE,
    LOOKUP_ERROR,
    TYPE_MISMATCH,
    KEY_NOT_FOUND,
    INPUT_ABORT,
    INPUT_TIMEOUT_ERROR,
    INPUT_ERROR,
    TXN_PENDING,
    STATUS_ERROR,
    TXN_NOT_FOUND,
    CARD_APPROVED,
    CASH_STATUS_UNKNOWN,
    CARD_STATUS_UNKNOWN,
    VAS_STATUS_UNKNOWN,
    VAS_DECLINED,
    CARD_PAYMENT_DECLINED,
    NOT_LOGGED_IN,
    SMART_CARD_DETECT_ERROR,
    SMART_CARD_UPDATE_ERROR,
    TOKEN_UNAVAILABLE
} VasError;

struct VasResult {
    VasError error;
    std::string message;

    VasResult(VasError error = VAS_ERROR, const char* message = "")
        : error(error)
        , message(message)
    {
    }
};

int initVasTables();

const char* serviceToString(Service service);
const char* paymentString(PaymentMethod method);
const char* apiPaymentMethodString(PaymentMethod method);
const char* serviceToProductString(Service service);

PaymentMethod getPaymentMethod(const PaymentMethod preferredMethods);

Service selectService(const char * title, const std::vector<Service>& services);
VasResult vasResponseCheck(const iisys::JSObject& response);

void getVasTransactionReference(std::string& reference, const iisys::JSObject& responseData);
void getVasTransactionSequence(std::string& sequence, const iisys::JSObject& responseData);
void getVasTransactionDateTime(std::string& dateTime, const iisys::JSObject& responseData);

VasError requeryVas(iisys::JSObject& transaction, const char * sequence);

int printVas(std::map<std::string, std::string>& record);

int printVasReceipt(std::map<std::string, std::string> &record, const VAS_Menu_T type);
int printVasEod(std::map<std::string, std::string> &records);

std::string getVasCurrencySymbol();
unsigned long getVasAmount(const char* title);

std::string majorDenomination(unsigned long amount);

VasError getVasPin(std::string& pin, const char* message = "Payvice Pin");

std::string vasApplicationVersion();

const char* vasChannel();
std::string getRefCode(const std::string& vasCategory, const std::string& vasProduct = "");
void printVasFooter();

#endif
