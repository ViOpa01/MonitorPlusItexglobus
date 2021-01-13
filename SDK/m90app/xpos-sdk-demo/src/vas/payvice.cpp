#include "payvice.h"

#include <sstream>
#include <algorithm>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


#include "platform/platform.h"
#include "./platform/simpio.h"

#include "expatvisitor.h"
#include "vasbridge.h"
#include "vascomproxy.h"

extern "C" {
#include "../util.h"
#include "../ezxml.h"
}

#ifdef _VRXEVO
static char* strdup(const char* s)
{
    char* p = (char*)malloc(strlen(s) + 1);
    if (p)
        strcpy(p, s);
    return p;
}
#endif

const char* Payvice::USER = "username";
const char* Payvice::PASS = "passsword";
const char* Payvice::WALLETID = "walletId";
const char* Payvice::KEY = "key";
const char* Payvice::TOKEN = "token";
const char* Payvice::TOKEN_EXP = "tokenExp";

const char* Payvice::VIRTUAL = "virtual";
const char* Payvice::TID = "tid";
const char* Payvice::MID = "mid";
const char* Payvice::NAME = "merchantName";
const char* Payvice::SKEY = "sessionKey";
const char* Payvice::PKEY = "pinKey";
const char* Payvice::CUR_CODE = "currencyCode";
const char* Payvice::COUNTRYCODE = "countryCode";
const char* Payvice::MCC = "mcc";

const char* Payvice::IS_AGENT = "isAgent";

struct TamsPayviceResponse {
    std::string message;
    std::string macros_tid;
    std::string status;
    std::string result;
    std::string agentType;
};

struct TamsResponseParser : ExpatVisitor {

    explicit TamsResponseParser(TamsPayviceResponse& response)
        : response(response)
    {
    }

    void setTag(const char* tag)
    {
        (void) tag;
        val.clear();
    }

    void endTag(const char* tag)
    {
        if (strcmp(tag, "message") == 0) {
            response.message = val;
        } else if (strcmp(tag, "status") == 0) {
            response.status = val;
        } else if (strcmp(tag, "result") == 0) {
            response.result = val;
        } else if (strcmp(tag, "macros_tid") == 0) {
            response.macros_tid = val;
        } else if (strcmp(tag, "AgentType") == 0) {
            response.agentType = val;
        }
    }

    void visit(const char* start, int len)
    {
        val += std::string(start, len);
    }

private:
    TamsPayviceResponse& response;
    std::string val;
};

Payvice::Payvice()
    : _filename(PAYVICE_CONF)
{
    std::ifstream file;
    std::stringstream stream;
    bool fileExists = isFileExists(_filename.c_str());

    if (!fileExists) {
        resetFile();
    } else {
        file.open(_filename.c_str());
    }

    if (!file.is_open()) {
        err = SOME_ERROR_OCCURED;
        return;
    }

    stream << file.rdbuf();
    if (!object.load(stream.str().c_str())) {
        err = SOME_ERROR_OCCURED;
        return;
    }
    file.close();
}

int Payvice::resetFile()
{
    object(WALLETID) = "";
    object(USER) = "";
    object(PASS) = "";
    object(KEY) = "";
    object(TOKEN) = "";
    object(TOKEN_EXP) = 0;
    object(IS_AGENT) = false;

    return save();
}

int Payvice::resetApiToken()
{
    Payvice payvice;

    payvice.object(TOKEN) = "";
    payvice.object(TOKEN_EXP) = 0;

    return payvice.save();
}

std::string Payvice::getApiToken()
{
    std::string apiToken;

    if (object(TOKEN).getString().empty() || object(TOKEN_EXP).isNull() || time(NULL) >= (time_t)object(TOKEN_EXP).getInt()) {
        time_t now = time(NULL) - (time_t) 60 * 10;
        std::string tokenResponse = fetchVasToken();
        if (tokenResponse.empty()) {
            return apiToken;
        }

        if (extractVasToken(tokenResponse, now) != 0) {
            return apiToken;
        }
        save();
    }
    apiToken = object(TOKEN).getString();
    return apiToken;
}

int Payvice::error() const { return err ? 1 : 0; }

bool Payvice::isFileExists(const char* filename)
{
    struct stat st;
    int result = stat(filename, &st); // this approach doesn't work if we don't have read access right to the DIRECTORY CONTAINING THE FILE
    return result == 0;
}

int Payvice::save()
{
    FILE* config;

    if ((config = fopen(_filename.c_str(), "w+")) == NULL) {
        err = SAVE_ERR;
        return -1;
    }
    fprintf(config, "%s", object.dump().c_str());
    fclose(config);
    err = NOERROR;
    return 0;
}

std::string Payvice::fetchVasToken()
{
    MerchantData mParam;
    NetWorkParameters netParam;
    std::string response;

    memset(&netParam, 0x00, sizeof(NetWorkParameters));
    memset(&mParam, 0x00, sizeof(MerchantData));

    readMerchantData(&mParam);


    strncpy((char *)netParam.host, VAS_IP, sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Request", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = atoi(VAS_PORT);
    netParam.endTag = "";
    std::string path = "/api/vas/authenticate/me";
    char requestBody[1024] = {'\0'};

    snprintf(requestBody, sizeof(requestBody), "{\"wallet\": \"%s\", \"username\": \"%s\", \"password\": \"%s\", \"identifier\": \"itexlive\", \"terminal\": \"%s\"}", 
        object(Payvice::WALLETID).getString().c_str(), object(Payvice::USER).getString().c_str(), object(Payvice::PASS).getString().c_str(), mParam.tid);

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST %s HTTP/1.1\r\n", path.c_str());
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s\r\n", netParam.host);

    if (requestBody) {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", strlen(requestBody));

        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", requestBody);

    }

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return response;
    }

    response = bodyPointer((const char*)netParam.response);

    return response;
}

int Payvice::extractVasToken(const std::string& response, const time_t now)
{
    iisys::JSObject json;

    if (!json.load(response)) {
        return -1;
    }

    const iisys::JSObject& responseCode = json("responseCode");
    if (!responseCode.isString()) {
        return -1;
    } else if (responseCode.getString() != "00") {
        return -1;
    }

    const iisys::JSObject& data = json("data");
    const iisys::JSObject& token = data("apiToken");
    const iisys::JSObject& expiration = data("expiration");

    if (!token.isString() || !expiration.isString()) {
        return -1;
    }

    object(Payvice::TOKEN) = token;
    object(Payvice::TOKEN_EXP) = now + atoi(expiration.getString().c_str()) * 60 * 60;
    printf("============TOKEN EXPIRED TIME : %d(s), NOW : %d ============\n",  object(Payvice::TOKEN_EXP).getInt(), now);

    return 0;
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

    len = vasimpl::hex2bin(key, (char*)keyBin, strlen(key));
    sha256_update(&Context, keyBin, len);
    sha256_update(&Context, (unsigned char*)message, strlen(message));
    sha256_finish(&Context, digest);

    vasimpl::bin2hex(digest, hash, 32);

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

TamsPayviceResponse loginPayVice(void* userData, const char* key, Payvice& payvice)
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

    setupBaseHugeNetwork(&netParam, urlPath.c_str());
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

TamsPayviceResponse getPayviceKeys(void* userData, Payvice& payvice)
{
    NetWorkParameters netParam = {'\0'};
    char  encodedUrl[128];
    TamsPayviceResponse payviceResponse;

    std::string urlPath = "/ctms/eftpos/devinterface/transactionadvice.php?action=TAMS_LOGIN&termid=203301ZA";
    safe_url_encode(payvice.object(Payvice::USER).getString().c_str(), encodedUrl, sizeof(encodedUrl), 1);
    urlPath += std::string("&userid=") + std::string(encodedUrl) + "&control=INIT";

    // http://basehuge.itexapp.com/ctms/eftpos/devinterface/transactionadvice.php?action=TAMS_LOGIN&termid=203301ZA&userid=urlEncoded(username)&control=INIT

    setupBaseHugeNetwork(&netParam, urlPath.c_str());
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

    Demo_SplashScreen("www.payvice.com", "Please wait...");
    payviceResponse = getPayviceKeys(NULL, payvice);
    if (payviceResponse.status != "1" || payviceResponse.result != "0") {
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

    payviceResponse = loginPayVice(NULL, key, payvice);
    if (payviceResponse.result != "0" || payviceResponse.status != "0") {
        UI_ShowButtonMessage(2000, "Error", payviceResponse.message.c_str(), "OK", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    std::transform(payviceResponse.agentType.begin(), payviceResponse.agentType.end(), payviceResponse.agentType.begin(), ::tolower);
    if (payviceResponse.agentType == "staff" || payviceResponse.agentType == "agent") {
        payvice.object(Payvice::IS_AGENT) = true;
    } else {
        payvice.object(Payvice::IS_AGENT) = false;
    }

    payvice.save();

    UI_ShowButtonMessage(2000, "Payvice", "Login Successful", "OK", UI_DIALOG_TYPE_CONFIRMATION);

    return 0;
}

int isPayvicePasswordValid(const char* password)
{
	size_t i;
	size_t state = 0;
	const size_t UPPER_CASE_PRESENT		= 1 << 0;
	const size_t ALPHABET_PRESENT   	= 1 << 1;
	const size_t NUMBER_PRESENT		 	= 1 << 2;

	if (!password) {
		return 0;
	}

	i = 0;
	while (password[i]) {
		if (isupper(password[i])) {
			state |= UPPER_CASE_PRESENT;
		}

		if (isalpha(password[i])) {
			state |= ALPHABET_PRESENT;
		}

		if (isdigit(password[i])) {
			state |= NUMBER_PRESENT;
		}

		++i;
	}

	if (i >= 8 && state == (UPPER_CASE_PRESENT | ALPHABET_PRESENT | NUMBER_PRESENT)) {
		return 1;
	}

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

    if (getPassword(password) < 0) {
        return -1;
    } else if (isPayvicePasswordValid(password.c_str()) == 0) {
        UI_ShowButtonMessage(60000,  "Error", "Password must contain an uppercase, lowercase, a number and should be at least 8 characters long", "OK", UI_DIALOG_TYPE_CAUTION);
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

    if (!walletId.isString() || walletId.getString().empty()
        || !username.isString() || username.getString().empty()
        || !password.isString() || password.getString().empty()
        || !key.isString() || key.getString().empty()) {
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

std::string getClientReference(std::string& customReference)
{
    const char* modelTag = "08";
    const char* serialTag = "09";
    const char* referenceTag = "11";
    const char* terminalIdTag = "12";

    const std::string reference     = customReference.empty() ? vasimpl::cardReference() : customReference;
    const std::string deviceModel   = vasimpl::getDeviceModel();
    const std::string terminalId    = vasimpl::getDeviceTerminalId();
    const std::string deviceSerial  = vasimpl::getDeviceSerial();

    size_t i = 0;
    char clientRef[1024] = { 0 };

    i += sprintf(&clientRef[i], "%s%02d%s", modelTag, (int)deviceModel.length(), deviceModel.c_str());
    i += sprintf(&clientRef[i], "%s%02d%s", serialTag, (int)deviceSerial.length(), deviceSerial.c_str());
    i += sprintf(&clientRef[i], "%s%02d%s", terminalIdTag, (int)terminalId.length(), terminalId.c_str());
    i += sprintf(&clientRef[i], "%s%02d%s", referenceTag, (int)reference.length(), reference.c_str());

    if (customReference.empty()) {
        customReference = reference;
    }

    return std::string(clientRef);
}

std::string getClientReference()
{
    std::string reference;
    return getClientReference(reference);
}
