/**
 * File: tribease.c
 * -------------
 * Implement Tribease application.
 */

#include "tribease.h"

#include <map>
#include <stdio.h>
#include <stdlib.h>

#include "../vas/platform/simpio.h"
#include "tribease/tribease.h"
#include "ussdadmin.h"
#include "ussdb.h"
#include "ussdservices.h"

extern "C" {
#include "../util.h"
}

std::map<std::string, std::string> tribeaseToRecord(
    const USSDService service, TribeaseClient* tribeaseClient)
{
    std::map<std::string, std::string> record;
    char amountStr[16] = { 0 };
    char formattedTimestamp[32] = { 0 };

    snprintf(amountStr, sizeof(amountStr), "%lu", tribeaseClient->amount);
    getFormattedDateTime(formattedTimestamp, sizeof(formattedTimestamp));

    record[USSD_SERVICE] = ussdServiceToString(service);
    if (tribeaseClient->customerPhoneNo[0]) {
        record[USSD_PHONE] = tribeaseClient->customerPhoneNo;
    }
    if (tribeaseClient->name[0]) {
        record[USSD_CUSTOMER_NAME] = tribeaseClient->name;
    }
    if (tribeaseClient->paymentRef[0]) {
        record[USSD_PAYMENT_REF] = tribeaseClient->paymentRef;
    }
    if (tribeaseClient->ref[0]) {
        record[USSD_REF] = tribeaseClient->transactionRef;
    }
    if (tribeaseClient->token) {
        char buff[16] = {'\0'};
        sprintf(buff, "%d", tribeaseClient->token);
        record[USSD_REF] = buff;
    }
    if (tribeaseClient->responseMessage[0]) {
        record[USSD_STATUS_MESSAGE] = tribeaseClient->responseMessage;
    }
    if (tribeaseClient->transactionRef[0]) {
        record[USSD_AUDIT_REF] = tribeaseClient->ref;
    }
    record[USSD_AMOUNT] = amountStr;
    record[USSD_DATE] = formattedTimestamp;

    if (tribeaseClient->transactionStatus) {
        record[USSD_STATUS] = USSDB::trxStatusString(USSDB::APPROVED);
    } else {
        record[USSD_STATUS] = USSDB::trxStatusString(USSDB::DECLINED);
    }

    return record;
}

static void tribeasePurchase(void)
{
    TribeaseClient tribeaseClient = { '\0' };
    if (tribeaseGetAuthToken(&tribeaseClient) != EXIT_SUCCESS) {
        return;
    }
    if (tribeaseGetClientData(&tribeaseClient, 0) != EXIT_SUCCESS) {
        return;
    }
    if (tribeaseValidation(&tribeaseClient) != EXIT_SUCCESS) {
        return;
    }
    if (tribeasePayment(&tribeaseClient) != EXIT_SUCCESS) {
        return;
    }
    std::map<std::string, std::string> record
        = tribeaseToRecord(TRIBEASE_PURCHASE, &tribeaseClient);
    printUSSDReceipt(record);
}

static void tribeaseToken(void)
{
    TribeaseClient tribeaseClient = { '\0' };
    if (tribeaseGetAuthToken(&tribeaseClient) != EXIT_SUCCESS) {
        return;
    }
    if (tribeaseGetClientData(&tribeaseClient, 1) != EXIT_SUCCESS) {
        return;
    }
    if (tribeaseGeneratePaymentToken(&tribeaseClient) != EXIT_SUCCESS) {
        return;
    }
    if (tribeaseValidatePaymentToken(&tribeaseClient) != EXIT_SUCCESS) {
        return;
    }
    std::map<std::string, std::string> record
        = tribeaseToRecord(TRIBEASE_TOKEN, &tribeaseClient);
    printUSSDReceipt(record);
}

void tribease(void)
{
    USSDService menuItems[] = { TRIBEASE_PURCHASE, TRIBEASE_TOKEN };
    std::vector<std::string> menu;

    for (size_t i = 0; i < sizeof(menuItems) / sizeof(USSDService); i++) {
        menu.push_back(ussdServiceToString(menuItems[i]));
    }
    while (1) {
        int selection = UI_ShowSelection(30000, "Tribease", menu, 0);

        if (selection < 0) {
            return;
        }
        switch (selection) {
        case 0:
            tribeasePurchase();
            break;
        case 1:
            tribeaseToken();
            break;
        default:
            break;
        }
    }
}
