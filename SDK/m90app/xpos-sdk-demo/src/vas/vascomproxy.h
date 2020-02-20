#ifndef VAS_VASCOMPROXY_H
#define VAS_VASCOMPROXY_H


#include "vas.h"
#include "vas/jsobject.h"

#include "Nibss8583.h"

struct CardPurchase {
    unsigned long amount;
    std::string refcode;
    unsigned long primaryIndex;
    std::string purchaseTid;
    Eft trxContext;
};

struct VasComProxy {

    virtual ~VasComProxy() {};

    virtual VasStatus lookup(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>* headers = 0) = 0;
    virtual VasStatus initiate(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>* headers = 0, CardPurchase* = 0) = 0;
    virtual VasStatus complete(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>* headers = 0, CardPurchase* = 0) = 0;
    virtual VasStatus reverse(CardPurchase& cardPurchase) = 0;
};

struct Postman : public VasComProxy {

    Postman();
    virtual ~Postman();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             

    VasStatus lookup(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>*);
    VasStatus initiate(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>*, CardPurchase*);
    VasStatus complete(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>*, CardPurchase*);
    VasStatus reverse(CardPurchase& cardPurchase);

private:
    VasStatus
    doRequest(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>* headers, CardPurchase*);
    
    VasStatus
    sendVasRequest(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>* headers);
    
    VasStatus
    sendVasCardRequest(const char* url, const iisys::JSObject*, const std::map<std::string, std::string>* headers, CardPurchase*);

    std::string generateRequestAuthorization(const std::string& requestBody, const std::string& date);

    std::string vas;

};
#endif
