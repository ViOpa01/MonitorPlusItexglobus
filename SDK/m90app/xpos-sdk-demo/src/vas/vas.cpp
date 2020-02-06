#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>



#include "vas.h"
#include "simpio.h"
#include "vasdb.h"
#include "Receipt.h"

#include "vas/jsobject.h"

#include "electricity.h"
#include "cashio.h"
#include "airtime.h"
#include "data.h"
#include "paytv.h"
#include "smile.h"

#include "vasadmin.h"
#include "vas.h"

#include "merchant.h"

extern int formatAmount(std::string& ulAmount);

extern "C" {
#include "util.h"
}



const char* vasMenuString(VAS_Menu_T type)
{
    switch (type) {
    case ENERGY:
        return "Electricity";
    case AIRTIME:
        return "Airtime";
    case TV_SUBSCRIPTIONS:
        return "TV Subscriptions";
    case DATA:
        return "Data";
    case SMILE:
        return "Smile";
    case CASHIO:
        return "Cash In / Cash Out";
    default:
        return "";
    }
}

const char* serviceToString(Service service)
{
    switch (service) {
    case IKEJA:
        return "Ikeja Electric";
    case EKEDC:
        return "Eko Electric";
    case EEDC:
        return "Enugu Electric";
    case IBEDC:
        return "Ibadan Electric";
    case PHED:
        return "Port-Harcourt Elec.";
    case AEDC:
        return "Abuja Electric";
    case KEDCO:
        return "Kano Electric";
    case AIRTELVTU:
        return "Airtel VTU";
    case AIRTELVOT:
        return "Airtel VOT";
    case AIRTELVOS:
        return "Airtel VOS";
    case AIRTELDATA:
        return "Airtel Data";
    case ETISALATVTU:
        return "9MOBILE VTU";
    case ETISALATVOT:
        return "9MOBILE VOT";
    case ETISALATVOS:
        return "9MOBILE VOS";
    case ETISALATDATA:
        return "9Mobile Data";
    case GLOVTU:
        return "Glo VTU";
    case GLOVOT:
        return "Glo VOT";
    case GLOVOS:
        return "Glo VOS";
    case GLODATA:
        return "Glo Data";
    case MTNVTU:
        return "MTN VTU";
    case MTNVOT:
        return "MTN VOT";
    case MTNVOS:
        return "MTN VOS";
    case MTNDATA:
        return "MTN Data";
    case DSTV:
        return "DSTv";
    case GOTV:
        return "GoTv";
    case SMILETOPUP:
        return "Smile Topup";
    case SMILEBUNDLE:
        return "Smile Bundle";
    case STARTIMES:
        return "Startimes";
    case TRANSFER:
        return "Transfer";
    case WITHDRAWAL:
        return "Withdrawal";
    default:
        return "";
    }
}

const char* serviceStringToLogoFile(const std::string& serviceString)
{
    if (serviceString == serviceToString(IKEJA)) {
        return "../../../flash/vaslogo/ikeja.bmp";
    } else if (serviceString == serviceToString(EKEDC)) {
        return "../../../flash/vaslogo/ekedc.bmp";
    } else if (serviceString == serviceToString(EEDC)) {
        return "../../../flash/vaslogo/eedc.bmp";
    } else if (serviceString == serviceToString(IBEDC)) {
        return "../../../flash/vaslogo/ibedc.bmp";
    } else if (serviceString == serviceToString(PHED)) {
        return "../../../flash/vaslogo/phed.bmp";
    } else if (serviceString == serviceToString(AEDC)) {
        return "../../../flash/vaslogo/aedc.bmp";
    } else if (serviceString == serviceToString(KEDCO)) {
        return "../../../flash/vaslogo/kano.bmp";
    } else if (serviceString == serviceToString(DSTV)) {
        return "../../../flash/vaslogo/dstv.bmp";
    } else if (serviceString == serviceToString(GOTV)) {
        return "../../../flash/vaslogo/gotv.bmp";
    } else if (serviceString == serviceToString(SMILETOPUP)
                || serviceString == serviceToString(SMILEBUNDLE)) {
        return "../../../flash/vaslogo/smile.bmp";
    } else if (serviceString == serviceToString(ETISALATVTU)
                || serviceString == serviceToString(ETISALATDATA)
                ||  serviceString == serviceToString(ETISALATVOS)
                ||  serviceString == serviceToString(ETISALATVOT) ) {
        return "../../../flash/vaslogo/etisalat.bmp";
    } else if (serviceString == serviceToString(AIRTELVTU)
                || serviceString == serviceToString(AIRTELDATA)
                ||  serviceString == serviceToString(AIRTELVOS)
                ||  serviceString == serviceToString(AIRTELVOT) ) {
        return "../../../flash/vaslogo/airtel.bmp";
    } else if (serviceString == serviceToString(GLOVTU)
                || serviceString == serviceToString(GLODATA)
                ||  serviceString == serviceToString(GLOVOS)
                ||  serviceString == serviceToString(GLOVOT) ) {
        return "../../../flash/vaslogo/glo.bmp";
    } else if (serviceString == serviceToString(MTNVTU)
                || serviceString == serviceToString(MTNDATA)
                ||  serviceString == serviceToString(MTNVOS)
                ||  serviceString == serviceToString(MTNVOT) ) {
        return "../../../flash/vaslogo/mtn.bmp";

    }
    return "";
}

const char* serviceToProductString(Service service)
{
	switch (service) {
	case IKEJA:
		return "IKEJAELECTRIC";
	case EKEDC:
		return "EKOELECTRIC";
    case EEDC:
		return "ENUGUELECTRIC";
    case IBEDC:
		return "IBADANELECTRIC";
    case PHED:
		return "PORTELECTRIC";
    case AEDC:
		return "ABUJAELECTRIC";
    case KEDCO:
		return "KANOELECTRIC";
	case ETISALATVTU:
		return "ETISALATVTU";
	case AIRTELVTU:
		return "AIRTELVTU";
	case GLOVTU:
		return "GLOVTU";
	case MTNVTU:
		return "MTNVTU";
	case ETISALATDATA:
		return "ETISALATDATA";
	case AIRTELDATA:
		return "AIRTELDATA";
	case GLODATA:
		return "GLODATA";
	case MTNDATA:
		return "MTNDATA";
	case DSTV:
		return "DSTV";
	case GOTV:
		return "GOTV";
	case WITHDRAWAL:
		return "WITHDRAWAL";
	case TRANSFER:
		return "TRANSFER";
	case SMILETOPUP:
		return "SMILE TOPUP";
	case SMILEBUNDLE:
		return "SMILE BUNDLE";
	// case VAS_GENESIS:
	// 	return "GENESIS";
	case MTNVOS:
		return "MTNVOS";
	case ETISALATVOS:
		return "ETISALATVOS";
	case AIRTELVOS:
		return "AIRTELVOS";
    case GLOVOS:
		return "GLOVOS";
	case MTNVOT:
		return "MTNVOT";
    case ETISALATVOT:
		return "ETISALATVOT";
    case AIRTELVOT:
		return "AIRTELPIN";
    case GLOVOT:
		return "GLOVOT";
	default:
		return "VAS";
	}
}

const char* paymentString(PaymentMethod method)
{
    switch (method) {
    case PAY_WITH_CARD:
        return "Card";
    case PAY_WITH_CASH:
        return "Cash";
    case PAY_WITH_MCASH:
        return "mCash";
    default:
        return "";
    }
}

int vasTransactionTypes()
{
    static int once_flag = VasDB::init();
    std::vector<std::string> menu;
    VAS_Menu_T menuOptions[] = { ENERGY, AIRTIME, TV_SUBSCRIPTIONS, DATA, SMILE, CASHIO };

    (void)once_flag;

    for (size_t i = 0; i < sizeof(menuOptions) / sizeof(VAS_Menu_T); ++i) {
        menu.push_back(vasMenuString(menuOptions[i]));
    }
    menu.push_back("Vas Admin");

    while (1) {
        int selection = UI_ShowSelection(30000, "Services", menu, 0);

        if (selection < 0) {
            return -1;
        } else if (menu.size() - 1 == selection) {
            vasAdmin();
        } else {
            doVasTransaction(menuOptions[selection]);
        }
    }
    
    return 0;
}

int doVasTransaction(VAS_Menu_T menu)
{
    VasFlow flow;
    Postman postman;
    const char * title = vasMenuString(menu);

    switch (menu) {
    case ENERGY: {
        Electricity electricity(title, postman);
        startVas(flow, &electricity);
        break;
    }
    case AIRTIME: {
        Airtime airtime(title, postman);
        startVas(flow, &airtime);
        break;
    }
    case TV_SUBSCRIPTIONS: {
        PayTV televisionSub(title, postman);
        startVas(flow, &televisionSub);
        break;
    }
    case DATA: {
        Data data(title, postman);
        startVas(flow, &data);
        break;
    }
    case SMILE: {
        Smile smile(title, postman);
        startVas(flow, &smile);
        break;
    }
    case CASHIO: {
        ViceBanking cashIO(title, postman);
        startVas(flow, &cashIO);
        break;
    }
    default:
        break;
    }

    return 0;
}

int printVas(std::map<std::string, std::string>& record)
{
    iisys::JSObject serviceData;
    int printStatus;

    if (serviceData.load(record[VASDB_SERVICE_DATA]) && serviceData.isObject()) {
        for (iisys::JSObject::iterator begin = serviceData.begin(),
                                        end = serviceData.end(); begin != end; ++begin) {
            if (record.find(begin->first) == record.end())
                record[begin->first] = begin->second.getString();
        }
    }

    formatAmount(record[VASDB_AMOUNT]);
    record["currencySymbol"] = getCurrencySymbol();
    if(record.find(VASDB_DATE) != record.end() && record[VASDB_DATE].length() > 19) {
        record[VASDB_DATE].erase(19, std::string::npos);
    }

    record["vaslogo"] = serviceStringToLogoFile(record[VASDB_SERVICE]);

    const char* copies[] = {"- Customer Copy -", "- Agent Copy -"};    
    int count = 1;
    if (record.find("receipt_copy") == record.end()) {
        if (record[VASDB_STATUS] == VasDB::trxStatusString(VasDB::APPROVED)) {
            count = (int) sizeof(copies) / sizeof(char*);
        }
        record["receipt_copy"] = copies[0];
    }

    for (int i = 0; i < count; ++i) {
        if (count > 1) {
            record["receipt_copy"] = copies[i];
            if (i > 0 && UI_ShowOkCancelMessage(10000, copies[i], "Print", UI_DIALOG_TYPE_CONFIRMATION) != 0) {
                break;
            }
        }

        if (record[VASDB_CATEGORY] == vasMenuString(ENERGY)) {
            // printStatus = checkedPrint(record, "vaselectricity.html");
        } else if (record[VASDB_CATEGORY] == vasMenuString(AIRTIME) || record[VASDB_CATEGORY] == vasMenuString(DATA)) {
            // printStatus = checkedPrint(record, "vasairtimedata.html");
        } else if (record[VASDB_CATEGORY] == vasMenuString(TV_SUBSCRIPTIONS)) {
            // printStatus = checkedPrint(record, "vastvsubs.html");
        } else if (record[VASDB_CATEGORY] == vasMenuString(SMILE)) {
            // printStatus = checkedPrint(record, "vasReceipt.html");
        } else {
            // printStatus = checkedPrint(record, "vasReceipt.html");
        }
    }


    return printStatus;
}


std::string getCurrencySymbol()
{
    return "NGN";
    // iisys::JSObject currency = Merchant().object(config::CURRENCY_SYMBOL);

    // if (currency.isNull() || currency.getString().empty()) {
    //     return std::string("NGN");
    // }

    // return currency.getString();
}

PaymentMethod getPaymentMethod(const PaymentMethod preferredMethods)
{
    int selection;
    std::vector<std::string> menu;
    std::vector<PaymentMethod> methods;
    PaymentMethod knownMethods[] = {PAY_WITH_CARD, PAY_WITH_CASH, PAY_WITH_MCASH};


    for (size_t i = 0; i < sizeof(knownMethods) / sizeof(PaymentMethod); ++i) {
        if (preferredMethods & knownMethods[i]) {
            methods.push_back(knownMethods[i]);
            menu.push_back(paymentString(knownMethods[i]));
        }
    }

    if (methods.size() == 0) {
        return PAYMENT_METHOD_UNKNOWN;
    }


    selection = UI_ShowSelection(30000, "Payment Options", menu, 0);
    if (selection < 0) {
        return PAYMENT_METHOD_UNKNOWN;
    }
    
    return methods[selection];
}

Service selectService(const char * title, std::vector<Service>& services)
{
    int selection;
    std::vector<std::string> menu;

    if (services.size() == 0) {
        return SERVICE_UNKNOWN;
    }

    for (size_t i = 0; i < services.size(); ++i) {
        menu.push_back(serviceToString(services[i]));
    }

    selection = UI_ShowSelection(30000, title, menu, 0);
    if (selection < 0) {
        return SERVICE_UNKNOWN;
    }
    
    return services[selection];
}

VasStatus vasErrorCheck(const iisys::JSObject& data)
{
    iisys::JSObject error = data("error");
    iisys::JSObject message = data("message");

    if (error.isNull()) {
        return VasStatus(KEY_NOT_FOUND, "No Error Tag");
    }

    if (error.getBool() == true) {
        if (message.isNull() || !message.isString()) {
            return VasStatus(VAS_ERROR);
        }
        return VasStatus(VAS_ERROR, message.getString().c_str());
    }

    if (message.isNull() || !message.isString()) {
        return VasStatus(NO_ERRORS);
    }

    return VasStatus(NO_ERRORS, message.getString().c_str());
}

VasError requeryVas(iisys::JSObject& transaction, const char * clientRef, const char *serverRef)
{
    NetWorkParameters netParam = {'\0'};
    iisys::JSObject json;
    Payvice payvice;

    if (!loggedIn(payvice)) {
        return NOT_LOGGED_IN;
    }    

    json("wallet") = payvice.object(Payvice::WALLETID);
    json("username") = payvice.object(Payvice::USER);
    json("password") = payvice.object(Payvice::PASS);
    json("sequence") = serverRef;
    json("channel") =  "POS";

	if (serverRef) {
        json("sequence") = serverRef;
	} else if (clientRef) {
        json("clientReference") = clientRef;
	}

    std::string body = json.dump();

    strncpy((char *)netParam.host, "basehuge.itexapp.com", sizeof(netParam.host) - 1);
    netParam.receiveTimeout = 60000;
	strncpy(netParam.title, "Requery", 10);
    netParam.isHttp = 1;
    netParam.isSsl = 0;
    netParam.port = 8090;
    netParam.endTag = "";  // I might need to comment it out later
    
    
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "POST /api/v1/account/requery HTTP/1.1\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Host: %s:%d\r\n", netParam.host, netParam.port);
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Type: application/json\r\n");
    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "Content-Length: %zu\r\n", body.length());

    netParam.packetSize += sprintf((char *)(&netParam.packet[netParam.packetSize]), "\r\n%s", body.c_str());

    if (sendAndRecvPacket(&netParam) != SEND_RECEIVE_SUCCESSFUL) {
        return VAS_ERROR;
    }

    if (!json.load(bodyPointer((const char*)netParam.response))) {
        return INVALID_JSON;
    }

    iisys::JSObject& status = json("status");
    iisys::JSObject& error = json("error");

    if (!error.isBool() || !status.isNumber()) {
        return VAS_ERROR;
    }

    if (status.getInt() == 3) {
        return TXN_NOT_FOUND;
    } else if (status.getInt() == 14) {
        return TXN_PENDING;
    } else if (status.getInt() != 1 || error.getBool() == true ) {
        return VAS_ERROR;
    }

    transaction = json("transaction");  // I hope this is an implicit copy

	/*
	{
    "status": 1,
    "message": "Successful",
    "error": false,
    "description": "",
    "transaction": {
        "sequence": 1000111,
        "amount": 2000,
        "terminal": null,
        "VASCustomerAccount": "62154000814",
        "VASCustomerPhone": "08161663466",
        "VASCustomerEmail": null,
        "VASCustomerAddress": null,
        "status": "successful",
        "paymentMethod": "card",
        "currency": "NGN",
        "name": "Ibadan Electricity",
        "reversal": 0,
        "reversalReference": null,
        "reference": "ITEXIBEDC5c0d4ba0ee483358",
        "walletReference": "172454",
        "VASBillerName": "IBEDC",
        "VASProviderName": "FETSIBEDC",
        "category": "Electricity",
        "product": "IBEDC",
        "channel": "POS",
        "providerReference": null,
        "reason": "transactionsuccessful",
        "message": "Customer Payment Successful",
        "value": "0",
        "token": "5671  4405  3529  4889  1880  ",
        "response": "{\"status\":1,\"error\":false,\"message\":\"Customer Payment Successful\",\"description\":\"0\",\"reference\":\"ITEXIBEDC5c0d4ba0ee483358\",\"name\":\"\",\"account\":\"62154000814\",\"amount\":2000,\"type\":\"prepaid\",\"value\":0,\"token\":\"5671  4405  3529  4889  1880  \",\"tokenData\":[],\"tokenDetails\":{\"tIndex\":\"\",\"rate\":\"0\",\"rnd\":[{\"amount\":\"20.0\",\"kind\":\"Bill\",\"description\":\"PREPAID\",\"taxAmount\":\"1.0\"}],\"kwH\":0.76,\"cFee\":0,\"sgc\":\"0000\"},\"tokenDataType\":\"\",\"tokenDataToken\":\"\",\"tokenDetailsRate\":\"0\",\"tokenDetailsAmount\":\"20.0\",\"tokenDetailsKind\":\"Bill\",\"tokenDetailsDescription\":\"PREPAID\",\"tokenDetailsTaxAmount\":\"1.0\",\"tokenDetailsKWH\":\"0.76\",\"productCode\":\"37BAE6F83B8EB8CAAD40BA7456A1ED719E9C265C2E5506DD6AE27A7498154BAFE0F67BCBFAEA442354C6FBD685F080ABAFA9A2F8C3ABD0F2543C275E861E2D76|eyJwcm9kdWN0IjoiSUJBREFORUxFQ1RSSUNJVFkiLCJhY2NvdW50IjoiMTcxMjMwMTY0MjAxIiwibmFtZSI6IiBNVVJJVEFMQSBSRElMT1JJTiBTRUdVTiAgICAgR0FSVUJBICAiLCJhZGRyZXNzIjoiIE1VUklUQUxBIFJESUxPUklOIiwidHlwZSI6InBvc3RwYWlkIiwicGF5bWVudFR5cGUiOiIwIiwiYW1vdW50IjowLCJtaW5pbXVtQW1vdW50IjowLCJvdXRzdGFuZGluZ0Ftb3VudCI6MCwicGhvbmUiOiIwMDAwMDAwMDAwMCIsImVtYWlsIjoiIiwidGFycmlmQ29kZSI6IlIyICAiLCJ0aGlyZFBhcnR5Q29kZSI6IkNIQUwiLCJ1bmlxdWVQcm9kdWN0Q29kZUlkIjoiNWMwOTIxMTZjMjVkMDIzIn0%3D\",\"externalReference\":\"\"}",
        "request": null,
        "date": "2018-12-09 18:06:51",
        "updateDate": "2018-12-09 18:06:51"
    }
}
	*/
	
	/*
	enum('status', ['initialized', 'debited', 'pending', 'successful', 'failed', 'declined'])->default('initialized');
	*/

	return NO_ERRORS;
}


const char * getDeviceTerminalId()
{
    MerchantData mParam = { 0 };
    static char tid[9] = { 0 };
    
    readMerchantData(&mParam);
    memset(tid, 0, sizeof(tid));
    strcpy(tid, mParam.tid);
    
    return tid;
}

char menuendl()
{
    return ' ';
}
