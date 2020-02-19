
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wallet.h"
#include "payvice.h"
#include "vasadmin.h"
#include "vas.h"
#include "EmvDB.h"
#include "simpio.h"

#include "merchant.h"


extern "C" {
#include "remoteLogo.h"
#include "util.h"
#include "ezxml.h"
}

int walletRequest(int balanceOrTransfer)
{
    NetWorkParameters netParam = {'\0'};
    iisys::JSObject json;
    Payvice payvice;
    std::string data;
    long long amount = 0L;

    if (!loggedIn(payvice)) {
        return NOT_LOGGED_IN;
    }    

    strncpy((char *)netParam.host, "basehuge.itexapp.com", sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Payvice", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = 80;
    netParam.endTag = "";  // I might need to comment it out later

    if (balanceOrTransfer == 1/*FOR_WALLET_BALANCE*/) {

        data = "&control=BALANCE&termid=" + payvice.object(Payvice::WALLETID).getString() + "&userid=" + payvice.object(Payvice::USER).getString();

    } else if(balanceOrTransfer == 2 /*FOR_WALLET_TRANSFER*/)
    {
        // Get Amount
        std::string pin;
        char buff[16] = {'\0'};

        amount = getAmount("Transfer Amount");
        if(amount <= 0) return -1;

        sprintf(buff, "%lld", amount);

        if(getPin(pin, "Payvice Pin") != EMV_CT_PIN_INPUT_OKAY) return -1;
        // Build request
        data = "&control=ComTrans&userid=" + payvice.object(Payvice::USER).getString() + "&pin=" + encryptedPin(Payvice(), pin.c_str()) + "&amount=" + buff + "&termid=" + payvice.object(Payvice::WALLETID).getString();

    }
    
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "GET /ctms/eftpos/devinterface/transactionadvice.php?action=TAMS_WEBAPI%s\r\n", data.c_str());
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "User-Agent: ItexMorefun\r\n\r\n");


    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return VAS_ERROR;
    }

    printf("Wallet Balance response : %s\n", netParam.response);
    parseWalletResponse((char *)netParam.response, balanceOrTransfer, amount);

}

int parseWalletResponse(char* response, int balanceOrTransfer, long long amount)
{

    ezxml_t root, tran;
    std::string msg;
    std::string balance;
    std::string commission;
    std::string message;

    if (balanceOrTransfer == 1/*FOR_WALLET_BALANCE*/) {

        root = ezxml_parse_str(response, strlen(response));
        if (!root) {
            printf("Error, Please Try Again\n");
            return 0;
        }

        tran = ezxml_child(root, "tran");
        if (!tran) {
            printf("Tag Not Found\n");
            UI_ShowButtonMessage(2000, "Tag", "Tag Not Found", "OK", UI_DIALOG_TYPE_CAUTION);
            ezxml_free(root);
            return 0;
        }

        message = ezxml_child(tran, "message")->txt;

        if (message.empty()) {
            printf("Message Not Found\n");
            UI_ShowButtonMessage(2000, "Message", "Message Not Found", "OK", UI_DIALOG_TYPE_CAUTION);
            ezxml_free(root);
            return -1;
        } else if (strcmp("0", ezxml_child(tran, "result")->txt) || strcmp("0", ezxml_child(tran, "status")->txt)) {
            printf("%s\n", message.c_str());
            UI_ShowButtonMessage(2000, "Message", message.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
            ezxml_free(root);
            return 0;
        }


        balance = ezxml_child(tran, "balance")->txt;
        commission = ezxml_child(tran, "commission")->txt;

        msg = std::string("Wallet Balance:\n") + balance + "\nCommission Balance:\n" + commission;
        UI_ShowButtonMessage(0, "Wallet Balance", msg.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);

        ezxml_free(root);
    }

    if (balanceOrTransfer == 2/*FOR_WALLET_TRANSFER*/) {

        iisys::JSObject json;

        if (!json.load((const char*)response)) {
            return INVALID_JSON;
        }

        iisys::JSObject& result = json("result");
        iisys::JSObject& status = json("status");
        iisys::JSObject& message = json("message");
        iisys::JSObject& balance = json("balance");
        iisys::JSObject& commission = json("commission");


        if (!status.isString() || !result.isString()) {
            char buff[16] = {'\0'};

            sprintf(buff, "NGN %.2f", amount / 100.0);
            msg = std::string("Requested Amount: ") + buff + "\n" + message.getString();
            UI_ShowButtonMessage(0, "Insufficient Balance", msg.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
            
        } else {
            UI_ShowButtonMessage(0, "Message", message.getString().c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
		}

		if (strcmp(status.getString().c_str(), "2") && strcmp(result.getString().c_str(), "1")) {
            msg = std::string("Wallet Balance:\n") + balance.getString() + "\nCommission Balance:\n" + commission.getString();
            UI_ShowButtonMessage(0, "Wallet Info", msg.c_str(), "OK", UI_DIALOG_TYPE_CAUTION);
        }
			
    }
    return 1;
}