#ifndef VAS_PAYVICE_H
#define VAS_PAYVICE_H

#include <openssl/sha.h>
#include "filejson.h"


#define PAYVICE_CONF "itex/payvice.json"    // To be in sync with private directory

extern int formatAmount(std::string& ulAmount);

#define rot13(word)                                                                 \
{                                                                                   \
    size_t i = strlen(word);                                                        \
    while (i--) {                                                                   \
        char c = word[i];                                                           \
        if ((c >= 'A' && c <= 'M') || (c >= 'a' && c <= 'm')) {                     \
            word[i] += '\r';                                                        \
        } else if ((c >= 'N' && c <= 'Z') || (c >= 'n' && c <= 'z')) {              \
            word[i] -= '\r';                                                        \
        }                                                                           \
    }                                                                               \
}                                                                                   \

typedef struct {
    char nonce[33];
    char signature[SHA512_DIGEST_LENGTH * 2 + 1];
} KeyChain;

struct Payvice : public FileJson {

    static const char* USER;
    static const char* PASS;
    static const char* WALLETID;
    static const char* KEY;
    static const char* SESSION;

    static const char* VIRTUAL;
    static const char* TID;
    static const char* MID;
    static const char* NAME;
    static const char* SKEY;
    static const char* PKEY;
    static const char* CUR_CODE;
    static const char* COUNTRYCODE;
    static const char* MCC;

    Payvice()
        : FileJson(PAYVICE_CONF)
    {
        if (!error() || err != FILE_NOT_EXIST) {
            return;
        }

        // File doesn't exist
        resetFile();
    }

    int resetFile()
    {
        object(WALLETID) = "";
        object(USER) = "itextest@hotmail.com";
        object(PASS) = "God@777";
        object(KEY) = "";
        object(SESSION) = "";

        save();
        return 0;
    }

private:
    std::string _filename;
};

int logIn(Payvice& payvice);
int logOut(Payvice& payvice);
bool loggedIn(const Payvice& payvice);

std::string encryptedPassword(const Payvice& payvice);
std::string encryptedPin(const Payvice& payvice, const char* pin);

int injectPayviceCredentials(iisys::JSObject& obj);

std::string getClientReference();

int cashIOVasToken(const char *msg, const char *prefix, KeyChain *keys);



#endif
