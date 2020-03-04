#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "EmvDBUtil.h"
#include "vas/simpio.h"
#include "vas/jsobject.h"
#include "merchant.h"

#include "ussdb.h"
#include "ussdadmin.h"
#include "ussdservices.h"

#include "cgate.h"

extern "C" {
#include "util.h"
#include "network.h"
#include "nibss.h"
}

extern void formattedDateTime(char* dateTime, size_t len);

std::string fetchCoralCode(USSDService service, float amount, const char* ref = NULL)
{
    NetWorkParameters netParam;
    MerchantData mParam;
    MerchantParameters parameter;
    char path[1024] = { 0 };
    char transType[4] = { 0 };

    memset(&netParam, 0x00, sizeof(NetWorkParameters));
    memset(&mParam, 0x00, sizeof(MerchantData));
    memset(&parameter, 0x00, sizeof(MerchantParameters));

    readMerchantData(&mParam);
    getParameters(&parameter);

    strncpy((char *)netParam.host, "basehuge.itexapp.com", sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Request", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = 80;
    netParam.endTag = "";

    std::string response;

    if (service == CGATE_CASHOUT) {
        strcpy(transType, "30");
    } else if (service == CGATE_PURCHASE) {
        strcpy(transType, "0");
    }

    if (ref) {
        snprintf(path, sizeof(path), "/tams/eftpos/devinterface/transactionadvice.php?action=TAMS_WEBAPI&control=CORALPAY_CONFIRMATION&AMOUNT=%.2f&termid=12345678&tid=%s&merchantcode=%s&REF=%s&transType=%s", amount, mParam.tid, parameter.cardAcceptiorIdentificationCode, ref, transType);
    } else {
        snprintf(path, sizeof(path), "/tams/eftpos/devinterface/transactionadvice.php?action=TAMS_WEBAPI&control=CORALPAY&AMOUNT=%.2f&termid=12345678&tid=%s&merchantcode=%s&REF=&transType=%s", amount, mParam.tid, parameter.cardAcceptiorIdentificationCode, transType);
    }

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "GET %s HTTP/1.1\r\n", path);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n\r\n", netParam.host, netParam.port);

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        // UI_ShowButtonMessage(10000, "Error", errorMsg, "Ok", UI_DIALOG_TYPE_WARNING);
        return response;
    }

    response = bodyPointer((const char*)netParam.response);

    return response;
}

std::map<std::string, std::string> cGateToRecord(const USSDService service, const unsigned long amount, const iisys::JSObject& confirmation)
{
    std::map<std::string, std::string> record;
    char amountStr[16] = {0};
    char formattedTimestamp[32] = {0};

    if (!confirmation("CoralPAY_amount").isNull()) {
        // assert(confirmation("CoralPAY_amount") == amount)
    }
    snprintf(amountStr, sizeof(amountStr), "%lu", amount);
    formattedDateTime(formattedTimestamp, sizeof(formattedTimestamp));

    const iisys::JSObject& phone = confirmation("CoralPAY_customer_mobile");
    const iisys::JSObject& merchantName = confirmation("CoralPAY_merchantname");
    const iisys::JSObject& institutionCode = confirmation("CoralPAY_institutionCode");
    const iisys::JSObject& completionRef = confirmation("CoralPAY_reference");
    const iisys::JSObject& statusMessage = confirmation("CoralPAY_responsemessage");
    const iisys::JSObject& auditRef = confirmation("CoralPAY_tnx");
    const iisys::JSObject& respCode = confirmation("CoralPAY_responseCode");

    record[USSD_SERVICE] = ussdServiceToString(service);
    record[USSD_PHONE] = phone.getString();
    record[USSD_MERCHANT_NAME] = merchantName.getString();
    record[USSD_INSTITUTION_CODE] = institutionCode.getString();
    record[USSD_REF] = completionRef.getString();
    record[USSD_STATUS_MESSAGE] = statusMessage.getString();
    record[USSD_AUDIT_REF] = auditRef.getString();
    record[USSD_AMOUNT] = amountStr;
    record[USSD_DATE] = formattedTimestamp;

    if (respCode.getString() == "00") {
        record[USSD_STATUS] = USSDB::trxStatusString(USSDB::APPROVED);
    } else {
        record[USSD_STATUS] = USSDB::trxStatusString(USSDB::DECLINED);
    }

    return record;
}

int coralTransaction(const USSDService service)
{
    long amount = getAmount(ussdServiceToString(service));

    if (amount <= 0) {
        return -1;
    }

    Demo_SplashScreen("Initiating Transaction", "Please wait...");
    std::string body = fetchCoralCode(service, amount / 100.0f);
    if (body.empty()) {
        UI_ShowButtonMessage(5000, "No Response", "", "Ok", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    iisys::JSObject cgate;
    if (!cgate.load(body)) {
        UI_ShowButtonMessage(5000, "Error", "", "Ok", UI_DIALOG_TYPE_WARNING);
        return -1;
    }

    iisys::JSObject& cgateRef = cgate("CoralPAY_reference");
    if (cgateRef.isNull()) {
        UI_ShowButtonMessage(5000, "Error", "No Reference", "Ok", UI_DIALOG_TYPE_WARNING);
        return -1;
    }
    
    bool firstCheck = true;
    std::string responseCode;
    std::string responseMessage;
    iisys::JSObject confirmation;
    std::string reference = "*BankUSSDCode*000*" + cgateRef.getString() + "#";
    do {
        int ret = 0;

        if (firstCheck) {
            firstCheck = false;
            UI_ShowButtonMessage(10 * 60 * 1000, "C'GATE USSD CODE", reference.c_str(), "Check Status", UI_DIALOG_TYPE_CONFIRMATION);
        } else {
            const char* msg = responseMessage.empty() ? "C'GATE USSD CODE" : responseMessage.c_str();
            ret = UI_ShowOkCancelMessage(60000, msg, reference.c_str(), UI_DIALOG_TYPE_CONFIRMATION);

        }

        if (ret < 0) {
            break;
        }

        Demo_SplashScreen("Checking Status", "Please wait...");
        body = fetchCoralCode(service, amount / 100.0f, reference.c_str());
        if (!confirmation.load(body)) {
            return -1;
        }

        if (!confirmation("CoralPAY_responsemessage").isNull()) {
            responseMessage = confirmation("CoralPAY_responsemessage").getString();
        }

        iisys::JSObject& respCode = confirmation("CoralPAY_responseCode");
        if (respCode.isNull()) {
            break;
        }
        responseCode = respCode.getString();

    } while (responseCode == "09" /* Transaction is still processing...*/);


    if (responseCode == "00") {
        std::map<std::string, std::string> record = cGateToRecord(service, amount, confirmation);
        USSDB::saveUssdTransaction(record);
        printUSSDReceipt(record, "cgate.html");
    }
    return 0;
}

int coralPayMenu()
{
    USSDService menuOptions[] = { CGATE_PURCHASE, CGATE_CASHOUT };
    std::vector<std::string> menu;

    for (size_t i = 0; i < sizeof(menuOptions) / sizeof(USSDService); ++i) {
        menu.push_back(ussdServiceToString(menuOptions[i]));
    }

    while (1) {
        int selection = UI_ShowSelection(30000, "USSD Payments", menu, 0);

        if (selection < 0) {
            return -1;
        }

        coralTransaction(menuOptions[selection]);
    }

    return 0;
}

