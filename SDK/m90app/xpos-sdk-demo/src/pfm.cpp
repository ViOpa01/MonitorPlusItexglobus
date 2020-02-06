

#include <errno.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfm.h"

iisys::JSObject getState()
{
    // int battery_charge;
    // int battery_temp;
    // int has_battery;
    // int charging;
    // int hPrinter;
    // char posUid[21] = {0};
    // char dateAndTime[26] = {0}; // yyyy-MM-dd HH:mm:ss
    // char posModel[13] = {0};
    // char simName[16] = {0};
    // char simID[32] = {0};
    // char imsi[32] = {0};
    // char cellinfo[128] = {0};
    // std::string lTxnAt;
    // std::string interface;
    // Merchant merchant;
    iisys::JSObject state;


    // interface = Device().object(config::NETERFACE).getString();

    // state("ptad") = "Itex Integrated Services";

    // if (!sysGetPropertyString(vfisysinfo::SYS_PROP_HW_SERIALNO, posUid, sizeof(posUid))) {
    //     state("serial") = posUid;
    // }

    // if (!sysGetPropertyInt(vfisysinfo::SYS_PROP_BATTERY_CHARGE_LEVEL, battery_charge)) {
    //     state("bl") = battery_charge;
    // }

    // if (!sysGetPropertyInt(vfisysinfo::SYS_PROP_BATTERY_TEMP, battery_temp)) {
    //     state("btemp") = battery_temp;
    // }

    // if (!sysGetPropertyString(vfisysinfo::SYS_PROP_RTC, dateAndTime, sizeof(dateAndTime))) {

    //     time_t timer;
    //     struct tm* tm_info;

    //     time(&timer);
    //     tm_info = localtime(&timer);
    //     strftime(dateAndTime, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    //     state("ctime") = dateAndTime;
    // }

    // if (!sysGetPropertyInt(vfisysinfo::SYS_PROP_BATTERY_CHARGING, charging)) {
    //     switch (charging) {
    //     case 0:
    //         state("cs") = "NotCharging";
    //         break;
    //     case 1:
    //         state("cs") = "Charging";
    //         break;
    //     default:
    //         state("cs") = "Unknown";
    //         break;
    //     }
    // }

    // if (!sysGetPropertyInt(vfisysinfo::SYS_PROP_PRINTER_AVAILABLE, hPrinter)) {
    //     switch (hPrinter) {
    //     case 0:
    //         state("ps") = "PrinterFailure";
    //         break;
    //     case 1:
    //         state("ps") = "PrinterAvailable";
    //         break;
    //     default:
    //         state("ps") = "Unknown";
    //         break;
    //     }
    // }

    // state("tid") = merchant.object(config::TID).getString();
    // state("mid") = merchant.object(config::MID).getString();

    // if (!strcmp(interface.c_str(), config::GPRS0)) {
    //     //COM_PROP_RADIO_ACCESS_TECH
    //     int netwType;
    //     if (!com_GetDevicePropertyInt(COM_PROP_RADIO_ACCESS_TECH, &netwType, NULL)) {
    //         switch (netwType) {
    //         case -1:
    //             state("coms") = "Notconnected";
    //             break;
    //         case 0:
    //             state("coms") = "GSM";
    //             break;
    //         case 1:
    //             state("coms") = "GSM/UMTSDualMode";
    //             break;
    //         case 2:
    //             state("coms") = "UTRAN";
    //             break;
    //         case 3:
    //             state("coms") = "GSMwithEGPRS";
    //             break;
    //         case 4:
    //             state("coms") = "UTRANwithHSDPA";
    //             break;
    //         case 5:
    //             state("coms") = "UTRANwithHSUPA";
    //             break;
    //         case 6:
    //             state("coms") = "UTRANwithHSDPA/HSUPA";
    //             break;
    //         case 7:
    //             state("coms") = "GSM/LTE";
    //             break;
    //         case 8:
    //             state("coms") = "LTE";
    //             break;
    //         case 9:
    //             state("coms") = "E-UTRAN";
    //             break;
    //         case 10:
    //             state("coms") = "UTRAN/LTE";
    //             break;
    //         default:
    //             state("coms") = "Unknown";
    //             break;
    //         }
    //     }
    //     // enum com_DevicePropertyString
    //     if (!com_GetDevicePropertyString(COM_PROP_GSM_PROVIDER_NAME, simName, sizeof(simName), NULL)) {
    //         state("sim") = simName;
    //     }

    //     if (!com_GetDevicePropertyString(COM_PROP_GSM_SIM_ID, simID, sizeof(simID), NULL)) {
    //         state("simID") = simID;
    //     }

    //     if (!com_GetDevicePropertyString(COM_PROP_GSM_IMSI, imsi, sizeof(imsi), NULL)) {
    //         state("imsi") = imsi;
    //     }

    //     // "cid:12,lac:134,mcc:758,mnc:9558,ss:89", COM_PROP_GSM_MCC, COM_PROP_GSM_MNC, COM_PROP_GSM_LAC
    //     if (!com_GetDevicePropertyString(COM_PROP_GSM_CELL_ID, cellinfo, sizeof(cellinfo), NULL)) {
    //         // char cloc[256] = { 0 };
    //         char mcc[16] = { 0 };
    //         char mnc[16] = { 0 };
    //         char lac[16] = { 0 };
    //         char ss[4] = { 0 };
    //         int ss_percent = 0;
    //         int ss_dbm = 0;
    //         char dbmStr[8] = { 0 };
    //         iisys::JSObject cloc;

    //         // COM_PROP_WLAN_SIGNAL_PERCENTAGE
    //         com_GetDevicePropertyInt(COM_PROP_GSM_SIGNAL_PERCENTAGE, &ss_percent, NULL);

    //         com_GetDevicePropertyString(COM_PROP_GSM_LAC, lac, sizeof(lac), NULL);
    //         com_GetDevicePropertyString(COM_PROP_GSM_MCC, mcc, sizeof(mcc), NULL);
    //         com_GetDevicePropertyString(COM_PROP_GSM_MNC, mnc, sizeof(mnc), NULL);
    //         com_GetDevicePropertyString(COM_PROP_GSM_MNC, mnc, sizeof(mnc), NULL);
    //         com_GetDevicePropertyInt(COM_PROP_GSM_SIGNAL_DBM, &ss_dbm, NULL);

    //         // snprintf(cloc, sizeof(cloc), "{cid:%s,lac:%s,mcc:%s,mnc:%s,ss:%ddbm}", cellinfo, lac, mcc, mnc, ss_dbm);
    //         sprintf(dbmStr, "%02ddbm", ss_dbm);
    //         cloc("cid") = cellinfo;
    //         cloc("lac") = lac;
    //         cloc("mcc") = mcc;
    //         cloc("mnc") = mnc;
    //         cloc("ss") = dbmStr;

    //         snprintf(ss, sizeof(ss), "%02d", ss_percent);

    //         state("ss") = ss;
    //         state("cloc") = cloc;
    //     }
    // } else if (!strcmp(interface.c_str(), config::WLAN0)) {
    //     char ss[4] = {0};
    //     int ss_percent = 0;
    //     char ssid[128] = {0};

    //     com_GetDevicePropertyInt(COM_PROP_WLAN_SIGNAL_PERCENTAGE, &ss_percent, NULL);
    //     com_GetDevicePropertyString(COM_PROP_WLAN_SSID, ssid, sizeof(ssid), NULL);

    //     snprintf(ss, sizeof(ss), "%02d", ss_percent);
    //     state("coms") = "WiFi";
    //     state("ss") = ss;
    //     state("ssid") = ssid;
    // }  else if (!strcmp(interface.c_str(), config::ETH0)) {
    //     char ss[4] = {0};
    //     int ss_percent = 0;
    //     char ssid[128] = {0};

    //     state("coms") = "LAN";
    //     state("ss") = ss;
    //     state("ssid") = ssid;
    // }

    // if (!sysGetPropertyString(vfisysinfo::SYS_PROP_HW_MODEL_NAME, posModel, sizeof(posModel))) {
    //     state("tmn") = posModel;
    // }

    // state("tmanu") = "Verifone";

    // //hb
    // if (!sysGetPropertyInt(vfisysinfo::SYS_PROP_BATTERY_STATUS_OK, has_battery)) {
    //     switch (has_battery) {
    //     case 0:
    //         state("hb") = "false";
    //         break;
    //     case 1:
    //         state("hb") = "true";
    //         break;
    //     default:
    //         state("hb") = "Unknown";
    //         break;
    //     }
    // }

    // state("sv") = applicationInfo();

    // if (EmvDB::lastTransactionTime(lTxnAt) == 0) {
    //     state("lTxnAt") = lTxnAt.c_str();
    // } else {
    //     state("lTxnAt") = "";
    // }

    // state("pads") = "";

    return state;
}

iisys::JSObject getJournal(const Eft* trxContext)
{
    iisys::JSObject journal;
    // Merchant merchant;
    // std::map<std::string, std::string> trx;

    // EmvDB::trxContextToMap(trx, trxContext);
    
    // journal("mid") = merchant.object(config::MID);
    // journal("stan") = trx[DB_STAN];


    // journal("mPan") = trx[DB_PAN];
    // journal("cardName") = trx[DB_NAME];
    // journal("expiryDate") = trx[DB_EXPDATE];
    // journal("rrn") = trx[DB_RRN];
    // // journal("acode") = trx[DB_E];
    // journal("amount") = trx[DB_AMOUNT];
    // journal("timestamp") = trx[DB_DATE];
    // journal("mti") = trx[DB_MTI];
    // journal("ps") = trx[DB_PS];
    // // journal("resp") = trx[DB_PS]; 
    // journal("merchantName") = merchant.object(config::NAME);
    // journal("mcc") = merchant.object(config::MCC);
    // journal("vm") = onlinePinEntered(trxContext) ? "OnlinePin" : "OfflinePin";

    // journal("ps") = trx[DB_PS];
    
    return journal;
}

short getStateStr(char* strState)
{
    iisys::JSObject state = getState();
    
    strcpy(strState, state.dump().c_str());

    return 0;
}
