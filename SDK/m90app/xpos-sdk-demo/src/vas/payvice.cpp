#include "payvice.h"

#include <sstream>
#include <string.h>
#include <stdio.h>

#include "simpio.h"

extern "C" {
#include "sha256.h"
#include "util.h"
#include "ezxml.h"
#include "network.h"
}


const char* Payvice::USER = "username";
const char* Payvice::PASS = "passsword";
const char* Payvice::WALLETID = "walletId";
const char* Payvice::KEY = "key";
const char* Payvice::SESSION = "session";

const char* Payvice::VIRTUAL = "virtual";
const char* Payvice::TID = "tid";
const char* Payvice::MID = "mid";
const char* Payvice::NAME = "merchantName";
const char* Payvice::SKEY = "sessionKey";
const char* Payvice::PKEY = "pinKey";
const char* Payvice::CUR_CODE = "currencyCode";
const char* Payvice::COUNTRYCODE = "countryCode";
const char* Payvice::MCC = "mcc";

struct TamsPayviceResponse {
    std::string message;
    std::string macros_tid;
    std::string status;
    std::string result;
};

static short setupPayviceNetwork(NetWorkParameters * netParam, char *path)
{
    char host[] = "basehuge.itexapp.com";
   
    strncpy((char *)netParam->host, host, strlen(host));
    netParam->receiveTimeout = 60000;
	strncpy(netParam->title, "Payvice", 10);
    netParam->isHttp = 1;
    netParam->isSsl = 0;
    netParam->port = 80;
    netParam->endTag = "";  // I might need to comment it out later

    
    netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "GET %s HTTP/1.1\r\n", path);
 	netParam->packetSize += sprintf((char *)(&netParam->packet[netParam->packetSize]), "Host: %s:%d\r\n\r\n", netParam->host, netParam->port);

}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
int genericSHA256(const char* msg, const char* key, char* hash)
{

    sha256_context Context;
    unsigned char keyBin[128 + 1] = { 0 };
    unsigned char digest[32 + 1] = { 0 };
    int len;
    char* message = strdup(msg);

    if (!message) {
        return -1;
    }

    sha256_starts(&Context);
    memset(keyBin, 0, sizeof(keyBin));

    len = hex2bin(key, (char*)keyBin, strlen(key));
    sha256_update(&Context, keyBin, len);
    sha256_update(&Context, (unsigned char*)message, strlen(message));
    sha256_finish(&Context, digest);

    bin2hex(digest, hash, 32);     

    free(message);

    return 0;
}

void parseTamsPayviceResponse(TamsPayviceResponse& payviceResponse, char* buffer)
{
    ezxml_t tran; 
    ezxml_t root = ezxml_parse_str(buffer, strlen(buffer));

    printf("Raw response is >>>> %s\n", buffer);

    if (!root) {
        printf("\nError, Please Try Again\n");
        return;
    }

    tran = ezxml_child(root, "tran");

    if (ezxml_child(tran, "message") != NULL) {
        payviceResponse.message = ezxml_child(tran, "message")->txt;
    }

    if (ezxml_child(tran, "macros_tid") != NULL) {
        payviceResponse.macros_tid = ezxml_child(tran, "macros_tid")->txt;
    }

    if (ezxml_child(tran, "status") != NULL) {
        payviceResponse.status = ezxml_child(tran, "status")->txt;
    }

    if (ezxml_child(tran, "result") != NULL) {
        payviceResponse.result = ezxml_child(tran, "result")->txt;
    }
    
}

TamsPayviceResponse loginPayVice(/*CURL* curl_handle, */const char* key, Payvice& payvice)
{
    NetWorkParameters netParam = {'\0'};
    char encPass[128] = { 0 };
    char encodedUrl[128];
    std::string urlPath = "/ctms/eftpos/devinterface/transactionadvice.php?action=TAMS_LOGIN&termid=203301ZA";
    int i;
    TamsPayviceResponse payviceResponse;

    safe_url_encode(payvice.object(Payvice::USER).getString().c_str(), encodedUrl, sizeof(encodedUrl), 1);
    urlPath += std::string("&userid=") + std::string(encodedUrl);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // http://basehuge.itexapp.com/ctms/eftpos/devinterface/transactionadvice.php?action=TAMS_LOGIN&termid=203301ZA&userid=userid&password=encryptedPassword
    if (genericSHA256((const char*)payvice.object(Payvice::PASS).getString().c_str(), key, encPass) < 0) {
        return payviceResponse;
    }

    // encrypted pass to lower
    i = strlen(encPass);
    while (i > 0) {
        encPass[i - 1] = tolower(encPass[i - 1]);
        i--;
    }

    urlPath += std::string("&password=") + encPass;

    setupPayviceNetwork(&netParam, (char *)urlPath.c_str());
    printf("request : \n%s\n", netParam.packet);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return payviceResponse;
    }

    parseTamsPayviceResponse(payviceResponse, (char*)netParam.response);

    return payviceResponse;
}

int extractPayViceKey(char* plainKey, const char* encryptedKey, const char* walletId)
{
    int i = 0;
    int len = strlen(walletId);

    if (strlen(encryptedKey) < 10) {
        return -1;
    }

    while (i < len && i < 32) {
        int c = *(walletId + i) - 48; // 48 is ascii zero
        if (c < 0 || c > 9) {
            // default to index 7
            *(plainKey + i) = *(encryptedKey + 7);
        } else {
            *(plainKey + i) = *(encryptedKey + c);
        }
        i++;
    }

    return 0;
}

TamsPayviceResponse getPayviceKeys(/*CURL* curl_handle, */ Payvice& payvice)
{
    NetWorkParameters netParam = {'\0'};
    char  encodedUrl[128];
    TamsPayviceResponse payviceResponse;

    std::string urlPath = "/ctms/eftpos/devinterface/transactionadvice.php?action=TAMS_LOGIN&termid=203301ZA";
    safe_url_encode(payvice.object(Payvice::USER).getString().c_str(), encodedUrl, sizeof(encodedUrl), 1);
    urlPath += std::string("&userid=") + std::string(encodedUrl) + "&control=INIT";

    // http://basehuge.itexapp.com/ctms/eftpos/devinterface/transactionadvice.php?action=TAMS_LOGIN&termid=203301ZA&userid=urlEncoded(username)&control=INIT

    setupPayviceNetwork(&netParam, (char *)urlPath.c_str());
    printf("request : \n%s\n", netParam.packet);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return payviceResponse;
    }

    parseTamsPayviceResponse(payviceResponse, (char*)netParam.response);

    return payviceResponse;
}

int beginLoginSequence(Payvice& payvice)
{
    char key[128] = { 0 };
    TamsPayviceResponse payviceResponse;
    
    // CURL* curl_handle; Handle send and receive

    Demo_SplashScreen("www.payvice.com", "Please wait...");
    payviceResponse = getPayviceKeys(payvice);
    if (payviceResponse.status != "1" || payviceResponse.result != "0") {
        UI_ShowButtonMessage(10000, "Error", payviceResponse.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    } else if (payviceResponse.macros_tid.empty()) {
        return -1;
    } else {
        std::istringstream iStream(payviceResponse.message);
        std::string token;
        std::getline(iStream, token, '|');
        std::getline(iStream, payviceResponse.message, '|');
    }

    int status = extractPayViceKey(key, payviceResponse.message.c_str(), payviceResponse.macros_tid.c_str());
    if (status < 0) {
        return -1;
    }

    payvice.object(Payvice::WALLETID) = payviceResponse.macros_tid;
    payvice.object(Payvice::KEY) = key;

    payviceResponse = loginPayVice(/* curl_handle, */key, payvice);
    if (payviceResponse.result != "0" || payviceResponse.status != "0") {
        UI_ShowButtonMessage(10000, "Error", payviceResponse.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    payvice.save();

    UI_ShowButtonMessage(2000, "Payvice", "Login Successful", "OK", UI_DIALOG_TYPE_CONFIRMATION);

    return 0;
}

int logIn(Payvice& payvice)
{
    std::string username;
    std::string password;

    iisys::JSObject& userObj = payvice.object(Payvice::USER);

    if (!userObj.isNull()) {
        username = userObj.getString();
    }

    if (getText(username, 0, 30000, "Username", "Your email or phone no", UI_DIALOG_TYPE_NONE) < 0) {
        return -1;
    }

    payvice.object(Payvice::USER) = username;

    password = payvice.object(Payvice::PASS).getString();
    if (getPassword(password) < 0) {
        return -1;
    }

    payvice.object(Payvice::PASS) = password;

    return beginLoginSequence(payvice);
}

int logOut(Payvice& payvice)
{
    return payvice.resetFile();
}

bool loggedIn(const Payvice& payvice)
{
    const iisys::JSObject& walletId = payvice.object(Payvice::WALLETID);
    const iisys::JSObject& username = payvice.object(Payvice::USER);
    const iisys::JSObject& password = payvice.object(Payvice::PASS);
    const iisys::JSObject& key = payvice.object(Payvice::KEY);

    if (walletId.isNull() || walletId.getString().empty()
        || username.isNull() || username.getString().empty()
        || password.isNull() || password.getString().empty()
        || key.isNull() || key.getString().empty()) {
        return false;
    }
        
    return true;
}

std::string encryptedPassword(const Payvice& payvice)
{
	char encPass[128] = { 0 };

	genericSHA256(payvice.object(Payvice::PASS).getString().c_str(), payvice.object(Payvice::KEY).getString().c_str(), encPass);

	int i = 0;
	while (encPass[i]) {
		encPass[i] = tolower(encPass[i]);
		i++;
	}

    return std::string(encPass);
}

std::string encryptedPin(const Payvice& payvice, const char* pin)
{
	std::string encPassword = encryptedPassword(payvice);
    char encPin[128] = { 0 };

	genericSHA256(pin, encPassword.c_str(), encPin);

	int i = 0;
	while (encPin[i]) {
		encPin[i] = tolower(encPin[i]);
		i++;
	}

	return std::string(encPin);
}

int injectPayviceCredentials(iisys::JSObject& obj)
{
    Payvice payvice;

    if (!loggedIn(payvice)) {
        return -1;
    }
    
    obj("username") = payvice.object(Payvice::USER);
    obj("password") = payvice.object(Payvice::PASS);
    obj("wallet") = payvice.object(Payvice::WALLETID);
    obj("channel")  = "POS";

    return 0;
}

std::string getClientReference()
{
	char randomString[16] = {0};
	char formattedTimestamp[32] = {0};
	char tempRef[512] = {0};
	char *b64EncodedMsg = 0;
    Payvice payvice;
    std::string ref;
	size_t i = 0;

	generateRandomString(randomString, sizeof(randomString));
	formattedDateTime(formattedTimestamp, sizeof(formattedTimestamp));


	snprintf(tempRef, sizeof(tempRef), "{\"sessionKey\": \"%s\", \"timestamp\": \"%s\",\"randomString\":\"%s\"}"
		, payvice.object(Payvice::SESSION).getString().c_str(), formattedTimestamp, randomString);

	b64EncodedMsg = (char*)base64_encode((unsigned char*)tempRef, strlen(tempRef), &i);

	if (!b64EncodedMsg) {
		return ref;
	}

    ref = std::string(b64EncodedMsg, i);

	free(b64EncodedMsg);

	return ref;
}

int cashIOVasToken(const char *msg, const char *prefix, KeyChain *keys)
{
    char *b64EncodedMsg = 0;
    char urlEncoded[3 * 0x1000] = {0};
    char toHash[4 * 0x1000] = {0}; // May truncate but not likely
    size_t i;
    
    int msgLen = strlen(msg);

    b64EncodedMsg = (char*)base64_encode((const unsigned char*)msg, msgLen, &i);

    safe_url_encode(b64EncodedMsg, urlEncoded, sizeof(urlEncoded), 0);

	if (prefix) {
		int pLen = strlen(prefix);
		strncat(keys->nonce, prefix, sizeof(keys->nonce) - 1);
		generateRandomString(keys->nonce + pLen, sizeof(keys->nonce) - pLen);
	} else {
		generateRandomString(keys->nonce, sizeof(keys->nonce));
	}

    sprintf(toHash, "%s%s%s", keys->nonce, "IL0v3Th1sAp11111111UC4NDoV4SSWITHVICEBANKING", urlEncoded);

    sha512Hex(keys->signature, toHash, strlen(toHash));

    free(b64EncodedMsg);

    return (int)strlen(keys->signature);
}

