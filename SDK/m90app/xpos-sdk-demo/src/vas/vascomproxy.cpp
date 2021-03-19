#include <string.h>
#include <openssl/hmac.h>
#include <openssl/x509.h>

#include <algorithm>

#include "vascomproxy.h"
#include "payvice.h"
#include "vasbridge.h"
#include "virtualtid.h"
#include "./platform/platform.h"
#include "vasdb.h"

#include  "../pfm.h"
#include "../auxPayload.h"
#include "../EmvDB.h"
#include "../EmvDBUtil.h"
#include "../Receipt.h"
#include "../EftDbImpl.h"
#include "../merchant.h"

#include "jsonwrapper/jsobject.h"

extern "C" {
#include "../EmvEft.h"
#include "../Nibss8583.h"
#include "../network.h"
#include "../util.h"
}

static const char* TOKEN_UNAVAILABLE_STR = "Token Unavailable";

std::string vasApiKey();

bool isValidVasTransactionType(const CardData& cardData);
int vasPayloadGenerator(void* jsobject, void* data, const void *eft);

Postman::Postman(const std::string& hostaddr, const std::string& port)
    : txnHost(hostaddr)
    , portValue(port)
{
}

Postman::~Postman()
{
}

VasResult Postman::lookup(const char* url, const iisys::JSObject* json)
{
    return doRequest(url, json, NULL);
}

VasResult Postman::initiate(const char* url, const iisys::JSObject* json, CardData* card)
{
    return doRequest(url, json, card);
}

VasResult Postman::complete(const char* url, const iisys::JSObject* json, CardData* card)
{
    return doRequest(url, json, card);
}

VasResult Postman::reverse(CardData& cardData)
{
    VasResult result;
    NetWorkParameters netParam = {'\0'};

    EmvDB db(cardData.trxContext.tableName, cardData.trxContext.dbName);
    
    memset(&netParam, 0x00, sizeof(NetWorkParameters));
    getNetParams(&netParam, CURRENT_PLATFORM, 0);

    if (autoReversalInPlace(&cardData.trxContext, &netParam)) {
        db.updateTransaction(cardData.primaryIndex, ctxToUpdateMap(&cardData.trxContext));
        return result;
    }

    
    db.updateTransaction(cardData.primaryIndex, ctxToUpdateMap(&cardData.trxContext));

    result = NO_ERRORS;
    return result;
}

VasResult Postman::doRequest(const char* url, const iisys::JSObject* json, CardData* card)
{
    VasResult result;
    
    if (card) {
        result = sendVasCardRequest(url, json, card);
    } else {
        result = sendVasCashRequest(url, json);
    }

    return result;
}

std::string Postman::generateRequestAuthorization(const std::string& requestBody)
{
    char signature[SHA256_DIGEST_LENGTH] = { 0 };
    char signaturehex[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };
    size_t i;

    {
        std::string apiKey = vasApiKey();
        vasimpl::hmac_sha256((const unsigned char*)requestBody.c_str(), requestBody.length(), (const unsigned char*)apiKey.c_str(), (int)apiKey.length(), signature);
    }

    vasimpl::bin2hex((unsigned char*)signature, signaturehex, sizeof(signature));
    i = strlen(signaturehex);
    while (i--) {
        signaturehex[i] = tolower(signaturehex[i]);
    }

    return std::string(signaturehex);
}

VasResult Postman::sendVasCashRequest(const char* url, const iisys::JSObject* json)
{
    VasResult result(CASH_STATUS_UNKNOWN);
    std::string body;
    Payvice payvice;
    
    NetWorkParameters netParam = {'\0'};

   
    strncpy((char *)netParam.host, txnHost.c_str(), sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 120000;
	strncpy(netParam.title, "vas", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = atol(portValue.c_str());   // staging 8028, live 80, test 8018
    netParam.endTag = "";  // I might need to comment it out later

    const std::string apiToken = payvice.getApiToken();
    if (apiToken.empty()) {
        result.error = TOKEN_UNAVAILABLE;
        result.message = TOKEN_UNAVAILABLE_STR;
        return result;
    }

    if (json) {
        body = json->dump();
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST %s HTTP/1.1\r\n", url);
    } else {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "GET %s\r\n", url);
    }

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "token: %s\r\n", apiToken.c_str());
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "signature: %s\r\n", generateRequestAuthorization(body).c_str());
    

    if (json) {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", body.length());
    }
    

    if (body.length()) {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", body.c_str());
    } 

    enum CommsStatus commStatus = sendAndRecvPacket(&netParam);
    if(commStatus == SEND_RECEIVE_SUCCESSFUL){
        result.error = NO_ERRORS;
        result.message = strchr((const char*)netParam.response, '{');
        return result;

    } else if (!json || (*json)("clientReference").isNull()) {

        if (commStatus == CONNECTION_FAILED) {
            result.message = "Connection Error";
        } else if (commStatus == SENDING_FAILED) {
            result.message = "Send Error";
        } else if (commStatus == RECEIVING_FAILED) {
            result.message = "Receive error";
        } else {
            result.message = "Unknown Network Error";
        }

        result.error = CASH_STATUS_UNKNOWN;
        return result;
    }

    // Attempt to requery automatically
    const iisys::JSObject& clientRef = (*json)("clientReference");
    const iisys::JSObject& walletId = payvice.object(Payvice::WALLETID);
    VasResult requeryStatus =  requeryVas(clientRef.getString().c_str(), walletId.getString().c_str(), payvice.getApiToken().c_str());

    
    if (requeryStatus.error == NO_ERRORS) {
        result.error = NO_ERRORS;
        result.message = requeryStatus.message;
        return result;
    } else {
        result.error = CASH_STATUS_UNKNOWN;
        result.message = requeryStatus.message;
        return result;
    }


    return result;
}

VasResult
Postman::sendVasCardRequest(const char* url, const iisys::JSObject* json, CardData* cardData)
{
    VasResult result(CARD_STATUS_UNKNOWN);
    iisys::JSObject jsonReq;
    std::string body;
    Payvice payvice;

    if (cardData->reference.empty()) {
        result.error = VAS_ERROR;
        return result;
    }

    const std::string apiToken = payvice.getApiToken();
    if (apiToken.empty()) {
        result.error = TOKEN_UNAVAILABLE;
        result.message = TOKEN_UNAVAILABLE_STR;
        return result;
    }


    memset((void*)&cardData->trxContext, 0, sizeof(Eft));
    cardData->trxContext.vas.switchMerchant = itexIsMerchant() ? 0 : 1;
   
    strcpy(cardData->trxContext.dbName, VASDB_FILE);
    strcpy(cardData->trxContext.tableName, VASDB_CARD_TABLE);
    

    if (json) {
        body = json->dump();
        jsonReq("body") = *json;
        jsonReq("method") = "POST";
        jsonReq("headers")("Content-Type") = "application/json";
    } else {
        jsonReq("method") = "GET";
    }
    
    // jsonReq("host") = std::string("http://") +  vas + ":8028" + url; // staging
    jsonReq("host") = std::string("http://") +  txnHost + ":" + portValue + url;


    jsonReq("headers")("token") = apiToken;
    jsonReq("headers")("signature") = generateRequestAuthorization(body);

    jsonReq("terminalId") = vasimpl::getDeviceTerminalId();

    if (addPayloadGenerator(&cardData->trxContext, vasPayloadGenerator, (void*)&jsonReq) != 0) {
        return result;
    }
    
    strncpy(cardData->trxContext.echoData, cardData->refcode.c_str(), sizeof(cardData->trxContext.echoData) - 1);
    strncpy(cardData->trxContext.rrn, cardData->reference.c_str(), strlen(cardData->reference.c_str()));
    strcpy(cardData->trxContext.paymentInformation, cardData->upWithdrawal.field60.c_str()); // TODO enable for upsl withdrawal

    if (doVasCardTransaction(&cardData->trxContext, cardData->amount) < 0) {
        return result;  // We weren't able to initiate transaction
    }

    cardData->primaryIndex = cardData->trxContext.atPrimaryIndex;
    cardData->transactionTid = cardData->trxContext.returnTid;

    if (cardData->trxContext.vas.auxResponse[0]) {
        // Extra check to assert that card tranaction was successful?
        if (!strcmp(cardData->trxContext.responseCode, "00") && isValidVasTransactionType(*cardData)) {
            result.error = NO_ERRORS;
        }
        result.message = cardData->trxContext.vas.auxResponse;
        return result;
    }
    
    if (!cardData->trxContext.responseCode[0] && requeryMiddleware(&cardData->trxContext, cardData->transactionTid.c_str()) < 0) {
        result.error = CARD_STATUS_UNKNOWN;
        return result;
    }

    if (strcmp(cardData->trxContext.responseCode, "00") != 0) {
        result.error = CARD_PAYMENT_DECLINED;
        result.message = responseCodeToStr(cardData->trxContext.responseCode);
    } else if (isValidVasTransactionType(*cardData)) {
        // So card approved and we didn't get vas response

        const iisys::JSObject& clientRef = (*json)("clientReference");
        const iisys::JSObject& walletId = payvice.object(Payvice::WALLETID);

        // So card approved and we didn't get vas response
        result.error = CARD_APPROVED;
        result.message = "";

        if (clientRef.isNull() || clientRef.getString().empty()) {
            return result;
        }

        VasResult requeryResult =  requeryVas(clientRef.getString().c_str(), walletId.getString().c_str(), payvice.getApiToken().c_str());

        if (requeryResult.error == NO_ERRORS) {
            iisys::JSObject requeryJson;

            if (!requeryJson.load(requeryResult.message)) {
                return result;
            } else if (vasResponseCheck(requeryJson).error == TXN_NOT_FOUND) {
                return result;
            }

            result.error = NO_ERRORS;
            result.message = requeryResult.message;
        }

    }
    
    return result;
}

VasResult Postman::requeryVas(const char* clientRef, const char* walletId, const char* token) const
{
    VasResult result;
    NetWorkParameters netParam = {'\0'};
    iisys::JSObject json;
    
    strncpy((char *)netParam.host, REQUERY_IP, sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Requery", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = REQUERY_PORT;
    netParam.endTag = "";
    
    
    char path[] = "/api/v1/vas/storage/fetch/transaction";

    json("wallet") = walletId;
    json("clientReference") = clientRef;
    json("channel") = vasChannel();
    json("version") = vasApplicationVersion();


    const std::string body = json.getString();

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST %s HTTP/1.1\r\n", path);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "token: %s\r\n", token);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "signature: %s\r\n", generateRequestAuthorization(body).c_str());
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", body.length());

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", body.c_str());

    enum CommsStatus commStatus = sendAndRecvPacket(&netParam);
    if(commStatus == SEND_RECEIVE_SUCCESSFUL){
        result.error = NO_ERRORS;
        result.message = strchr((const char*)netParam.response, '{');

    } else {

        if (commStatus == CONNECTION_FAILED) {
            result.message = "Connection Error";
        } else if (commStatus == SENDING_FAILED) {
            result.message = "Send Error";
        } else if (commStatus == RECEIVING_FAILED) {
            result.message = "Receive error";
        } else {
            result.message = "Unknown Network Error";
        }

    }

    return result;
}

VasResult Postman::sendVasRequest(const char* url, const iisys::JSObject* json)
{
    VasResult result(CASH_STATUS_UNKNOWN);
    NetWorkParameters netParam = {'\0'};
    Payvice payvice;
    std::string body;

    if (!loggedIn(payvice)) {
        return VasResult(VAS_ERROR);
    }

    strncpy((char *)netParam.host, txnHost.c_str(), sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Payvice", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = atol(portValue.c_str());
    netParam.endTag = ""; 

    const std::string apiToken = payvice.getApiToken();
    if (apiToken.empty()) {
        result.error = TOKEN_UNAVAILABLE;
        result.message = TOKEN_UNAVAILABLE_STR;
        return result;
    }

    if (json) {
        body = json->dump();
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST %s HTTP/1.1\r\n", url);
    } else {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "GET %s HTTP/1.1\r\n", url);
    }

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "token: %s\r\n", apiToken.c_str());
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "signature: %s\r\n", generateRequestAuthorization(body).c_str());

    if (json) {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", body.length());
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", body.c_str());
    } else {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n");
    }


    enum CommsStatus commStatus = sendAndRecvPacket(&netParam);
    if(commStatus != SEND_RECEIVE_SUCCESSFUL){

        if (commStatus == CONNECTION_FAILED) {
            result.message = "Connection Error";
        } else if (commStatus == SENDING_FAILED) {
            result.message = "Send Error";
        } else if (commStatus == RECEIVING_FAILED) {
            result.message = "Receive error";
        } else {
            result.message = "Unknown Network Error";
        }
        result.error = VAS_ERROR;

        return result;
    } 

    result.error = NO_ERRORS;
    result.message = strchr((const char*)netParam.response, '{');   

    return result;
}


VasError Postman::manualRequery(iisys::JSObject& transaction, const std::string& sequence, const std::string& cardRef)
{
    Payvice payvice;
    NetWorkParameters netParam = {'\0'};
    iisys::JSObject json;
    
    strncpy((char *)netParam.host, REQUERY_IP, sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Requery", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = REQUERY_PORT;
    netParam.endTag = "";
    
    char path[] = "/api/v1/vas/storage/fetch/transaction/custom";
    
    const iisys::JSObject& walletId = payvice.object(Payvice::WALLETID);
    json("wallet") = walletId;
    if (!sequence.empty()) {
        json("sequence") = sequence;
    } else if (!cardRef.empty()) {
        std::string ref = cardRef;
        std::string clientReference = getClientReference(ref);
        json("clientReference") = clientReference;
    }

    const std::string body = json.getString();
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST %s HTTP/1.1\r\n", path);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    // netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "token: %s\r\n", payvice.getApiToken().c_str());
    // netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "signature: %s\r\n", generateRequestAuthorization(body).c_str());
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", body.length());

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", body.c_str());

    enum CommsStatus commStatus = sendAndRecvPacket(&netParam);
    if(commStatus != SEND_RECEIVE_SUCCESSFUL){
        return VAS_ERROR;
    }    

    if (!json.load(strchr((const char*)netParam.response, '{'))) {
    
        return INVALID_JSON;
    }
    
    const VasResult result = vasResponseCheck(json);
    if (result.error != NO_ERRORS) {
        return result.error;
    }

    transaction = json("data");
    revalidateSmartCard(transaction);

    return NO_ERRORS;
}

bool isValidVasTransactionType(const CardData& cardData)
{

    if (cardData.trxContext.transType == EFT_PURCHASE || cardData.trxContext.transType == EFT_CASHADVANCE) {
        return true;
    }

    return false;
}


int vasPayloadGenerator(void* jsobject, void* data, const void *eft)
{
    //TODO later niyen
    // if (isReversal(context) || isManualReversal()) {
    //     return 0;
    // }

    Eft *context = (Eft*)eft;

    iisys::JSObject* json = static_cast<iisys::JSObject*>(jsobject);
    iisys::JSObject* jsonReq = static_cast<iisys::JSObject*>(data);

    iisys::JSObject& card = (*jsonReq)("card");

    card = getJournal(context);

    card("linuxTerminalGps") = getCellId();
    if (context->vas.switchMerchant) {
        card("vTid") = Payvice().object(Payvice::VIRTUAL)(Payvice::TID).getString();
    } else {
        card("vTid") = "";
    }

    (*json)("vas4Data") =  *jsonReq;

    return 0;

}

std::string vasApiKey()
{
    // char key[] = "a6Q6aoHHESonso27xAkzoBQdYFtr9cKC"; //test
    char key[] = "o83prs088n4943231342p7sq53o6502q";    //live

    rot13(key);
    return std::string(key);
}
