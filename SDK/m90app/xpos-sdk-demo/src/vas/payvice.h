#ifndef VAS_PAYVICE_H
#define VAS_PAYVICE_H

#include <fstream>
#include <string>
#include <sys/stat.h>

#include "jsonwrapper/jsobject.h"
#include "vascomproxy.h"

#define PAYVICE_CONF "itex/payvice.json"
#define SHA512_DIGEST_LENGTH 64

#define rot13(word)                                                        \
    {                                                                      \
        size_t i = strlen(word);                                           \
        while (i--) {                                                      \
            char c = word[i];                                              \
            if ((c >= 'A' && c <= 'M') || (c >= 'a' && c <= 'm')) {        \
                word[i] += '\r';                                           \
            } else if ((c >= 'N' && c <= 'Z') || (c >= 'n' && c <= 'z')) { \
                word[i] -= '\r';                                           \
            }                                                              \
        }                                                                  \
    }


struct Payvice {

    static const char* USER;
    static const char* PASS;
    static const char* WALLETID;
    static const char* KEY;

    static const char* VIRTUAL;
    static const char* TID;
    static const char* MID;
    static const char* NAME;
    static const char* SKEY;
    static const char* PKEY;
    static const char* CUR_CODE;
    static const char* COUNTRYCODE;
    static const char* MCC;

    static const char* IS_AGENT;

    typedef enum {
        NOERROR,
        SOME_ERROR_OCCURED,
        FILE_NOT_EXIST,
        SAVE_ERR
    } FileErr;

    iisys::JSObject object;

    Payvice();

    int resetFile();
    static int resetApiToken();

    std::string getApiToken();
    
    int error() const;

    bool isFileExists(const char* filename);

    int save();

private:
    FileErr err;
    std::string _filename;
    static const char* TOKEN;
    static const char* TOKEN_EXP;

    std::string fetchVasToken();
    int extractVasToken(const std::string& response, const time_t now);
};

int logIn(Payvice& payvice);
int logOut(Payvice& payvice);
bool loggedIn(const Payvice& payvice);

std::string encryptedPassword(const Payvice& payvice);
std::string encryptedPin(const Payvice& payvice, const char* pin);

std::string getClientReference(std::string& customReference);
std::string getClientReference();

int beginLoginSequence(Payvice& payvice);

#endif
