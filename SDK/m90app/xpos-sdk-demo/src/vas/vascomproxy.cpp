#include <string.h>
#include <stdio.h>

#include "vascomproxy.h"
#include  "pfm.h"
#include "payvice.h"
#include "vasbridge.h"
#include "merchant.h"
#include "util.h"
#include "Receipt.h"
#include "virtualtid.h"

#include "vasdb.h"

extern "C" {
// #include "EmvEft.h"
// #include "Nibss8583.h"
#include "network.h"
}


extern "C" int bin2hex(unsigned char *pcInBuffer, char *pcOutBuffer, int iLen);
extern void getFormattedDateTime(char* dateTime, size_t len);

// extern void prepareTransactionAmmounts(unsigned long long inAmount,
//     unsigned long long inCashbackAmount,
//     unsigned char* outAmountBcd,
//     unsigned char* outCashbackAmountBcd);

const char* vasOrganizationName();
const char* vasOrganizationCode();
std::string vasApiKey();

int vasPayloadGenerator(char* auxPayload, const size_t auxPayloadSize, const Eft* context);

Postman::Postman() : vas("vas.itexapp.com")
{

}

Postman::~Postman()
{
}

VasStatus Postman::lookup(const char* url, const iisys::JSObject* json, const std::map<std::string, std::string>* headers)
{
    return doRequest(url, json, headers, NULL);
}

VasStatus Postman::initiate(const char* url, const iisys::JSObject* json, const std::map<std::string, std::string>* headers, CardPurchase* card)
{
    return doRequest(url, json, headers, card);
}

VasStatus Postman::complete(const char* url, const iisys::JSObject* json, const std::map<std::string, std::string>* headers, CardPurchase* card)
{
    return doRequest(url, json, headers, card);
}

VasStatus Postman::reverse(CardPurchase& cardPurchase)
{
    VasStatus status;
    NetWorkParameters netParam = {'\0'};
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    getNetParams(&netParam, CURRENT_PLATFORM, 0);

    if (autoReversalInPlace(&cardPurchase.trxContext, &netParam)) {
            return status;
    }

   status.error = NO_ERRORS;
   return status;
}


VasStatus Postman::doRequest(const char* url, const iisys::JSObject* json, const std::map<std::string, std::string>* headers, CardPurchase* card)
{
    VasStatus status;
    
    if (card) {
        status = sendVasCardRequest(url, json, headers, card);
    } else {
        status = sendVasRequest(url, json, headers);
    }

    return status;
}

std::string Postman::generateRequestAuthorization(const std::string& requestBody, const std::string& date)
{
    size_t i = 0;
    char digestHex[2 * SHA512_DIGEST_LENGTH + 1] = { 0 };
    
    char signature[SHA256_DIGEST_LENGTH] = { 0 };
    char signaturehex[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };
    char* token = NULL;
    
    sha512Hex(digestHex, requestBody.c_str(), requestBody.length());
    i = strlen(digestHex);
    while (i--) { digestHex[i] = tolower(digestHex[i]); }


    printf("Body(%zu): %s\n", requestBody.length(), requestBody.c_str());
    printf("SHA 512 of body: %s\n", digestHex);

    {
        std::string key = vasApiKey() + "itex";
        token = (char*)base64_encode((const unsigned char*)key.c_str(), key.length(), &i);
    }

    printf("Token: %s\n", token);

    if (!token) {
        return std::string();
    }

    hmac_sha256((const unsigned char*)digestHex, strlen(digestHex), (const unsigned char*)token, (int) strlen(token), signature);

    free(token);

    bin2hex((unsigned char*)signature, signaturehex, sizeof(signature));
    i = strlen(signaturehex);
    while (i--) { signaturehex[i] = tolower(signaturehex[i]); }

    printf("HMAC: %s\n", signaturehex);
    
    std::string full = std::string(signaturehex) + date + vasOrganizationCode();
    char* fullStr = (char*)base64_encode((const unsigned char*)full.c_str(), full.length(), &i);
    if (!fullStr) {
        return std::string();
    }
    full = std::string(fullStr);
    free(fullStr);

    return std::string(vasOrganizationName()) + '-' + full;
}

VasStatus Postman::sendVasRequest(const char* url, const iisys::JSObject* json, const std::map<std::string, std::string>* headers)
{
    VasStatus status;
    std::string body;

    NetWorkParameters netParam = {'\0'};

   
    strncpy((char *)netParam.host, vas.c_str(), sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 120000;
	strncpy(netParam.title, "vas", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = 80;
    netParam.endTag = "";  // I might need to comment it out later


    if (json) {
        body = json->dump();
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST %s HTTP/1.1\r\n", url);
    } else {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "GET %s\r\n", url);
    }

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);

    if (!headers && json) {
        char timestamp[32] = { 0 };

        getFormattedDateTime(timestamp, sizeof(timestamp));
        timestamp[16] = '\0';

        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Authorization: %s\r\n", generateRequestAuthorization(body, timestamp).c_str());
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Username: itex\r\n");
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Vend-Type: %s\r\n", vasOrganizationName());
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Date: %s\r\n", timestamp);
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "OrganisationCode: %s\r\n", vasOrganizationCode());
    } else if (headers) {
        std::map<std::string, std::string>::const_iterator item = headers->begin();
        while (item != headers->end()) {
            netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "%s: %s\r\n", item->first.c_str(), item->second.c_str());
            ++item;
        }
    }

    if (json) {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", body.length());
    }
    

    if (body.length()) {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", body.c_str());
    } 


    if(sendAndRecvPacket(&netParam) == SEND_RECEIVE_SUCCESSFUL){
        status.error = NO_ERRORS;
        status.message = bodyPointer((const char*)netParam.response);
        return status;
    } else if (!json) {
        return status;
    }

    // Attempt to requery automatically
    iisys::JSObject transaction;

    VasError requeryErr =  requeryVas(transaction, (*json)("clientReference").getString().c_str(), NULL);
    if (requeryErr == TXN_NOT_FOUND || requeryErr == TXN_PENDING) {
        status.message = std::string();     // If there is a http library provived error message
        status.error = requeryErr;
        return status;
    } else if (requeryErr != NO_ERRORS || transaction.isNull()) {
        status.message = std::string();     // If there is a http library provived error message
        return status;
    }

    // enum('status', ['initialized', 'debited', 'pending', 'successful', 'failed', 'declined'])->default('initialized');
    std::string statusMsg = transaction("status").getString();
    if (statusMsg == "successful" || statusMsg == "failed" || statusMsg == "declined") {
        status.error = NO_ERRORS;
        status.message = transaction("response").getString();
        return status;
    } else if (statusMsg == "initialized" || statusMsg == "pending" || statusMsg == "debited") {
        status.error = TXN_PENDING;
        return status;
    }


    return status;
}

VasStatus
Postman::sendVasCardRequest(const char* url, const iisys::JSObject* json, const std::map<std::string, std::string>* headers, CardPurchase* cardPurchase)
{
    VasStatus status;
    
    iisys::JSObject jsonReq;

    std::string body;

    memset((void*)&cardPurchase->trxContext, 0, sizeof(Eft));
    cardPurchase->trxContext.switchMerchant = itexIsMerchant() ? 0 : 1;
   
    strcpy(cardPurchase->trxContext.dbName, VASDB_FILE);
    strcpy(cardPurchase->trxContext.tableName, VASCARDTABLENAME);
    

    if (json) {
        body = json->dump();
        jsonReq("body") = *json;
        jsonReq("method") = "POST";
        jsonReq("headers")("Content-Type") = "application/json";
    } else {
        jsonReq("method") = "GET";
    }
    
    jsonReq("host") = std::string("http://") +  vas + url;

    if (!headers && json) {
        char timestamp[32] = { 0 };

        getFormattedDateTime(timestamp, sizeof(timestamp));
        timestamp[16] = '\0';

        jsonReq("headers")("Authorization") = generateRequestAuthorization(body, timestamp);
        jsonReq("headers")("Username") = "itex";
        jsonReq("headers")("Vend-Type") = vasOrganizationName();
        jsonReq("headers")("Date") = timestamp;
        jsonReq("headers")("OrganisationCode") = vasOrganizationCode();

    } else if (headers) {
        std::map<std::string, std::string>::const_iterator item = headers->begin();
        while (item != headers->end()) {
            jsonReq("headers")(item->first) = item->second;
            ++item;
        }
    }

    jsonReq("terminalId") = getDeviceTerminalId();

    cardPurchase->trxContext.genAuxPayload = vasPayloadGenerator;
    cardPurchase->trxContext.callbackdata = (void*)&jsonReq;
    
    strncpy(cardPurchase->trxContext.echoData, cardPurchase->refcode.c_str(), sizeof(cardPurchase->trxContext.echoData) - 1);

    if (doVasCardTransaction(&cardPurchase->trxContext, cardPurchase->amount) < 0) {
        return status;  // We weren't able to initiate transaction
    }

    cardPurchase->primaryIndex = cardPurchase->trxContext.atPrimaryIndex;
    if (cardPurchase->trxContext.switchMerchant) {
        cardPurchase->purchaseTid = Payvice().object(Payvice::VIRTUAL)(Payvice::TID).getString();
    } else {
        cardPurchase->purchaseTid = getDeviceTerminalId();
    }

    if (cardPurchase->trxContext.auxResponse[0]) {
        // Extra check to assert that card tranaction was successful?
        if (!strcmp(cardPurchase->trxContext.responseCode, "00") && cardPurchase->trxContext.transType == EFT_PURCHASE) {
            status.error = NO_ERRORS;
        }
        status.message = cardPurchase->trxContext.auxResponse;
        return status;
    }
    
    if (!cardPurchase->trxContext.responseCode[0]) {

        if (requeryMiddleware(&cardPurchase->trxContext, cardPurchase->purchaseTid.c_str()) < 0) {
            status.error = CARD_STATUS_UNKNOWN;
            return status;
        }
        
    }
    
    if (strcmp(cardPurchase->trxContext.responseCode, "00")) {
        status.error = CARD_PAYMENT_DECLINED;
        status.message = responseCodeToStr(cardPurchase->trxContext.responseCode);
    } else if (cardPurchase->trxContext.transType == EFT_PURCHASE) {
        // So card approved and we didn't get vas response
        iisys::JSObject transaction;

        VasError requeryErr =  requeryVas(transaction, (*json)("clientReference").getString().c_str(), NULL);
        // Could it be that the middleware is still processing the vas leg?
        // For card approved transactions, could there be an endpoint for proactively notifying customer care?
        if (requeryErr == TXN_NOT_FOUND || requeryErr == TXN_PENDING) {
            status.error = CARD_APPROVED;
            return status;
        } else if (requeryErr != NO_ERRORS || transaction.isNull()) {
            status.error = CARD_APPROVED;
            return status;
        }

        // enum('status', ['initialized', 'debited', 'pending', 'successful', 'failed', 'declined'])->default('initialized');
        std::string statusMsg = transaction("status").getString();
        if (statusMsg == "successful" || statusMsg == "failed" || statusMsg == "declined") {
            status.error = NO_ERRORS;
            status.message = transaction("response").getString();
        } else if (statusMsg == "initialized" || statusMsg == "pending" || statusMsg == "debited")  {
            // pending
            status.error = TXN_PENDING;
            return status;
        }
    }
    


    return status;
}

int vasPayloadGenerator(char* auxPayload, const size_t auxPayloadSize, const struct Eft* context)
{
    iisys::JSObject* jsonReq = static_cast<iisys::JSObject*>(context->callbackdata);
    
    (*jsonReq)("journal") = getJournal(context);
    strncpy(auxPayload, jsonReq->dump().c_str(), auxPayloadSize -1 );

    return 0;

}

const char* vasOrganizationName()
{
    return "ITEX";
}

const char* vasOrganizationCode()
{
    return "100100";
}

std::string vasApiKey()
{
    char key[] = "a6Q6aoHHESonso27xAkzoBQdYFtr9cKC";
    rot13(key);
    return std::string(key);
}
