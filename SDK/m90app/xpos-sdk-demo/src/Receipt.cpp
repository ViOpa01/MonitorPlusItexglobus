
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

#include "EmvEft.h"

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


static void printDottedLine()
{
    UPrint_SetFont(8, 2, 2);
    UPrint_Str(DOTTEDLINE, 2, 1);
}

void printFooter()
{
	char buff[64] = {'\0'};

	sprintf(buff, "%s %s", APP_NAME, APP_VER);
    UPrint_StrBold(buff, 1, 4, 1);
	UPrint_StrBold("POWERED BY ITEX", 1, 4, 1);
    UPrint_StrBold("www.iisysgroup.com", 1, 4, 1);
	UPrint_StrBold("0700-2255-4839", 1, 4, 1);

	UPrint_Feed(108);
}

static char * getReceiptCopyLabel(enum receiptCopy copy)
{
	if(copy == MERCHANT_COPY)
	{
		return "**MERCHANT COPY**";
	} else if(copy == CUSTOMER_COPY)
	{
		return "**CUSTOMER COPY**";
	} else if(copy == REPRINT_COPY){
		return "**REPRINT COPY**";
	} else 
	{
		return "";
	}
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

void getPrinterStatus(const int status)
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

void printBankLogo()
{
	char filename[32] = {'\0'};

	sprintf(filename, "xxxx\\%s", BANKLOGO);
	UPrint_BitMap(filename, 1);//print image
}

static char* accountTypeToString(enum AccountType type)
{
    switch (type)
	{
	case SAVINGS_ACCOUNT :
		return "Savings";
		
	case CURRENT_ACCOUNT :
		return "Current";
	
	case CREDIT_ACCOUNT :
		return "Credit";

	case DEFAULT_ACCOUNT :
		return "Default";

	case UNIVERSAL_ACCOUNT :
		return "Universal";

	case INVESTMENT_ACCOUNT :
		return "Investment";

	default:
		return "Default";
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

static const char *responseCodeToStr(const char responseCode[3])
{

    if (strcmp(responseCode, "00") == 0)
        return "Approved or Completed Successfully";
    if (strcmp(responseCode, "01") == 0)
        return "Refer to card issuer";
    if (strcmp(responseCode, "02") == 0)
        return "Refer to card issuer, special condition";
    if (strcmp(responseCode, "03") == 0)
        return "Invalid merchant";
    if (strcmp(responseCode, "04") == 0)
        return "Pick-up card";
    if (strcmp(responseCode, "05") == 0)
        return "Do not honor";
    if (strcmp(responseCode, "06") == 0)
        return "Error";
    if (strcmp(responseCode, "07") == 0)
        return "Pick-up card, special condition";
    if (strcmp(responseCode, "08") == 0)
        return "Honor with identification";
    if (strcmp(responseCode, "09") == 0)
        return "Request in progress";
    if (strcmp(responseCode, "10") == 0)
        return "Approved, partial";
    if (strcmp(responseCode, "11") == 0)
        return "Approved, VIP";
    if (strcmp(responseCode, "12") == 0)
        return "Invalid transaction";
    if (strcmp(responseCode, "13") == 0)
        return "Invalid amount";
    if (strcmp(responseCode, "14") == 0)
        return "Invalid card number";
    if (strcmp(responseCode, "15") == 0)
        return "No such issuer";
    if (strcmp(responseCode, "16") == 0)
        return "Approved, update track 3";
    if (strcmp(responseCode, "17") == 0)
        return "Customer cancellation";
    if (strcmp(responseCode, "18") == 0)
        return "Customer dispute";
    if (strcmp(responseCode, "19") == 0)
        return "Re-enter transaction";
    if (strcmp(responseCode, "20") == 0)
        return "Invalid response";
    if (strcmp(responseCode, "21") == 0)
        return "No action taken";
    if (strcmp(responseCode, "22") == 0)
        return "Suspected malfunction";
    if (strcmp(responseCode, "23") == 0)
        return "Unacceptable transaction fee";
    if (strcmp(responseCode, "24") == 0)
        return "File update not supported";
    if (strcmp(responseCode, "25") == 0)
        return "Unable to locate record";
    if (strcmp(responseCode, "26") == 0)
        return "Duplicate record";
    if (strcmp(responseCode, "27") == 0)
        return "File update edit error";
    if (strcmp(responseCode, "28") == 0)
        return "File update file locked";
    if (strcmp(responseCode, "29") == 0)
        return "File update failed";
    if (strcmp(responseCode, "30") == 0)
        return "Format error";
    if (strcmp(responseCode, "31") == 0)
        return "Bank not supported";
    if (strcmp(responseCode, "32") == 0)
        return "Completed partially";
    if (strcmp(responseCode, "33") == 0)
        return "Expired card, pick-up";
    if (strcmp(responseCode, "34") == 0)
        return "Suspected fraud, pick-up";
    if (strcmp(responseCode, "35") == 0)
        return "Contact acquirer, pick-up";
    if (strcmp(responseCode, "36") == 0)
        return "Restricted card, pick-up";
    if (strcmp(responseCode, "37") == 0)
        return "Call acquirer security, pick-up";
    if (strcmp(responseCode, "38") == 0)
        return "PIN tries exceeded, pick-up";
    if (strcmp(responseCode, "39") == 0)
        return "No credit account";
    if (strcmp(responseCode, "40") == 0)
        return "Function not supported";
    if (strcmp(responseCode, "41") == 0)
        return "Lost card";
    if (strcmp(responseCode, "42") == 0)
        return "No universal account";
    if (strcmp(responseCode, "43") == 0)
        return "Stolen card";
    if (strcmp(responseCode, "44") == 0)
        return "No investment account";
    if (strcmp(responseCode, "51") == 0)
        return "Not sufficient funds";
    if (strcmp(responseCode, "52") == 0)
        return "No check account";
    if (strcmp(responseCode, "53") == 0)
        return "No savings account";
    if (strcmp(responseCode, "54") == 0)
        return "Expired card";
    if (strcmp(responseCode, "55") == 0)
        return "Incorrect PIN";
    if (strcmp(responseCode, "56") == 0)
        return "No card record";
    if (strcmp(responseCode, "57") == 0)
        return "Transaction not permitted to cardholder";
    if (strcmp(responseCode, "58") == 0)
        return "Transaction not permitted on terminal";
    if (strcmp(responseCode, "59") == 0)
        return "Suspected fraud";
    if (strcmp(responseCode, "60") == 0)
        return "Contact acquirer";
    if (strcmp(responseCode, "61") == 0)
        return "Exceeds withdrawal limit";
    if (strcmp(responseCode, "62") == 0)
        return "Restricted card";
    if (strcmp(responseCode, "63") == 0)
        return "Security violation";
    if (strcmp(responseCode, "64") == 0)
        return "Original amount incorrect";
    if (strcmp(responseCode, "65") == 0)
        return "Exceeds withdrawal frequency";
    if (strcmp(responseCode, "66") == 0)
        return "Call acquirer security";
    if (strcmp(responseCode, "67") == 0)
        return "Hard capture";
    if (strcmp(responseCode, "68") == 0)
        return "Response received too late";
    if (strcmp(responseCode, "75") == 0)
        return "PIN tries exceeded";
    if (strcmp(responseCode, "77") == 0)
        return "Intervene, bank approval required";
    if (strcmp(responseCode, "78") == 0)
        return "Intervene, bank approval required for partial amount";
    if (strcmp(responseCode, "90") == 0)
        return "Cut-off in progress";
    if (strcmp(responseCode, "91") == 0)
        return "Issuer or switch inoperative";
    if (strcmp(responseCode, "92") == 0)
        return "Routing error";
    if (strcmp(responseCode, "93") == 0)
        return "Violation of law";
    if (strcmp(responseCode, "94") == 0)
        return "Duplicate transaction";
    if (strcmp(responseCode, "95") == 0)
        return "Reconcile error";
    if (strcmp(responseCode, "96") == 0)
        return "System malfunction";
    if (strcmp(responseCode, "98") == 0)
        return "Exceeds cash limit";
    if (strcmp(responseCode, "A1") == 0)
        return "Unknown";

    return NULL;
}


/**
 * Function: alignBuffer 
 * Usage: 
 * ----------------------
 */

static void alignBuffer(char *output, const char *input, const int expectedLen, const enum AlignType alignType)
{
    int len = 0;
    int requiredSpaces = 0;
    char tempBuffer[33] = {0};


    if(output == NULL || input == NULL){
        return;
    }

	len = strlen(input);
	// requiredSpaces = expectedLen - len;
	requiredSpaces = expectedLen;

	printf("Len -> %d, requireSpace -> %d\n", len, requiredSpaces);

    const char * space = " ";
    if(strcmp(space, output) == 0){
        strcpy(output, space);
        return;
    }

    if (alignType == ALIGH_RIGHT) {

        int index = 0;

        while(index < requiredSpaces - len){
            strncpy(&tempBuffer[index], space , 1);
            index++;
        }

        strncpy(&tempBuffer[index], input, len);
        strcpy(output, tempBuffer);
    } 
    else if(alignType == ALIGN_LEFT) {

        memset(tempBuffer, '\0', sizeof(tempBuffer));
        int index = 0;
        strncpy(&tempBuffer[index], input, len);
        index = len;

        while(index < requiredSpaces){
            strncpy(&tempBuffer[index], space , 1);
            index++;
        }
        strcpy(output, tempBuffer);
    }
    else if(alignType == ALIGN_CENTER) {

		int leftSpace = requiredSpaces / 2;

		int index = 0;
		memset(tempBuffer, '\0', sizeof(tempBuffer));
		while(index < leftSpace){ 
			strncpy(&tempBuffer[index], space , 1);
			index++;
		}

		strncpy(&tempBuffer[index], input, len);
		index = index + len;

		while(index < leftSpace){
			strncpy(&tempBuffer[index], space , 1);
			index++;
		}

    strncpy(output, tempBuffer, index);

	}
}

//Add a line of print data
static void printLine(char *head, char *val)
{
	char rightAligned[45] = {'\0'};
	int printerWidth = 32;

	alignBuffer(rightAligned, val, printerWidth - strlen(head), ALIGH_RIGHT);
	
	UPrint_SetFont(8, 2, 2);
	UPrint_Str(head, 1, 0);
	UPrint_Str(rightAligned, 1, 1);
}

static void printReceiptAmount(const long long amount, short center)
{
    char buffer[21];
    char line[32] = { '\0' };
    int len;
	const int printerWidth = 32; //Not sure


    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "NGN %.2f", amount / 100.0);


    len = strlen(buffer);
    memset(line, '*', len + 4);

    if (center) {
		UPrint_StrBold(line, 1, 1, 1);
		UPrint_StrBold(buffer, 1, 1, 1);
		UPrint_StrBold(line, 1, 1, 1);
    } else {
		char alignedLine[35] = {'\0'};
		char alignedAmount[35] = {'\0'};
		const char * amountLabel = "AMOUNT";

		alignBuffer(alignedLine, line, printerWidth, ALIGH_RIGHT);
		alignBuffer(alignedAmount, buffer, printerWidth - strlen(amountLabel), ALIGH_RIGHT);

       UPrint_Feed(6);

       UPrint_StrBold(alignedLine, 1, 1, 1);
	   printLine("AMOUNT", alignedAmount);
	   UPrint_StrBold(alignedLine, 1, 1, 1);
    }

	UPrint_Feed(6);
}

static void displayPaymentStatus(char * responseCode)
{
    if (isApprovedResponse(responseCode)) {
        gui_messagebox_show(responseCode , "APPROVED", "" , "" , 200);   
    } else {
		gui_messagebox_show(responseCode, "DECLINED", "", "", 2000);
	}

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
	sprintf(formatAmnt, "NGN %.2f", amount / 100.0);

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

 static void printExpiry(char * expiry)
{
    if (expiry && *expiry) {
        printLine("EXPIRY", expiry);
    }
}

static int printEftReceipt(enum receiptCopy copy, Eft *eft)
{
	int ret = 0;
	char dt[14] = {'\0'};
	char buff[64] = {'\0'};
	char maskedPan[25] = {'\0'};
	// char filename[128] = {'\0'};
    MerchantParameters parameter = {'\0'};
	MerchantData mParam = {'\0'};
	short isApproved = isApprovedResponse(eft->responseCode);
	
    getParameters(&parameter);
	readMerchantData(&mParam);
    getDateAndTime(dt);
    sprintf(buff, "%.2s-%.2s-%.2s / %.2s:%.2s", &dt[2], &dt[4], &dt[6], &dt[8], &dt[10]);

	displayPaymentStatus(eft->responseCode);

	ret = UPrint_Init();

	if (ret == UPRN_OUTOF_PAPER)
	{
		gui_messagebox_show("Print", "No paper", "", "confirm", 0);
	}

	printBankLogo();	// Prints Logo
	
	UPrint_SetFont(8, 2, 2);
	UPrint_StrBold(mParam.name, 1, 0, 1);
    UPrint_StrBold(mParam.address, 1, 0, 1);
    printLine("MID", parameter.cardAcceptiorIdentificationCode);
    printLine("DATE TIME", buff);
    printDottedLine();

	UPrint_StrBold(transTypeToString(eft->transType), 1, 4, 1);
	UPrint_StrBold(getReceiptCopyLabel(copy), 1, 4, 1);

	UPrint_SetDensity(3); //Set print density to 3 normal
	UPrint_SetFont(7, 2, 2);

	
	if (isApprovedResponse(eft->responseCode))
	{
		UPrint_StrBold("APPROVED", 1, 4, 1); //Centered large font print title,empty 4 lines
	}
	else{
		UPrint_StrBold("DECLINED", 1, 4, 1); 
	}

	UPrint_Feed(12);
	

    if (*eft->aid) {
        printLine("AID", eft->aid);
    }
	
	MaskPan(eft->pan, maskedPan);
	printLine("PAN", maskedPan);
	printLine("EXPIRY", eft->expiryDate);
	printLine("LABEL", eft->cardLabel);
    printLine("ACCOUNT", accountTypeToString(eft->fromAccount));

	if (isApproved) {
		printLine("AUTH CODE", eft->authorizationCode);
	}
	
	printLine("RRN", eft->rrn);
	printLine("STAN", eft->stan);
	printLine("TID", mParam.tid);

    if (*eft->cardHolderName) {
        printLine("CARD NAME", eft->cardHolderName);
    }

	if(eft->transType == EFT_BALANCE && isApproved)
	{
		processBalance(eft->balance);
	} else {
		printReceiptAmount(atoll(eft->amount), isApproved);
	}

	UPrint_Str("\n\n", 2, 1);

    if (*eft->tvr) {
        printLine("TVR", eft->tvr);
    }

    if (*eft->tsi) {
        printLine("TSI", eft->tsi);
    }

	UPrint_Feed(12);

	if (*eft->message) {
		printLine("", eft->message);
	}

	if (!isApproved) { 
		char declinedMessage[65];

		sprintf(declinedMessage, "%s: %s", eft->responseCode, responseCodeToStr(eft->responseCode));
		printLine(declinedMessage, eft->message);
	}

	printDottedLine();
	
	printFooter();

	ret = UPrint_Start(); // Output to printer
	getPrinterStatus(ret);

	return 0;
}



static int printPaycodeReceipt(enum receiptCopy copy, Eft *eft)
{
	int ret = 0;
	char dt[14] = {'\0'};
	char buff[64] = {'\0'};
	char maskedPan[25] = {'\0'};
	// char filename[128] = {'\0'};
    MerchantParameters parameter = {'\0'};
	MerchantData mParam = {'\0'};
	short isApproved = isApprovedResponse(eft->responseCode);
    SubTransType subTransType = (SubTransType) eft->otherTrans;
	
    getParameters(&parameter);
	readMerchantData(&mParam);
    getDateAndTime(dt);
    sprintf(buff, "%.2s-%.2s-%.2s / %.2s:%.2s", &dt[2], &dt[4], &dt[6], &dt[8], &dt[10]);

	displayPaymentStatus(eft->responseCode);

	ret = UPrint_Init();

	if (ret == UPRN_OUTOF_PAPER)
	{
		gui_messagebox_show("Print", "No paper", "", "confirm", 0);
	}

	printBankLogo();	// Prints Logo
	
	UPrint_SetFont(8, 2, 2);
	UPrint_StrBold(mParam.name, 1, 0, 1);
    UPrint_StrBold(mParam.address, 1, 0, 1);
    printLine("MID", parameter.cardAcceptiorIdentificationCode);
    printLine("DATE TIME", buff);
    printDottedLine();

	UPrint_StrBold(payCodeTypeToStr(subTransType), 1, 4, 1);
	UPrint_StrBold(getReceiptCopyLabel(copy), 1, 4, 1);

	UPrint_SetDensity(3); //Set print density to 3 normal
	UPrint_SetFont(7, 2, 2);

	
	if (isApprovedResponse(eft->responseCode))
	{
		UPrint_StrBold("APPROVED", 1, 4, 1); //Centered large font print title,empty 4 lines
	}
	else{
		UPrint_StrBold("DECLINED", 1, 4, 1); 
	}

	UPrint_Feed(12);

    printExpiry(eft->expiryDate);

	if (isApproved) {
		printLine("AUTH CODE", eft->authorizationCode);
	}
	
    printLine("PAYCODE", eft->otherData);
	printLine("RRN", eft->rrn);
	printLine("STAN", eft->stan);
	printLine("TID", mParam.tid);

    if (*eft->cardHolderName) {
        printLine("CARD NAME", eft->cardHolderName);
    }

    printReceiptAmount(atoll(eft->amount), isApproved);

	UPrint_Str("\n\n", 2, 1);


	UPrint_Feed(12);

	if (*eft->message) {
		printLine("", eft->message);
	}

	if (!isApproved) { 
		char declinedMessage[65];

		sprintf(declinedMessage, "%s: %s", eft->responseCode, responseCodeToStr(eft->responseCode));
		printLine(declinedMessage, eft->message);
	}

	printDottedLine();
	
	printFooter();

	ret = UPrint_Start(); // Output to printer
	getPrinterStatus(ret);

	return 0;
}

short printReceipts(Eft * eft, const short isReprint)
{
	int ret = -1;

	ret = printEftReceipt(isReprint ? REPRINT_COPY : CUSTOMER_COPY, eft);

	if (isApprovedResponse(eft->responseCode) && !isReprint) {

		if (gui_messagebox_show("MERCHANT COPY", "Print Copy?", "No", "Yes", 0) == 1)
		{
			printf("Merchant copy\n");

			ret = printEftReceipt(MERCHANT_COPY, eft);
			
		}
	}

	return ret;
}


short printPaycodeReceipts(Eft * eft, const short isReprint)
{
	int ret = -1;

	ret = printPaycodeReceipt(isReprint ? REPRINT_COPY : CUSTOMER_COPY, eft);

	if (isApprovedResponse(eft->responseCode) && !isReprint) {

		if (gui_messagebox_show("MERCHANT COPY", "Print Copy?", "No", "Yes", 0) == 1)
		{
			printf("Merchant copy\n");

			ret = printPaycodeReceipt(MERCHANT_COPY, eft);
			
		}
	}

	return ret;
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
	UPrint_StrBold(mParam->name, 1, 0, 1);
    UPrint_StrBold(mParam->address, 1, 0, 1);

    printLine("TID", mParam->tid);
    printLine("MID", parameter.cardAcceptiorIdentificationCode);
    printLine("DATE TIME: ", buff);
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

	printReceipts(&eft, 1);
}

void reprintLastTrans()
{
	Eft eft;

	memset(&eft, 0x00, sizeof(Eft));

	if(getLastTransaction(&eft)) return;

	printReceipts(&eft, 1);
}
