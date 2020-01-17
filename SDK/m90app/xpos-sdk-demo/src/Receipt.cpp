
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>

#include "Receipt.h"
#include "appInfo.h"

extern "C" {
#include "Nibss8583.h"
#include "nibss.h"
#include "util.h"
#include "logo.h"
#include "itexFile.h"
#include "libapi_xpos/inc/libapi_print.h"
#include "libapi_xpos/inc/libapi_file.h"
#include "libapi_xpos/inc/libapi_gui.h"

}

#ifdef WIN32
#define ATOLL _atoi64
#else
#define ATOLL atoll
#endif

#define DOTTEDLINE		"--------------------------------"
#define DOUBLELINE		"================================"
#define LOGOIMG "logo.bmp"

//Add a line of print data
static void printLine(char *head, char *val)
{
	UPrint_SetFont(8, 2, 2);
	UPrint_Str(head, 1, 0);
	UPrint_Str(val, 1, 1);
}

static void printDottedLine()
{
    UPrint_SetFont(8, 2, 2);
    UPrint_Str(DOTTEDLINE, 2, 1);
}

static void printFooter()
{
	char buff[64] = {'\0'};

	sprintf(buff, "%s %s", APP_NAME, APP_VER);
    UPrint_StrBold(buff, 1, 4, 1);
    UPrint_StrBold("iisysgroup.com", 1, 4, 1);
	// If something else to be printed

	UPrint_Feed(108);
}

static int formatAmount(std::string& ulAmount)
{
    int position;

    if (ulAmount.empty()) return -1;

    position = ulAmount.length();
    if ((position -= 2) > 0) {
        ulAmount.insert(position, ".");
    } else if (position == 0) {
        ulAmount.insert(0, "0.");
    } else {
        ulAmount.insert(0, "0.0");
    }

    while ((position -= 3) > 0) {
        ulAmount.insert(position, ",");
    }

    return 0;
}

static void getPrinterStatus(const int status)
{
    if (status == UPRN_SUCCESS)
	{
		gui_messagebox_show("Print", "Print Success", "", "confirm", 0);
	}
	else if (status == UPRN_OUTOF_PAPER)
	{
		gui_messagebox_show("Print", "No paper", "", "confirm", 0);
	}
	else if (status == UPRN_FILE_FAIL)
	{
		gui_messagebox_show("Print", "Open File Fail", "", "confirm", 0);
	}
	else if (status == UPRN_DEV_FAIL)
	{
		gui_messagebox_show("Print", "Printer device failure", "", "confirm", 0);
	}
	else
	{
		gui_messagebox_show("Print", "Printer unknown fault", "", "confirm", 0);
	}
}

static void creat_bmp()
{
	int ret = 0;
	int fp;
	ret = UFile_OpenCreate(LOGOIMG, FILE_PRIVATE, FILE_CREAT, &fp, 0);//File open / create
	if( ret == UFILE_SUCCESS){
		UFile_Write(fp, Skye, sizeof(Skye));
		UFile_Close(fp);
	}
	else{
		gui_messagebox_show( "PrintTest" , "File open or create fail" , "" , "confirm" , 0);
	}
}

int printEftReceipt(Eft *eft)
{
	int ret = 0;
	char dt[14] = {'\0'};
	char buff[64] = {'\0'};
	char maskedPan[25] = {'\0'};
    MerchantParameters parameter = {'\0'};

    getParameters(&parameter);
    getDateAndTime(dt);
    sprintf(buff, "%.2s-%.2s-%.2s / %.2s:%.2s", &dt[2], &dt[4], &dt[6], &dt[8], &dt[10]);


	printf("Starting Print Routine\n");
	ret = UPrint_Init();

	if (ret == UPRN_OUTOF_PAPER)
	{
		gui_messagebox_show("Print", "No paper", "", "confirm", 0);
	}
	
	UPrint_SetFont(8, 2, 2);
    UPrint_Str(eft->merchantName, 2, 1);
    printLine("MID : ", eft->merchantId);
    printLine("DATE TIME   : ", buff);
    printDottedLine();

	UPrint_SetDensity(3); //Set print density to 3 normal
	UPrint_SetFont(7, 2, 2);
	if (isApprovedResponse(eft->responseCode))
	{
		UPrint_StrBold("APPROVED", 1, 4, 1); //Centered large font print title,empty 4 lines
	}
	else{
		UPrint_StrBold("DECLINED", 1, 4, 1); 
	}
	

	printLine("AID            ", eft->aid);

	MaskPan(eft->pan, maskedPan);
	printLine("PAN:           ", maskedPan);
	printLine("EXPIRY         ", eft->expiryDate);
	printLine("RRN:           ", eft->rrn);
	printLine("STAN:          ", eft->stan);
	printLine("TERMINAL NO.:  ", eft->terminalId);

	printLine("CARD NAME:   ", eft->cardHolderName);

	std::string formattedAmount(eft->amount);
	formatAmount(formattedAmount);
	UPrint_StrBold("***************", 1, 4, 1);
	UPrint_StrBold((char *)formattedAmount.c_str(), 1, 4, 1);
    UPrint_StrBold("***************", 1, 4, 1);

	UPrint_Str("\n\n", 2, 1);
	printLine("TVR:           ", eft->tvr);
	printLine("TSI:           ", eft->tsi);

	printDottedLine();
	
	printFooter();

	ret = UPrint_Start(); // Output to printer
	getPrinterStatus(ret);

	return 0;
}

void printHandshakeReceipt(MerchantData *mParam)
{
    int ret = 0; 
	char dt[14] = {'\0'};
	char filename[128] = {0};
    char buff[64] = {'\0'};
    MerchantParameters parameter = {'\0'};

    getParameters(&parameter);
    getDateAndTime(dt);
    sprintf(buff, "%.2s-%.2s-%.2s / %.2s:%.2s", &dt[2], &dt[4], &dt[6], &dt[8], &dt[10]);
	creat_bmp();
	// Prompt is printing
	gui_begin_batch_paint();			
	gui_clear_dc();
	gui_text_out(0, GUI_LINE_TOP(0), "printing...");
	gui_end_batch_paint();

    ret = UPrint_Init();

	if (ret == UPRN_OUTOF_PAPER)
	{
		gui_messagebox_show( "Print" , "No paper" , "" , "confirm" , 0);
	}

	UPrint_SetFont(8, 2, 2);
    UPrint_Str(mParam->address, 2, 1);

    printLine("TID : ", mParam->tid);
    printLine("MID : ", parameter.cardAcceptiorIdentificationCode);
    printLine("DATE TIME   : ", buff);
    printDottedLine();

    
    sprintf(filename, "xxxx\\%s", LOGOIMG);
	UPrint_BitMap(filename, 1);//print image
	// UPrint_Feed(20);

    UPrint_SetFont(4, 2, 2);
    UPrint_StrBold("************", 1, 4, 1);
	UPrint_StrBold("SUCCESSFUL", 1, 4, 1);//Centered large font print title,empty 4 lines
    UPrint_StrBold("************", 1, 4, 1);
    printDottedLine();

    printFooter();

	ret = UPrint_Start();
	getPrinterStatus(ret);


}