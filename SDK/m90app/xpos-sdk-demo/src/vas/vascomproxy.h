#ifndef VAS_VASCOMPROXY_H
#define VAS_VASCOMPROXY_H
#include <string.h>

#include <map>
#include <string>

#include <map>
#include <string>

#include "vas.h"
#include "jsonwrapper/jsobject.h"
#include "../merchant.h"

#include "../Nibss8583.h"

// LIVE REQUERY "http://197.253.19.76:8017"
// Test REQUERY "http://197.253.19.76:8006"
// #define REQUERY_IP ""

// Test VAS "http://197.253.19.76:1880"
// Live VAS "http://197.253.19.76:8019"
#define VAS_TEST

#ifdef VAS_TEST
#define VAS_PORT "6565" //"1880"
#define VAS_URL "197.253.19.78"
#define REQUERY_PORT 6565 //8006
#else
#define REQUERY_PORT 8017
#define VAS_PORT "8019"
#endif

struct CardData {
    CardData(const unsigned long amt = 0, const std::string& rrn = "")
        : amount(amt)
        , refcode("")
        , reference(rrn)
        , primaryIndex(0)
        , transactionTid("")
    {
        memset(&trxContext, 0x00, sizeof(Eft));
    };

    unsigned long amount;
    std::string refcode;
    std::string reference;
    unsigned long primaryIndex;
    std::string transactionTid;
    Eft trxContext;

    struct UP {
        UP() : field60("") {}
        std::string field60;
    } upWithdrawal;

};

struct VasComProxy {

    virtual ~VasComProxy() {};

    virtual VasResult lookup(const char* url, const iisys::JSObject*) = 0;
    virtual VasResult initiate(const char* url, const iisys::JSObject*, CardData* = NULL) = 0;
    virtual VasResult complete(const char* url, const iisys::JSObject*, CardData* = NULL, std::map<std::string, std::string>* = NULL) = 0;
    virtual VasResult reverse(CardData& cardData) = 0;

    virtual VasResult sendVasRequest(const char* url, const iisys::JSObject* = NULL, std::map<std::string, std::string>* = NULL) = 0;
};

struct Postman : public VasComProxy {

    Postman(const std::string& hostaddr = "", const std::string& port = VAS_PORT);
    virtual ~Postman();

    VasResult lookup(const char* url, const iisys::JSObject*);
    VasResult initiate(const char* url, const iisys::JSObject*, CardData*);
    VasResult complete(const char* url, const iisys::JSObject*, CardData*, std::map<std::string, std::string>* = NULL);
    VasResult reverse(CardData& cardData);
    VasResult sendVasRequest(const char* url, const iisys::JSObject* = NULL, std::map<std::string, std::string>* = NULL);

    static VasError manualRequery(iisys::JSObject& transaction, const std::string& sequence, const std::string& cardRef);


private:
    VasResult
    doRequest(const char* url, const iisys::JSObject*, CardData*, std::map<std::string, std::string>* = NULL);
    
    VasResult
    sendVasCashRequest(const char* url, const iisys::JSObject*, std::map<std::string, std::string>* recvHeaders);
    
    VasResult
    sendVasCardRequest(const char* url, const iisys::JSObject*, CardData*);

    static std::string generateRequestAuthorization(const std::string& requestBody);

    VasResult requeryVas(const char* clientRef, const char* walletId, const char* token) const;

    std::string txnHost;
    std::string portValue;

};

Postman getVasPostMan();
#endif
