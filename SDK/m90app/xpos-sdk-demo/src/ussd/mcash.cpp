#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <string>
#include <vector>

#include "EmvDBUtil.h"
#include "vas/simpio.h"
#include "vas/jsobject.h"
#include "merchant.h"

#include "ussdb.h"
#include "ussdadmin.h"
#include "ussdservices.h"

#include "mcash.h"

extern "C" {
#include "util.h"
#include "network.h"
}


std::string dateTimeRef()
{
    char dateTime[32] = {0};
    time_t now = time(NULL);
    strftime(dateTime, sizeof(dateTime), "%Y%m%d%H%M%S", localtime(&now));
    return std::string(dateTime + 2);
}

std::string sendAndReceiveMCashData(const char* requestBody, const char* path)
{
    NetWorkParameters netParam;
    std::string response;

    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    strncpy((char *)netParam.host, "197.253.19.76", sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Request", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = 9898;
    netParam.endTag = "";


    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST %s HTTP/1.1\r\n", path);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s\r\n", netParam.host);

    if (requestBody) {
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", strlen(requestBody));

        netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", requestBody);

    }

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        // UI_ShowButtonMessage(10000, "Error", errorMsg, "Ok", UI_DIALOG_TYPE_WARNING);
        return response;
    }

    response = bodyPointer((const char*)netParam.response);

    return response;
}

std::string generateInitiateMessage(const char* phone, const unsigned long amount, const char* tid)
{
    char message[1024] = { 0 };
    char key[] = "itex-4839";
    // char email[] = "ewebstech@gmail.com";

    sprintf(message, "{\"phoneNumber\":\"%s\",\"amount\":\"%.2f\",\"terminalId\":\"%s\",\"key\":\"%s\"}", phone, amount / 100.0f, tid, key);

    return std::string(message);
}

std::string generateStatusCheckMessage(const char* operationId, const char* tid)
{
    char request[1024] = { 0 };
    char key[] = "itex-4839";

    sprintf(request, "{\"operationId\":\"%s\",\"terminalId\":\"%s\",\"key\":\"%s\"}", operationId, tid, key);

    return std::string(request);
}

int updateReceiptDetails(std::map<std::string, std::string>& record, const std::string& phone, unsigned long amount, const std::string& operationId, const USSDService service)
{
    char formattedTimestamp[32] = { 0 };
    char amountStr[16] = {0};
    
    snprintf(amountStr, sizeof(amountStr), "%lu", amount);
    getFormattedDateTime(formattedTimestamp, sizeof(formattedTimestamp));
    
    record[USSD_DATE] = formattedTimestamp;
    record[USSD_REF] = dateTimeRef();
    record[USSD_PHONE] = phone;
    record[USSD_AMOUNT] = amountStr;
    record[USSD_SERVICE] = ussdServiceToString(service);
    record[USSD_OP_ID] = operationId;

    return 0;
}

std::map<std::string, std::string> updateReceiptDetails(const std::string& phone, unsigned long amount, const std::string& operationId, const USSDService service)
{
    std::map<std::string, std::string> record;
    updateReceiptDetails(record, phone, amount, operationId, service);
    return record;
}

std::map<std::string, std::string> statusCheckToRecord(const iisys::JSObject& respObj)
{
    std::map<std::string, std::string> record;

    const iisys::JSObject& status = respObj("status");
    const iisys::JSObject& message = respObj("message");
    const iisys::JSObject& chargeRstId = respObj("chargeRequestId");

    if (status.isNumber()) {
        if (status.getInt() == 1) {
            record[USSD_STATUS] = USSDB::trxStatusString(USSDB::APPROVED);
        } else {
            record[USSD_STATUS] = USSDB::trxStatusString(USSDB::DECLINED);
        }
    } else {
        record[USSD_STATUS] = USSDB::trxStatusString(USSDB::STATUS_UNKNOWN);
    }

    if (chargeRstId.isString()) {
        record[USSD_CHARGE_REQ_ID] = chargeRstId.getString();
    }

    if (message.isString()) {
        record[USSD_STATUS_MESSAGE] = message.getString();
    }

    return record;
}

void mCashPurchase(const std::string& phone, const unsigned long amount)
{
    MerchantData mParam;

    memset(&mParam, 0x00, sizeof(MerchantData));
    readMerchantData(&mParam);

    std::string tid = mParam.tid;
    std::string initiateRequest = generateInitiateMessage(phone.c_str(), amount, tid.c_str());

    Demo_SplashScreen("Initializing Transaction", "Please wait...");
    std::string response = sendAndReceiveMCashData(initiateRequest.c_str(), "/mcash/merchant/payment");

    if (response.empty()) {
        UI_ShowButtonMessage(10000, "Initiate Failed", "No Response", "Ok", UI_DIALOG_TYPE_WARNING);
        return;
    }

    iisys::JSObject respObj;
    if (!respObj.load(response)) {
        UI_ShowButtonMessage(10000, "Error", "Invalid response", "Ok", UI_DIALOG_TYPE_WARNING);
        return;
    }

    const iisys::JSObject& status = respObj("status");
    const iisys::JSObject& message = respObj("message");
    const iisys::JSObject& operationIdObj = respObj("operationId");
    const iisys::JSObject& recoveryCode = respObj("recoveryShortCode");

    if (!status.isNumber() || status.getInt() != 1 || recoveryCode.isNull() || operationIdObj.isNull()) {
        if (!message.isNull()) {
            UI_ShowButtonMessage(10000, "Error", message.getString().c_str(), "Ok", UI_DIALOG_TYPE_WARNING);
        }
        return;
    }

    if (!message.isNull()) {
        UI_ShowButtonMessage(1000, "", message.getString().c_str(), "Ok", UI_DIALOG_TYPE_CONFIRMATION);
    }

    std::string paymentCode = std::string("USSD: ") + recoveryCode.getString();
    const std::string operationId = operationIdObj.getString();
    if (UI_ShowOkCancelMessage(10 * 60 * 100, "Continue Payment?", paymentCode.c_str(), UI_DIALOG_TYPE_CONFIRMATION) != 0) {
        std::map<std::string, std::string> record = updateReceiptDetails(phone, amount, operationId, MCASH_PURCHASE);
        record[USSD_STATUS_MESSAGE] = "Payment Canceled";
        USSDB::saveUssdTransaction(record);
        printUSSDReceipt(record);
        return;
    }

    // Complete transaction
    std::string statusCheck = generateStatusCheckMessage(operationId.c_str(), tid.c_str());

    Demo_SplashScreen("Confirming Status", "Please wait...");
    response = sendAndReceiveMCashData(statusCheck.c_str(), "/mcash/transaction/status");

    if (response.empty()) {
        UI_ShowButtonMessage(10000, "Error", "No Response", "Ok", UI_DIALOG_TYPE_WARNING);
        return;
    } else if (!respObj.load(response)) {
        UI_ShowButtonMessage(10000, "Error", "Invalid response", "Ok", UI_DIALOG_TYPE_WARNING);
        return;
    }

    std::map<std::string, std::string> record = statusCheckToRecord(respObj);
    updateReceiptDetails(record, phone, amount, operationId, MCASH_PURCHASE);
    USSDB::saveUssdTransaction(record);
    printUSSDReceipt(record);
}

void mCashTransaction(USSDService service)
{
    while (1) {
        std::string phone = getPhoneNumber("Phone Number", "");
        if (phone.empty()) {
            return;
        }

        long amount = getAmount(ussdServiceToString(service));
        if (amount <= 0) {
            continue;
        }

        char detail[128] = { 0 };
        snprintf(detail, sizeof(detail), "PAYER'S PHONE:\n%s\nAMOUNT:\nNGN %.2f\n\nCHARGES: N%.2f", phone.c_str(), amount / 100.0f, amount > 100000 ? 20.0f : 0.0f);

        if (UI_ShowOkCancelMessage(10 * 60 * 100, "Please Confirm", detail, UI_DIALOG_TYPE_CONFIRMATION) != 0) {
            continue;
        }

        return mCashPurchase(phone, (const unsigned long)amount);
    }
}

void paymentStatus()
{
    MerchantData mParam;
	USSDB database;
    std::vector<std::map<std::string, std::string> > transactions;

    memset(&mParam, 0x00, sizeof(MerchantData));
    readMerchantData(&mParam);

	std::string reference = getNumeric(0, 60000, "Reference Number", ussdServiceToString(MCASH_PURCHASE), UI_DIALOG_TYPE_NONE);
    if (reference.empty()) {
        return;
    }

	database.selectTransactionsByRef(transactions, reference.c_str(), "");

    int selection = showUssdTransactions(60000, "Select", transactions, 0);

    if (selection < 0) {
        return;
    }

    std::map<std::string, std::string>& transaction = transactions[selection];
    std::string tid = mParam.tid;
    std::string statusCheck = generateStatusCheckMessage(transaction[USSD_OP_ID].c_str(), tid.c_str());

    Demo_SplashScreen("Confirming Status", "Please wait...");
    std::string response = sendAndReceiveMCashData(statusCheck.c_str(), "/mcash/transaction/status");

    iisys::JSObject respObj;
    if (response.empty()) {
        UI_ShowButtonMessage(10000, "Error", "No Response", "Ok", UI_DIALOG_TYPE_WARNING);
        return;
    } else if (!respObj.load(response)) {
        UI_ShowButtonMessage(10000, "Error", "Invalid response", "Ok", UI_DIALOG_TYPE_WARNING);
        return;
    }

    long primaryIndex = atol(transaction[USSD_ID].c_str());
    std::map<std::string, std::string> record = statusCheckToRecord(respObj);

    UI_ShowButtonMessage(5000, "Status", record[USSD_STATUS].c_str(), "Ok", UI_DIALOG_TYPE_NONE);
    if (record[USSD_STATUS] != USSDB::trxStatusString(USSDB::STATUS_UNKNOWN)) {
        transaction[USSD_STATUS] = record[USSD_STATUS];
        transaction[USSD_CHARGE_REQ_ID] = record[USSD_CHARGE_REQ_ID];
        transaction[USSD_STATUS_MESSAGE] = record[USSD_STATUS_MESSAGE];
        
        database.updateUssdTransaction(record, primaryIndex);
        printUSSDReceipt(transaction);
    }
	
	return;
}

int mCashMenu()
{
    std::vector<std::string> menu;

    menu.push_back(ussdServiceToString(MCASH_PURCHASE));
    menu.push_back("Payment Status");

    while (1) {

        switch (UI_ShowSelection(30000, "USSD Payments", menu, 0)) {
        case 0:
            mCashTransaction(MCASH_PURCHASE);
            break;
        case 1:
            paymentStatus();
            break;
        default:
            return -1;
        }
    }

    return 0;
}
