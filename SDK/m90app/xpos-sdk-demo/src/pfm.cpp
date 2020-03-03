

#include <errno.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfm.h"
#include "EmvDBUtil.h"
#include "merchant.h"


extern "C" {
#include "nibss.h"
#include "Nibss8583.h"
#include "util.h"
#include "appInfo.h"
}

iisys::JSObject getState()
{
    MerchantParameters parameter = {'\0'};
    MerchantData mParam = {'\0'};
    Network netProfile = {'\0'};
    
    char dateAndTime[24] = {'\0'};
    char simID[32] = {0};
    char imsi[32] = {0};
    char mcc[4] = {'\0'};
    char mnc[3] = {'\0'};
    char softwareVersion[64] = {'\0'};
    char mid[20] = {'\0'};
    char tid[10] = {'\0'};
    char terminalSn[24] = {'\0'};
    char cellId[12] = {'\0'};
    int signalLevel = 0;
    char lac[12] = {'\0'};
    char ss[4] = {'\0'};
   

    getParameters(&parameter);
    readMerchantData(&mParam);
    getTerminalSn(terminalSn);
   
    strncpy(mid, parameter.cardAcceptiorIdentificationCode, sizeof(mid));
    strncpy(tid, mParam.tid, 8);
    getFormattedDateTime(dateAndTime, sizeof(dateAndTime));
    sprintf(softwareVersion, "TAMSLITE %s Built for %s", APP_VER, mParam.platform_label);   // "TAMSLITE v(1.0.6)Built for POSVAS onFri Dec 20 10:50:14 2019"
    sprintf(cellId, "%d", getCellId());
    sprintf(lac, "%d", getLocationAreaCode());
    sprintf(simID, "%s", getSimId());
    getImsi(imsi);
    getMcc(mcc);
    getMnc(mnc);
    imsiToNetProfile(&netProfile, imsi);
    
    signalLevel = getSignalLevel();
    signalLevel *=  25;
    sprintf(ss, "%d", signalLevel);


    iisys::JSObject state;
    iisys::JSObject cloc;

    state("ptad") = "Itex Integrated Services";
    state("sv") = "7.8.18";     // Register the Morefun version later
    // state("sv") = softwareVersion;
    /*  // Need to know why cash payment method is returning request not authorized with cash payment method
    state("serial") = terminalSn;
    state("bl") = "100";
    state("btemp") = "35";
    state("ctime") = dateAndTime;
    state("cs") = "Charging";
    state("ps") = "PrinterAvailable";
    state("tid") = tid;
    state("mid") = mid;
    state("coms") = "GSM/UMTSDualMode";
    state("sim") = netProfile.operatorName;
    state("simID") = simID;
    state("imsi") = imsi;
    state("ss") = ss;
    state("tmanu") = "Morefun";
    state("tmn") = APP_MODEL;
    state("hb") = "true";
    state("pads") = "";
    // state("ssid") = "";
    state("lTxnAt") = "";

    cloc("cid") = cellId;
    cloc("lac") = lac;
    cloc("mcc") = mcc;
    cloc("mnc") = mnc;
    cloc("ss") = "-87dbm";

    state("cloc") = cloc; 
    */

    return state;
}

iisys::JSObject getJournal(const Eft* trxContext)
{
    MerchantParameters parameter = {'\0'};
    iisys::JSObject journal;
    // Merchant merchant;
    std::map<std::string, std::string> trx;

    ctxToInsertMap(trx, trxContext);
    
    getParameters(&parameter);
    journal("mid") = parameter.cardAcceptiorIdentificationCode;//merchant.object(config::MID);
    journal("stan") = trx[DB_STAN];


    journal("mPan") = trx[DB_PAN];
    journal("cardName") = trx[DB_NAME];
    journal("expiryDate") = trx[DB_EXPDATE];
    journal("rrn") = trx[DB_RRN];
    // journal("acode") = trx[DB_E];
    journal("amount") = trx[DB_AMOUNT];
    journal("timestamp") = trx[DB_DATE];
    journal("mti") = trx[DB_MTI];
    journal("ps") = trx[DB_PS];
    // journal("resp") = trx[DB_PS]; 
    journal("merchantName") = parameter.merchantNameAndLocation; /*merchant.object(config::NAME);*/
    journal("mcc") = parameter.currencyCode;/*merchant.object(config::MCC);*/
    journal("vm") = trxContext->pinDataBcdLen ? "OnlinePin" : "OfflinePin";

    journal("ps") = trx[DB_PS];
    
    return journal;
}

short getStateStr(char* strState)
{
    iisys::JSObject state = getState();
    
    strcpy(strState, state.dump().c_str());

    return 0;
}
