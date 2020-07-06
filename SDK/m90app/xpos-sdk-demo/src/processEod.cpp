#include <stdio.h>
#include <string.h>
#include <algorithm>

#include "EftDbImpl.h"
#include "EmvDBUtil.h"
#include "Receipt.h"
#include "processEod.h"

extern "C"{
#include "libapi_xpos/inc/libapi_util.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_print.h"
#include "remoteLogo.h"
#include "util.h"
}


ProcessEod::ProcessEod(const std::vector<std::map<std::string, std::string> > &record, const std::string &db, const std::string &file)
    :
    totalNumberOfApproved(0),
    totalNumberOfDeclined(0),
    totalNumberOfTransactions(0),
    totalSumOfApprovedInNaira(0),
    totalSumOfDeclinedInNaira(0), 
    data(record),
    dbName(db),
    dbFile(file),
    labelList(std::vector<EodLabel>())
{

}

void ProcessEod::generateEodReceipt(bool isRRN)
{
    EmvDB db(dbFile, dbName);
    std::string filename = "xxxx\\"; 
    char buff[100] = {'\0'};

    filename.append(BANKLOGO);

    if(data.empty()) return;
    
    for(int index = 0; index < data.size(); index++){

        EodLabel label;
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

        if(data[index][DB_RESP] == "00") {

            totalNumberOfApproved += 1;
            totalSumOfApprovedInNaira = totalSumOfApprovedInNaira + atol(label.price) / 100.0;
        } else {

            totalNumberOfDeclined += 1;
            totalSumOfDeclinedInNaira = totalSumOfDeclinedInNaira + atol(label.price) / 100.0;
        }
        
        labelList.push_back(label);
    }
    
    gui_begin_batch_paint();			
    gui_clear_dc();
    gui_text_out(0, GUI_LINE_TOP(0), "printing...");
    gui_end_batch_paint();

    while(1) {

        int ret = UPrint_Init();

        if (ret == UPRN_OUTOF_PAPER) {
            if(gui_messagebox_show("Print", "No paper \nDo you wish to reload Paper?", "cancel", "confirm", 0) != 1) {
                break;
            }
        }

        printReceiptLogo(filename.c_str());
        generateEodHeader();
        UPrint_SetFont(7, 2, 2);
        UPrint_StrBold("EOD SUMMARY", 1, 0,1 );
        UPrint_SetFont(8, 2, 2);
        UPrint_Str(DOUBLELINE, 2, 1);

        memset(buff, 0x00, sizeof(buff));
        sprintf(buff,"NGN %.2f",  totalSumOfApprovedInNaira);
        printLine("APPROVED", buff);

        memset(buff, 0x00, sizeof(buff));
        sprintf(buff, "NGN %.2f",  totalSumOfDeclinedInNaira);
        printLine("DECLINED", buff);

        memset(buff, 0x00, sizeof(buff));
        sprintf(buff, "%d", totalNumberOfApproved);
        printLine("APPROVED TX", buff);

        memset(buff, 0x00, sizeof(buff));
        sprintf(buff, "%d", totalNumberOfDeclined);
        printLine("DECLINED TX", buff);

        memset(buff, 0x00, sizeof(buff));
        sprintf(buff, "%d", totalNumberOfTransactions);
        printLine("TOTAL", buff);
        UPrint_Feed(12);

        UPrint_SetFont(8, 2, 2);
        for(int index = 0; index < labelList.size(); index++) {

            memset(buff, 0x00, sizeof(buff));
            if(labelList[index].rrnOrPanStr.length() > 16) {
                sprintf(buff, "%s %s %.2f %s\n", labelList[index].time, labelList[index].rrnOrPanStr.c_str(), strtoul(labelList[index].price, NULL, 10)/100.0,labelList[index].flag );
            } else {
                sprintf(buff, "%s %s %.2f %s\n", labelList[index].time, labelList[index].rrnOrPanStr.c_str(), strtoul(labelList[index].price, NULL,10)/100.0, labelList[index].flag );
            }
            
            UPrint_Str(buff, 1, 0);
            UPrint_Str(DOTTEDLINE, 2, 1);
            
        }

        printFooter();
        ret = UPrint_Start();
        if(getPrinterStatus(ret) < 0) {
            break;
        }
        data.clear();
    }

    return;
}

void ProcessEod::generateEodHeader()
{
    MerchantData merchantdata = { 0 };
    readMerchantData(&merchantdata);
    
    char date[50] = {0};
    getDate(date);
    
    UPrint_Feed(8);
    UPrint_SetFont(8, 2, 2);
    UPrint_StrBold(merchantdata.name, 1, 0, 1);
    UPrint_StrBold(merchantdata.address, 1, 0, 1);
    printLine(merchantdata.tid, date);

    UPrint_Feed(8);
}
