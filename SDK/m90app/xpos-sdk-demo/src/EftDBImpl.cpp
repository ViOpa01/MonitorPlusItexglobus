/**
 * File: EftDBImpl.cpp
 * -------------------
 * Implements EftDBImpl.h's interface.
 */

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "EftDbImpl.h"
#include "EmvDBUtil.h"
#include "Nibss8583.h"
#include "appInfo.h"
#include "util.h"

extern "C"{
#include "libapi_xpos/inc/libapi_util.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_print.h"
#include "remoteLogo.h"
}
#include "Receipt.h"
#include "merchant.h"

#define DOTTEDLINE		"--------------------------------"
#define DOUBLELINE		"================================"

const  std::string DBNAME = "itex/emvdb.db";    // To be in sync with system init private directory
#define EFT_DEFAULT_TABLE "Transactions"
#define EMVDBUTILLOG "EMVDBUTILLOG"


void printHeader(){
    MerchantData merchantdata = { 0 };
    readMerchantData(&merchantdata);
    
    char tid[100] = {0};
    char date[50] = {0};
    char tidWithDate[100] = {0};
    getDate(date);
    
    sprintf(tid,"%s", merchantdata.tid);
    UPrint_Feed(8);
    sprintf(tidWithDate, "%s              %s\n", tid, date);
    UPrint_SetFont(8, 2, 2);
    UPrint_StrBold(merchantdata.name, 1, 0, 1);
    UPrint_StrBold(merchantdata.address, 1, 0, 1);

    UPrint_Str(tidWithDate, 1, 0);
    UPrint_Feed(8);
    
}

struct EodLabelStruct {

    char time[6];
    std::string rrnOrPanStr;
    char flag[2];
    char price[13];
};


struct EodStruct {
    
private:
    int totalNumberOfApproved;
    int totalNumberOfTransactions;
    int totalNumberOfDeclined;
    float totalSumOfDeclinedInNaira;
    float totalSumOfApprovedInNaira;
    std::vector<std::map<std::string, std::string> > data;
    std::vector<EodLabelStruct> labelList;

public:
    EodStruct(std::vector<std::map<std::string, std::string> > data_)
    :
    totalNumberOfApproved(0),
    totalNumberOfDeclined(0),
    totalNumberOfTransactions(0),
    totalSumOfApprovedInNaira(0),
    totalSumOfDeclinedInNaira(0), 
    data(data_),
    labelList(std::vector<EodLabelStruct>())
    {

    }

    void convertData(bool isRRN){
        EmvDB db(EFT_DEFAULT_TABLE, DBNAME);
        std::string filename = "xxxx\\bank.bmp"; // + BANKLOGO;

        if(data.empty()) return;
       
     
        for(int index = 0; index < data.size(); index++){

            EodLabelStruct label;
            label.rrnOrPanStr.clear();

            if(isRRN == true){
                label.rrnOrPanStr.assign(data[index][DB_RRN]);
            } else {
                label.rrnOrPanStr.assign(data[index][DB_PAN]);
            }   

            if(data[index][DB_RESP] != "00"){
                sprintf(label.flag, "%s", "D"); 
            }
            else{
                sprintf(label.flag, "%s", "A"); 
            }

            strcpy(label.price,  data[index][DB_AMOUNT].c_str());
            printf("\n%s\n", label.price);

            char dateInDB[30] = {0};
            strcpy(dateInDB, data[index][DB_DATE].c_str());
            printf("\n%s\n", dateInDB);

            char  timeExtract[20] = { 0 };
            strncpy(timeExtract, &dateInDB[11], 5);
            printf("\n%s\n",timeExtract);

            strcpy(label.time, timeExtract);
            printf("\n%s\n", label.time);
            totalNumberOfTransactions = totalNumberOfTransactions + 1;

            if(data[index][DB_RESP] == "00")
            {
    
                totalNumberOfApproved += 1;
                totalSumOfApprovedInNaira = totalSumOfApprovedInNaira + atol(label.price) / 100.0;
            }
            else
            {

                totalNumberOfDeclined += 1;
                totalSumOfDeclinedInNaira = totalSumOfDeclinedInNaira + atol(label.price) / 100.0;
            }
            
            labelList.push_back(label);
        }

        printf("\nTotal num of transactions is %d\n", totalNumberOfTransactions );
        printf("\nSum of approved transactions in Naira %.2f\n", totalSumOfApprovedInNaira);
        printf("\nSum of declined transactions in Naira %.2f\n", totalSumOfDeclinedInNaira);
        printf("Starting to print\n");
        printf("Size of label list is %zu", labelList.size());
        
        printReceiptLogo(filename.c_str());
        printHeader();
        UPrint_SetFont(7, 2, 2);
        UPrint_StrBold("EOD SUMMARY", 1, 0,1 );
        UPrint_SetFont(8, 2, 2);
        UPrint_Str(DOUBLELINE, 2, 1);
        char printData[64] = {0};
        sprintf(printData,"APPROVED    NGN %.2f\n",  totalSumOfApprovedInNaira);
        UPrint_SetFont(8, 2, 2);
        UPrint_Str(printData, 1, 0);
        sprintf(printData,"DECLINED    NGN %.2f\n",  totalSumOfDeclinedInNaira);
        

        UPrint_Str(printData, 1, 0);
        sprintf(printData, "APPROVED TX    %d\n", totalNumberOfApproved);
        UPrint_Str(printData, 1, 0);
        sprintf(printData, "DECLINED TX    %d\n", totalNumberOfDeclined);
        UPrint_Str(printData, 1, 0);
        sprintf(printData, "TOTAL          %d\n", totalNumberOfTransactions);
        UPrint_Str(printData, 1, 0);
        UPrint_Feed(12);
        UPrint_SetFont(8, 2, 2);

        for(int index = 0; index < labelList.size(); index++){

            printf("Printing\n");
            printf("\nIs the Label %s\n", labelList[index].rrnOrPanStr.c_str());
            char buff[100] = { 0 };

            if(labelList[index].rrnOrPanStr.length() > 16){

                sprintf(buff, "%s %s %.2f %s\n", labelList[index].time, labelList[index].rrnOrPanStr.c_str(), strtoul(labelList[index].price, NULL, 10)/100.0,labelList[index].flag );
            
            } else {
                sprintf(buff, "%s %s %.2f %s\n", labelList[index].time, labelList[index].rrnOrPanStr.c_str(), strtoul(labelList[index].price, NULL,10)/100.0, labelList[index].flag );
            }
            
            UPrint_Str(buff, 1, 0);
            UPrint_Str(DOTTEDLINE, 2, 1);
            
        }

        printFooter();
        int  ret = UPrint_Start();
        getPrinterStatus(ret);
        data.clear();
        return;
    }
  
};


static short updateEftRequired(const Eft * eft) 
{
    if (!strncmp(eft->responseCode, "00", 2)) return 1;

    //transaction not to be updated when declined.
    if ((eft->transType == EFT_REVERSAL || eft->transType == EFT_COMPLETION || eft->transType == EFT_REFUND)) 
    {
        fprintf(stderr, "No need to update, transaction declined...\n");
        return 0;
    }

    return 1;
}

short updateEft(const Eft * eft)
{
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    std::map<std::string, std::string> dbmap;

    if (!updateEftRequired(eft)) {
        return -1;
    }

    if (ctxToUpdateMap(dbmap, eft)) {
        return -3;
    }

    if (db.updateTransaction(eft->atPrimaryIndex, dbmap)) {
        return -3;
    }

    return 0;
}

short saveEft(Eft *eft)
{
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    std::map<std::string, std::string> dbmap;

    if (ctxToInsertMap(dbmap, eft)) {
        return -1;
    }

    printf("Started Saving to DB\n");

    eft->atPrimaryIndex = db.insertTransaction(dbmap);

    if (eft->atPrimaryIndex < 0) {
        return -2;
    }

    printf("Successfully saved to DB\n");

    return 0;
}

short getEft(Eft * eft)
{
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    std::vector<std::map<std::string, std::string> > vec_map;
    std::string yyyymmddhhmmss;
    std::map<std::string, std::string> dbmap;

    if (db.selectTransactionsByRef(vec_map, eft->rrn, ALL_TRX_TYPES)) {
        return -1;
    }

    if(vec_map.empty()) {
        printf("Ref doesnt exist\n");
        return -1;
    }
    
    dbmap = vec_map[0];

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), dbmap[DB_PAN].length());
    strncpy(eft->aid, dbmap[DB_AID].c_str(), dbmap[DB_AID].length());
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), dbmap[DB_ADDITIONAL_AMOUNT].length());
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), dbmap[DB_AUTHID].length());
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), dbmap[DB_NAME].length());
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), dbmap[DB_LABEL].length());
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), dbmap[DB_RESP].length());
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), dbmap[DB_STAN].length());
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), dbmap[DB_RRN].length());
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), dbmap[DB_TVR].length());
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), dbmap[DB_TSI].length());
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), dbmap[DB_MTI].length());
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), dbmap[DB_FISC].length());
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), dbmap[DB_EXPDATE].length());
    strncpy(eft->dateAndTime, dbmap[DB_DATE].c_str(), dbmap[DB_DATE].length() - 4);

    sprintf(eft->amount, "%012li", atol(dbmap[DB_AMOUNT].c_str()));

    yyyymmddhhmmss = dbmap[DB_DATE].substr(0, dbmap[DB_DATE].find('.'));     // 
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), ' '), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), ':'), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), '-'), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), '.'), yyyymmddhhmmss.end());

    strncpy(eft->yyyymmddhhmmss, yyyymmddhhmmss.c_str(), strlen(yyyymmddhhmmss.c_str()));
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, strlen(eft->yyyymmddhhmmss));

    if (decodeProcessingCode(&eft->transType, &eft->fromAccount, &eft->toAccount, dbmap[DB_PS].c_str(), dbmap[DB_MTI].c_str())) {
        fprintf(stderr, "Error decoding processing code...\n");
        return -2;
    }

    eft->atPrimaryIndex = atoll(dbmap[DB_ID].c_str());

    return 0;
}

short getLastTransaction(Eft * eft)
{
    EmvDB db(*eft->tableName ? eft->tableName : EFT_DEFAULT_TABLE,  *eft->dbName ? eft->dbName : DBNAME);
    std::string yyyymmddhhmmss;
    std::map<std::string, std::string> dbmap;
    eft->atPrimaryIndex = db.lastTransactionId();

    if(eft->atPrimaryIndex <= 0) {
        return -1;
    }

    printf("Last Primary index : %d\n", eft->atPrimaryIndex);

    db.select(dbmap, eft->atPrimaryIndex);

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), dbmap[DB_PAN].length());
    strncpy(eft->aid, dbmap[DB_AID].c_str(), dbmap[DB_AID].length());
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), dbmap[DB_ADDITIONAL_AMOUNT].length());
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), dbmap[DB_AUTHID].length());
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), dbmap[DB_NAME].length());
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), dbmap[DB_LABEL].length());
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), dbmap[DB_RESP].length());
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), dbmap[DB_STAN].length());
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), dbmap[DB_RRN].length());
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), dbmap[DB_TVR].length());
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), dbmap[DB_TSI].length());
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), dbmap[DB_MTI].length());
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), dbmap[DB_FISC].length());
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), dbmap[DB_EXPDATE].length());
    strncpy(eft->dateAndTime, dbmap[DB_DATE].c_str(), dbmap[DB_DATE].length() - 4);

    sprintf(eft->amount, "%012li", atol(dbmap[DB_AMOUNT].c_str()));

    yyyymmddhhmmss = dbmap[DB_DATE].substr(0, dbmap[DB_DATE].find('.'));     // 
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), ' '), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), ':'), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), '-'), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), '.'), yyyymmddhhmmss.end());

    strncpy(eft->yyyymmddhhmmss, yyyymmddhhmmss.c_str(), strlen(yyyymmddhhmmss.c_str()));
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, strlen(eft->yyyymmddhhmmss));

    if (decodeProcessingCode(&eft->transType, &eft->fromAccount, &eft->toAccount, dbmap[DB_PS].c_str(), dbmap[DB_MTI].c_str())) {
        fprintf(stderr, "Error decoding processing code...\n");
        return -2;
    }

    return 0;
}


void eodSubMenuHandler(int selected, const char** dateList, TrxType txtype) 
{

    EmvDB db(EFT_DEFAULT_TABLE, DBNAME);
    std::vector<std::map<std::string, std::string> > transactions;
    
    char * menu[] = {
        "By RRN",
        "By PAN"
    };

    if(!dateList){
        return;
    }
    
    db.selectTransactionsOnDate(transactions, dateList[selected],  txtype );
    
    if(transactions.empty()){
        return;
    }

    printf("Size of the data is %zu", transactions.size());
    EodStruct eods(transactions);

    printf("starting operation\n");
    int selectedOption = gui_select_page_ex("Print By?", menu, 2, 10000, 0);

    if(selectedOption == 0){
        eods.convertData(true);
    }
    else if(selectedOption == 1 ){
        eods.convertData(false);
        return;
    }
    else
    {
        return;
    }
    
    return;
}

void getListOfEod(Eft * eft, TrxType txtype)
{
    EmvDB db(EFT_DEFAULT_TABLE, DBNAME);
    MerchantData merchant;
    readMerchantData(&merchant);

    if(merchant.is_prepped == 0){
        gui_messagebox_show("MESSAGE", "Please Prep Terminal first", "", "Ok", 0);
        return;
    }

    std::vector<std::string> dateList;
    int selectedOption = 0;
    int index;
    size_t dateListLenght;

    db.selectUniqueDates(dateList, txtype);
    dateListLenght = dateList.size();

    if (dateListLenght == 0)
    {
        gui_messagebox_show("MESSAGE", "No record for this transaction type", "", "Ok", 0);
        return;
    }

    const char * C_dateList[dateListLenght] = { 0 };

    for(index = 0; index < dateList.size(); index++){
        printf("\n%s\n", dateList[index].c_str());
        C_dateList[index] = dateList[index].c_str();
    }

    selectedOption = gui_select_page_ex("EOD",(char **)C_dateList, dateListLenght , 10000, 0);
    if(selectedOption > index || selectedOption < 0){
        return;
    }

    eodSubMenuHandler(selectedOption, C_dateList, txtype);
    printf("size of vector is %zu\n", dateListLenght);

    return;

}

void contextMapToEft(std::map<std::string, std::string> dbmap ,  Eft * eft){
    
    if(dbmap.empty()){
        return;
    }

    std::string yyyymmddhhmmss;

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), dbmap[DB_PAN].length());
    strncpy(eft->aid, dbmap[DB_AID].c_str(), dbmap[DB_AID].length());
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), dbmap[DB_ADDITIONAL_AMOUNT].length());
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), dbmap[DB_AUTHID].length());
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), dbmap[DB_NAME].length());
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), dbmap[DB_LABEL].length());
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), dbmap[DB_RESP].length());
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), dbmap[DB_STAN].length());
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), dbmap[DB_RRN].length());
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), dbmap[DB_TVR].length());
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), dbmap[DB_TSI].length());
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), dbmap[DB_MTI].length());
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), dbmap[DB_FISC].length());
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), dbmap[DB_EXPDATE].length());
    sprintf(eft->amount, "%012li", atol(dbmap[DB_AMOUNT].c_str()));
    strncpy(eft->dateAndTime, dbmap[DB_DATE].c_str(), dbmap[DB_DATE].length() - 4);
    
    yyyymmddhhmmss = dbmap[DB_DATE].substr(0, dbmap[DB_DATE].find('.'));     // 
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), ' '), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), ':'), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), '-'), yyyymmddhhmmss.end());
    yyyymmddhhmmss.erase(std::remove(yyyymmddhhmmss.begin(), yyyymmddhhmmss.end(), '.'), yyyymmddhhmmss.end());

    strncpy(eft->yyyymmddhhmmss, yyyymmddhhmmss.c_str(), strlen(yyyymmddhhmmss.c_str()));
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, strlen(eft->yyyymmddhhmmss));
    printf("originalyyyymmddhhmmss : %s\n", eft->originalYyyymmddhhmmss);

    if (decodeProcessingCode(&eft->transType, &eft->fromAccount, &eft->toAccount, dbmap[DB_PS].c_str(), dbmap[DB_MTI].c_str())) {
        fprintf(stderr, "Error decoding processing code...\n");
        return ;
    }

    eft->atPrimaryIndex = atoll(dbmap[DB_ID].c_str());
    
}

void getListofEftToday(){

    EmvDB db(EFT_DEFAULT_TABLE, DBNAME);
    std::vector<std::map<std::string, std::string> > transactions;
    char ** menulist;
    char todayDate[20] = { 0 };
    int numberOfTrans;
    int index;
    int selectedOption;
    char message[1024] = {0};
    Eft eft;
    

    getDate(todayDate);
    db.selectTransactionsOnDate(transactions, todayDate, ALL_TRX_TYPES);
    
    if(transactions.empty()){
        return;
    }
    numberOfTrans = transactions.size();
    menulist = (char**) malloc(numberOfTrans * sizeof(char*));

    printf("number of transactions is %d\n",numberOfTrans);
    
    for(index = 0; index < numberOfTrans; index++){

        printf("converting of Eod\n");
        menulist[index] =  (char*)malloc(30 * sizeof(char));
        memset(&eft, '\0', sizeof(eft));
        contextMapToEft(transactions[index], &eft);
        strcpy(menulist[index], eft.rrn);
        
    }

    selectedOption = gui_select_page_ex("Select RRN", menulist, numberOfTrans, 10000, 0);
    
    if(selectedOption > index || selectedOption < 0){
        return;
    }
    

    memset(&eft, '\0', sizeof(eft));
    contextMapToEft(transactions[selectedOption], &eft);
    sprintf(message, "AMOUNT: NGN %.2f\nPAN: %s\nDATE:%.10s\n", (atol(eft.amount)/100.00), eft.pan, eft.dateAndTime);
    
    if (gui_messagebox_show("PRINT ?", message , "No", "Yes", 0) == 1)
    {  
        printReceipts(&eft, 1);
        
    }

	for(index = 0; index < numberOfTrans; index++)
    {
        free(menulist[index]);
    }

    free(menulist);
}

int clearDb() {
    EmvDB db(EFT_DEFAULT_TABLE, DBNAME);

    if (gui_messagebox_show("Notification", "Do you wish to print transaction record?" , "No", "Yes", 0) == 1)
    {
       return 1; 
        
    }
    return db.clear();
}
