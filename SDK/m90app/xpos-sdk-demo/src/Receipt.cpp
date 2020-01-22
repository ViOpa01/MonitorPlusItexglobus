
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdio.h>

#include "Receipt.h"
#include "appInfo.h"

// #include "EftDBImpl.h"
#include "EftDbImpl.h"

extern "C" {
#include "Nibss8583.h"
#include "nibss.h"
#include "util.h"
#include "logo.h"
#include "itexFile.h"
#include "remoteLogo.h"
#include "libapi_xpos/inc/libapi_print.h"
#include "libapi_xpos/inc/libapi_file.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_util.h"

}

#ifdef WIN32
#define ATOLL _atoi64
#else
#define ATOLL atoll
#endif

#define DOTTEDLINE		"--------------------------------"
#define DOUBLELINE		"================================"

enum AlignType{
	ALIGN_CENTER,
	ALIGN_LEFT,
	ALIGH_RIGHT,
};

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
	UPrint_StrBold("POWERED BY ITEX", 1, 4, 1);
    UPrint_StrBold("www.iisysgroup.com", 1, 4, 1);
	UPrint_StrBold("0700-2255-4839", 1, 4, 1);

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

static char* transTypeToString(enum TransType type)
{

	switch (type)
	{
	case EFT_PURCHASE :
		return "PURCHASE";
		
	case EFT_PREAUTH :
		return "PREAUTHORIZATION";
	
	case EFT_COMPLETION :
		return "COMPLETION";

	case EFT_REVERSAL :
		return "REVERSAL";

	case EFT_REFUND :
		return "REFUND";

	case EFT_CASHBACK :
		return "CASHBACK";

	case EFT_CASHADVANCE :
		return "CASHADVANCE";

	case EFT_BALANCE :
		return "BALANCE";
	
	default:
		return "NULL";
	}
}


/**
 * Function: alignBuffer 
 * Usage: 
 * ----------------------
 */

static void alignBuffer(char * output, const char * input, const int expectedLen, const enum AlignType alignType)
{
	int len = strlen(input);
	int requiredSpaces = expectedLen - len;

	if (len >= expectedLen) return;

	if (alignType == ALIGH_RIGHT) {
		memcpy(output, input, len);
		memset(&output[len], ' ', requiredSpaces);
	} else if(alignType == ALIGN_LEFT) {
		memset(output, ' ', requiredSpaces);
		memcpy(&output[requiredSpaces], input, len);
	} else if(alignType == ALIGN_CENTER) {
		requiredSpaces /= 2;
		memset(output, ' ', requiredSpaces);
		memcpy(&output[requiredSpaces], input, len);
		memset(&output[requiredSpaces + len], ' ', requiredSpaces);
	}

	output[expectedLen] = 0;
}

static void printReceiptAmount(const long long amount, short center)
{
    char buffer[21];
    char line[32] = { '\0' };
    char spaces[33] = { '\0' };
    int len;
	char centralizedAmount[34] = {'\0'};
	char centralizedLine[34] = {'\0'};
	const int printerWidth = 24; //Not sure


    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "NGN %.2f", amount / 100.0);

	alignBuffer(centralizedAmount, buffer, printerWidth, ALIGN_CENTER);


    len = strlen(buffer);
    memset(line, '*', len + 4);

	alignBuffer(centralizedLine, line, printerWidth, ALIGN_CENTER);

    if (center) {
		UPrint_StrBold(centralizedLine, 1, 1, 1);
		UPrint_StrBold(centralizedAmount, 1, 1, 1);
		UPrint_StrBold(centralizedLine, 1, 1, 1);
    } else {
		char alignedLine[35] = {'\0'};
		char alignedAmount[35] = {'\0'};
		const char * amountLabel = "AMOUNT";

		alignBuffer(alignedLine, line, printerWidth, ALIGH_RIGHT);
		alignBuffer(alignedAmount, buffer, printerWidth - strlen(amountLabel), ALIGN_LEFT);

       UPrint_Feed(6);

       UPrint_StrBold(alignedLine, 1, 1, 1);
	   printLine("AMOUNT", alignedAmount);
	   UPrint_StrBold(alignedLine, 1, 1, 1);
    }

	UPrint_Feed(6);
}

static void passBalanceAmount(const char *buff)
{
				
	char tag[32] = {'\0'};
	char formatAmnt[16] = {'\0'};
	long int amount = 0;
	short pos = 0;

	std::string val;
	val.append(buff);

	pos = val.find(":");
	strncpy(tag, val.substr(0, pos).c_str(), val.substr(0, pos).length());
	
	amount = atol(val.substr(pos + 1, std::string::npos).c_str());
	sprintf(formatAmnt, "      NGN %.2f", amount / 100.0);

	printLine(tag, formatAmnt);
}

static void processBalance(char *buff)
{
	char *account;
	account = strtok(buff, ",");
	
	printDottedLine();
	while(account != NULL)
	{
		passBalanceAmount(account);
		account = strtok(NULL, ",");
	}

	printDottedLine();
	free(account);
}

int printEftReceipt(Eft *eft)
{
	int ret = 0;
	char dt[14] = {'\0'};
	char buff[64] = {'\0'};
	char maskedPan[25] = {'\0'};
	char filename[128] = {'\0'};
    MerchantParameters parameter = {'\0'};
	MerchantData mParam = {'\0'};
	short isApproved = isApprovedResponse(eft->responseCode);
	
	char rightAligned[45];
	char alignLabel[45];
	int printerWidth = 32;

    getParameters(&parameter);
	readMerchantData(&mParam);
    getDateAndTime(dt);
    sprintf(buff, "%.2s-%.2s-%.2s / %.2s:%.2s", &dt[2], &dt[4], &dt[6], &dt[8], &dt[10]);


	printf("Starting Print Routine\n");
	ret = UPrint_Init();

	if (ret == UPRN_OUTOF_PAPER)
	{
		gui_messagebox_show("Print", "No paper", "", "confirm", 0);
	}

	sprintf(filename, "xxxx\\%s", BANKLOGO);
	UPrint_BitMap(filename, 1);//print image
	
	UPrint_SetFont(8, 2, 2);
    UPrint_Str(mParam.address, 2, 1);
    printLine("MID : ", parameter.cardAcceptiorIdentificationCode);
    printLine("DATE TIME   : ", buff);
    printDottedLine();

	UPrint_StrBold(transTypeToString(eft->transType), 1, 4, 1);

	UPrint_SetDensity(3); //Set print density to 3 normal
	UPrint_SetFont(7, 2, 2);

	
	//UPrint_StrBold((isApproved) ? "APPROVED" : "DECLINED", 1, 4, 1);
	
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

	if (isApproved) {
		sprintf(alignLabel, "%s", "AuthCode");
		alignBuffer(rightAligned, eft->authorizationCode, printerWidth - strlen(alignLabel), ALIGH_RIGHT);
		printLine(alignLabel, rightAligned);
	}
	
	printLine("RRN:           ", eft->rrn);
	printLine("STAN:          ", eft->stan);
	printLine("TERMINAL NO.:  ", mParam.tid);

	printLine("CARD NAME:   ", eft->cardHolderName);

	if(eft->transType == EFT_BALANCE && isApproved)
	{
		processBalance(eft->balance);
	} else {
		printReceiptAmount(atoll(eft->amount), isApproved);
	}

	UPrint_Str("\n\n", 2, 1);
	printLine("TVR:           ", eft->tvr);
	printLine("TSI:           ", eft->tsi);

	UPrint_Feed(6);

	if (*eft->message) {
		printLine("", eft->message);
	}

	if (!isApproved) { 
		char declinedMessage[65];

		sprintf(declinedMessage, "%s: %s", eft->responseCode, eft->responseDesc);
		printLine(declinedMessage, eft->message);
	}

	printDottedLine();
	
	printFooter();

	ret = UPrint_Start(); // Output to printer
	getPrinterStatus(ret);

	if (gui_messagebox_show("MERCHANT COPY", "Print Copy?", "No", "Yes", 0) == 1)
	{
		printf("Merchant copy\n");
		UPrint_Start();
	}

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


    sprintf(filename, "xxxx\\%s", BANKLOGO);
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

void reprintByRrn(void)
{
	Eft eft;

	int result;
	char rrn[13] = {'\0'};

	memset(&eft, 0x00, sizeof(Eft));
	gui_clear_dc();
	if((result = Util_InputMethod(GUI_LINE_TOP(2), "Enter RRN", GUI_LINE_TOP(5), eft.rrn, 12, 12, 1, 1000)) != 12)
	{
		printf("rrn input failed ret : %d\n", result);
		return;
	}

	if (getEft(&eft)) return;

	printEftReceipt(&eft);
}

void reprintLastTrans()
{
	Eft eft;

	memset(&eft, 0x00, sizeof(Eft));

	if(getLastTransaction(&eft)) return;

	printEftReceipt(&eft);
}