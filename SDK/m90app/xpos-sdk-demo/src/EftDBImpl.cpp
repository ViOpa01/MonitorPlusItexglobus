/**
 * File: EftDBImpl.cpp
 * -------------------
 * Implements EftDBImpl.h's interface.
 */

#include <stdio.h>
#include <string.h>
#include "EftDbImpl.h"
#include "EmvDBUtil.h"
#include "Nibss8583.h"
#include "appInfo.h"
#include "util.h"

extern "C"{
#include "libapi_xpos/inc/libapi_util.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_print.h"
}
#include "Receipt.h"
#include "merchant.h"

#define DOTTEDLINE		"--------------------------------"
#define DOUBLELINE		"================================"

const  std::string DBNAME = "emvdb.db";
#define EFT_DEFAULT_TABLE "Transactions"
#define EMVDBUTILLOG "EMVDBUTILLOG"

static void printLine(char *head, char *val)
{
	UPrint_SetFont(8, 2, 2);
	UPrint_Str(head, 1, 0);
	UPrint_Str(val, 1, 1);
}

void printHeader(){
    MerchantData merchantdata = { 0 };
    readMerchantData(&merchantdata);
    std::string merchantAddress(merchantdata.address);
    std::string merchantName;
    std::string realAddress;
    
    char address_pr[100] = {0};
    char name_pr[100] = {0};
    char tid[100] = {0};
    char date[50] = {0};
    char tidWithDate[100] = {0};
    int index = merchantAddress.find('|',0);
    getDate(date);
    merchantName = merchantAddress.substr(0, merchantAddress.length() - index + 1);
    realAddress = merchantAddress.substr(index, merchantAddress.length() - index);
    sprintf(address_pr, "%s\n", realAddress.c_str());
    sprintf(name_pr,"%s\n", merchantName.c_str());
    sprintf(tid,"%s", merchantdata.tid);
    UPrint_Feed(8);
    sprintf(tidWithDate, "%s              %s\n", tid, date);
    UPrint_SetFont(8, 2, 2);
    UPrint_StrBold(name_pr, 1, 0, 1);
    UPrint_StrBold(address_pr, 1, 0, 1);

  //  UPrint_Str(name_pr, 1, 0);
 	//UPrint_Str(address_pr, 1, 0);
   UPrint_Str(tidWithDate, 1, 0);
   UPrint_Feed(8);
    //UPrint_Str(tid, 7, 0);
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
        
        printBankLogo();
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

    std::map<std::string, std::string> dbmap;

    if (db.selectTransactionsByRef(vec_map, eft->rrn, ALL_TRX_TYPES)) {
        return -1;
    }

    if(vec_map.empty()) {
        printf("Ref doesnt exist\n");
        return -1;
    }
    
    dbmap = vec_map[0];

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), sizeof(eft->pan));
    strncpy(eft->aid, dbmap[DB_AID].c_str(), sizeof(eft->aid));
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), sizeof(eft->additionalAmount));
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), sizeof(eft->authorizationCode));
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), sizeof(eft->cardHolderName));
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), sizeof(eft->cardLabel));
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), sizeof(eft->responseCode));
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), sizeof(eft->stan));
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), sizeof(eft->rrn));
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), sizeof(eft->tvr));
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), sizeof(eft->tsi));
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), sizeof(eft->originalMti));
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), sizeof(eft->originalMti));
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), sizeof(eft->expiryDate));

    sprintf(eft->amount, "%012li", atol(dbmap[DB_AMOUNT].c_str()));
    normalizeDateTime(eft->yyyymmddhhmmss, dbmap[DB_DATE].c_str());
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, sizeof(eft->originalYyyymmddhhmmss));

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
    
    std::map<std::string, std::string> dbmap;
    eft->atPrimaryIndex = db.lastTransactionId();

    if(eft->atPrimaryIndex <= 0) {
        return -1;
    }

    printf("Last Primary index : %d\n", eft->atPrimaryIndex);

    db.select(dbmap, eft->atPrimaryIndex);

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), sizeof(eft->pan));
    strncpy(eft->aid, dbmap[DB_AID].c_str(), sizeof(eft->aid));
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), sizeof(eft->additionalAmount));
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), sizeof(eft->authorizationCode));
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), sizeof(eft->cardHolderName));
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), sizeof(eft->cardLabel));
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), sizeof(eft->responseCode));
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), sizeof(eft->stan));
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), sizeof(eft->rrn));
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), sizeof(eft->tvr));
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), sizeof(eft->tsi));
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), sizeof(eft->originalMti));
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), sizeof(eft->originalMti));
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), sizeof(eft->expiryDate));

    sprintf(eft->amount, "%012li", atol(dbmap[DB_AMOUNT].c_str()));
    normalizeDateTime(eft->yyyymmddhhmmss, dbmap[DB_DATE].c_str());
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, sizeof(eft->originalYyyymmddhhmmss));

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
    const char * C_dateList[250] = { 0 };
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

    strncpy(eft->pan, dbmap[DB_PAN].c_str(), sizeof(eft->pan));
    strncpy(eft->aid, dbmap[DB_AID].c_str(), sizeof(eft->aid));
    strncpy(eft->additionalAmount, dbmap[DB_ADDITIONAL_AMOUNT].c_str(), sizeof(eft->additionalAmount));
    strncpy(eft->authorizationCode, dbmap[DB_AUTHID].c_str(), sizeof(eft->authorizationCode));
    strncpy(eft->cardHolderName, dbmap[DB_NAME].c_str(), sizeof(eft->cardHolderName));
    strncpy(eft->cardLabel, dbmap[DB_LABEL].c_str(), sizeof(eft->cardLabel));
    strncpy(eft->responseCode, dbmap[DB_RESP].c_str(), sizeof(eft->responseCode));
    strncpy(eft->stan, dbmap[DB_STAN].c_str(), sizeof(eft->stan));
    strncpy(eft->rrn, dbmap[DB_RRN].c_str(), sizeof(eft->rrn));
    strncpy(eft->tvr, dbmap[DB_TVR].c_str(), sizeof(eft->tvr));
    strncpy(eft->tsi, dbmap[DB_TSI].c_str(), sizeof(eft->tsi));
    strncpy(eft->originalMti, dbmap[DB_MTI].c_str(), sizeof(eft->originalMti));
    strncpy(eft->forwardingInstitutionIdCode, dbmap[DB_FISC].c_str(), sizeof(eft->originalMti));
    strncpy(eft->expiryDate, dbmap[DB_EXPDATE].c_str(), sizeof(eft->expiryDate));

    sprintf(eft->amount, "%012li", atol(dbmap[DB_AMOUNT].c_str()));
    strcpy(eft->yyyymmddhhmmss, dbmap[DB_DATE].c_str());
    strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, sizeof(eft->originalYyyymmddhhmmss));

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
    char normallizedDate[30] = {0};
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
    
	char d[32] = {0};
    strcpy(d, eft.yyyymmddhhmmss);
	
    strncpy(normallizedDate, d, 10);
    memset(&eft, '\0', sizeof(eft));
    contextMapToEft(transactions[selectedOption], &eft);
    strncpy(normallizedDate, eft.originalYyyymmddhhmmss, 10);
    sprintf(message, "AMOUNT: NGN %.2f\nPAN: %s\nDATE:%s\n", (atol(eft.amount)/100.00), eft.pan, normallizedDate);
    
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