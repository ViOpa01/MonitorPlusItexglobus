#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "emvapi/inc/emv_api.h"
#include "libapi_xpos/inc/def.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "libapi_xpos/inc/libapi_util.h"
#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_security.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "network.h"
#include "Receipt.h"
#include "util.h"
#include "EmvEft.h"
#include "nibss.h"
#include "log.h"
#include "appInfo.h"
#include "merchant.h"
#include "virtualtid.h"

#include "sdk_security.h"
#include "EftDbImpl.h"

#include "paycode.h"

#include <pthread.h>

//Approved transaction will be reversed if icc update failed.
// #define DEV_MODE

#ifdef DEV_MODE 
	#include "TestCapk.h"
#else 
	#include "LiveCapk.h"
#endif

static short injectCapks(void)
{
#ifdef DEV_MODE 
	return injectTestCapks();
#else 
	return injectLiveCapks();
#endif
}

#define COUNTRYCODE "\x05\x66" //NGN
#define POS_CONDITION_CODE "00"
#define PIN_CAPTURE_CODE "04"

typedef enum
{
	NO = '0',
	YES = '1'
} YESORNO;

typedef struct __st_card_info
{
	char title[32];
	char pan[32];
	char amt[32];
	char expdate[32];
} st_card_info;

static int first = 0;

#define APP_TRACE(...)
#define APP_TRACE_BUFF_LOG(...)

static IccDataT nibssIccData[] = {
	{0x9F26, 1},
	{0x9F27, 1},
	{0x9F10, 1},
	{0x9F37, 1},
	{0x9F36, 1},
	{0x95, 1},
	{0x9A, 1},
	{0x9C, 1},
	{0x9F02, 1},
	{0x5F2A, 1},
	{0x82, 1},
	{0x9F1A, 1},
	{0x9F34, 1},
	{0x9F33, 1},
	{0x9F35, 1},
	{0x9F1E, 1},
	{0x84, 1},	
	{0x9F09, 1},	
	{0x9F06, 0},
	{0x9F03, 1},	
	{0x5F34, 1},
	{NULL, NULL},
};

static void TestSetTermConfig(TERMCONFIG *termconfig)
{
	//char szBuf[100] = {0};

	APP_TRACE("TestSetTermConfig");

	memset(termconfig, 0x00, sizeof(TERMCONFIG));
	memcpy(termconfig->TermCap, "\xE0\xD8\xC8", 3);					  /*Terminal performance '9F33'*/
	memcpy(termconfig->AdditionalTermCap, "\xFF\x80\xF0\x00\x01", 5); /*Terminal additional performance*/
	memcpy(termconfig->IFDSerialNum, "48397677", 8);					  /*IFD serial number '9F1E'*/
	memcpy(termconfig->TermCountryCode, COUNTRYCODE, 2);			  /*Terminal country code '9F1A'*/
	memcpy(termconfig->TermID, "12312312", 8);						  /*Terminal identification '9F1C'*/
	termconfig->TermType = 0x22;									  /*Terminal type '9F35'*/
	memcpy(termconfig->TransCurrCode, COUNTRYCODE, 2);				  /*Transaction currency '5F2A'*/
	termconfig->TransCurrExp = 0x02;								  /*Transaction currency index '5F36'*/
	memcpy(termconfig->szTransProp,"\x36\x00\xC0\x00",4);				/*TTQ'9F66'*/

	termconfig->bPSE = YES;				   /*Whether to support the choice PSE 1*/
	termconfig->bCardHolderConfirm = YES;  /*Whether to support cardholder confirmation 1*/
	termconfig->bPreferedOrder = YES;	  /*Whether to support the preferred display 1*/
	termconfig->bPartialAID = YES;		   /*Whether to support partial AID matching 1*/
	termconfig->bMultiLanguage = YES;	  /*Whether to support multiple languages 0*/
	termconfig->bCommonCharset = YES;	  /*Whether to support public character sets 0*/
	termconfig->bIPKCValidtionCheck = YES; /*Whether to check the validity of the issuer's public key authentication 1*/
	termconfig->bContainDefaultDDOL = YES; /*Whether to include the defaultDDOL 1*/
	termconfig->bCAPKFailOperAction = NO;  /*CAPK Is operator intervention required when reading an error? 1*/
	termconfig->bCAPKChecksum = YES;	   /*Whether to perform CAPK check 1*/
	/**<Cardholder Verification Method*/
	termconfig->bBypassPIN = NO;			 /*Whether to support skipping PIN input (requires change process, to be determined)*/
	termconfig->bGetDataForPINCounter = YES; /*PIN Try to check if the counter is supportedGetData 1*/
	termconfig->bFailCVM = YES;				 /*Whether to support the wrong CVM (must be Yes)*/
	termconfig->bAmountBeforeCVM = YES;		 /*CVM Whether the amount is known before 1*/
	/**<Term Risk Management*/
	termconfig->bLimitFloorCheck = YES;   /*Whether to perform a minimum check 1*/
	termconfig->bRandomTransSelect = YES; /*Whether to make random trading choices 1*/
	termconfig->bValocityCheck = YES;	 /*Whether to check the frequency 1*/
	termconfig->bTransLog = YES;		  /* Whether to record the transaction log 1*/
	termconfig->bExceptionFile = YES;	 /*Whether to support card blacklist 1*/
	termconfig->bTRMBaseOnAIP = NO;		  /*Whether terminal risk management is based on application interaction characteristics 1*/
	/**<Terminal Action Analysis*/
	termconfig->bTerminalActionCodes = YES;		 /*Whether to support terminal behavior code 1*/
	termconfig->bDefActCodesBefore1stGenAC = NO; /*Is the default behavior code prior to FirstGenerateAC ?*/
	termconfig->bDefActCodesAfter1stGenAC = NO;  /*Is the default behavior code after FirstGenerateAC ?*/
	/**<Completion Processing*/
	termconfig->bForceOnline = YES;		 /*Whether to allow forced online 1*/
	termconfig->bForceAccept = NO;		 /*Whether to allow forced acceptance of transactions 1*/
	termconfig->bAdvices = YES;			 /*Whether to support notification 0*/
	termconfig->bIISVoiceReferal = YES;  /*Whether to support the voice reference initiated by the card issuer ?*/
	termconfig->bBatchDataCapture = YES; /*Whether to support batch data collection*/
	termconfig->bDefaultTDOL = YES;		 /*Is there a default? TDOL*/
	termconfig->bAccountSelect = YES;	/*Whether to support account selection*/
	//visa check value
	termconfig->checkAmtZore=1;
	termconfig->checkRCTL=1;
	termconfig->checkStatus=1;
	termconfig->checkFloorLimit=1;
	termconfig->optionAmtZore=0;
	termconfig->checkCVMLimit=1;
	termconfig->checkOnPIN=1;
	termconfig->checkSig=1;
}

static void TestDownloadAID(TERMINALAPPLIST *TerminalApps)
{
	
	int i = 0;
	int n = 0;

	if(TerminalApps==0)
		return;

	APP_TRACE("TestDownloadAID");
	memset(TerminalApps, 0x00, sizeof(TERMINALAPPLIST));

	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x01\x10\x10", 7); //AID
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x03\x10\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x03\x20\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x03\x20\x20", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x03\x80\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x04\x10\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x04\x99\x99", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x04\x30\x60", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x04\x60\x00", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x05\x00\x01", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x25\x01", 6);
	TerminalApps->TermApp[n++].AID_Length = 6;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x29\x10\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x42\x10\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x42\x20\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x65\x10\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x01\x21\x10\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x01\x21\x47\x11", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x01\x41\x00\x01", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x01\x52\x30\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x03\x33\x01\x01\x01", 8);
	TerminalApps->TermApp[n++].AID_Length = 8;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x03\x33\x01\x01", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x03\x71\x00\x01", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x03\x24\x10\x10", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;
	memcpy(TerminalApps->TermApp[n].AID, "\xA0\x00\x00\x00\x10\x10\x30", 7);
	TerminalApps->TermApp[n++].AID_Length = 7;

	TerminalApps->bTermAppCount = n > UMAX_TERMINAL_APPL ? UMAX_TERMINAL_APPL : n;//The number of AID

	for (i = 0; i < n; i++)
	{
		TerminalApps->TermApp[i].bTerminalPriority = 0x03;							  //Terminal priority
		TerminalApps->TermApp[i].bMaxTargetPercentageInt = 0x00;					  /*Offset randomly selected maximum target percentage*/
		TerminalApps->TermApp[i].bTargetPercentageInt = 0x00;						  /*Randomly selected target percentage*/
		memcpy(TerminalApps->TermApp[i].abTFL_International, "\x00\x00\x3A\x98", 4);  /* Terminal minimum */
		memcpy(TerminalApps->TermApp[i].abThresholdValueInt, "\x00\x00\x13\x88", 4);  /*Offset randomly selected threshold*/
		memcpy(TerminalApps->TermApp[i].abTerminalApplVersion, "\x00\x96", 2);		  /* Terminal application version */
		memcpy(TerminalApps->TermApp[i].TAC_Default, "\x00\x00\x00\x00\x00", 5);	  /* TAC Default data format (n5) */
		memcpy(TerminalApps->TermApp[i].TAC_Denial, "\x00\x00\x00\x00\x00", 5);		  /* TAC Refuse: data format (n5) */
		memcpy(TerminalApps->TermApp[i].TAC_Online, "\x00\x00\x00\x00\x00", 5);		  /* TAC Online: data format (n5) */
		memcpy(TerminalApps->TermApp[i].abTrnCurrencyCode, COUNTRYCODE, 2);			  /* Currency code tag: 5F2A */
		// memcpy(TerminalApps->TermApp[i].abTerminalCountryCode, COUNTRYCODE, 2);		  /* Country code terminal tag: 9F1A */
		TerminalApps->TermApp[i].abTrnCurrencyExp = 0x02;							  /* tag: 5F36 */
		memcpy(TerminalApps->TermApp[i].abEC_TFL, "\x00\x00\x00\x20\x00", 6);		  /* Terminal electronic cash transaction limit tag: 9F7B n12*/
		memcpy(TerminalApps->TermApp[i].abRFOfflineLimit, "\x00\x00\x00\x20\x00", 6); /*Contactless offline minimum :DF19*/
		memcpy(TerminalApps->TermApp[i].abRFTransLimit, "\x00\x00\x02\x00\x00", 6);   /*Contactless transaction limit:DF20*/
		memcpy(TerminalApps->TermApp[i].abRFCVMLimit, "\x00\x00\x00\x10\x00", 6);	 /*Terminal performs CVM quota: DF21*/
		memcpy(TerminalApps->TermApp[i].abDDOL, "\x9F\x37\x04", 3);					  /* TDOL */
		TerminalApps->TermApp[i].DDOL_Length = 0x03;								  /* TDOL Length */
		// TerminalApps->TermApp[i].TerminalType = 0x22;								  /* Terminal type: data format (n 3) */
		// memcpy(TerminalApps->TermApp[i].TerminalCap, "\xE0\xE1\xC8", 3);			  /* Terminal capability: data format (n 3) */
		TerminalApps->TermApp[i].cOnlinePinCap = 0x01;								  /* Terminal online pin capability */
	}

}

void bcdToAsc(char *asc, unsigned char *bcd, const int size)
{
	int i;
	short pos = 0;

	for (i = 0; i < size; i++)
	{
		pos += sprintf(&asc[pos], "%02X", bcd[i]);
	}
}

static void m_DispOffPin(int Count)
{
	if (Count == 0)
	{
		gui_messagebox_show("", "PIN OK", "", "ok", 2000);
	}
	else if (Count == 1 || Count == 2)
	{
		gui_messagebox_show("", "WRONG PIN!", "", "ok", 4000);
	}
	else
	{
		gui_messagebox_show("", "INCORRECT PIN!", "", "ok", 4000);
	}
}

static short isPurchase(const Eft *eft)
{
	return eft->transType == EFT_PURCHASE;
}

static short isCashAdvance(const Eft *eft)
{
	return eft->transType == EFT_CASHADVANCE;
}

static short isBalance(const Eft *eft)
{
	return eft->transType == EFT_BALANCE;
}

static short isPreAuth(const Eft *eft)
{
	return eft->transType == EFT_PREAUTH;
}

static short isCashback(const Eft *eft)
{
	return eft->transType == EFT_CASHBACK;
}

static short isRefund(const Eft *eft)
{
	return eft->transType == EFT_REFUND;
}

static short isCompletion(const Eft *eft)
{
	return eft->transType == EFT_COMPLETION;
}

static short isReversal(const Eft *eft)
{
	return eft->transType == EFT_REVERSAL;
}

static enum TechMode cardTypeToTechMode(const int cardType, const int mode)
{
	switch (cardType)
	{
	case READ_CARD_RET_CANCEL:
		return UNKNOWN_MODE;

	case READ_CARD_RET_MAGNETIC:
		return MAGSTRIPE_MODE;

	case READ_CARD_RET_IC:
		return CHIP_MODE;

	case READ_CARD_RET_RF:
		return (mode == MODE_API_M_STRIPE) ? CONTACTLESS_MAGSTRIPE_MODE : CONTACTLESS_MODE;

	case READ_CARD_RET_TIMEOVER:
		return UNKNOWN_MODE;

	case READ_CARD_RET_FAIL:
		return UNKNOWN_MODE;

	default:
		return UNKNOWN_MODE;
	}
}

void populateEchoData(char echoData[256], const char* others)
{
	// char de59[] = "V240m-3GPlus~346-231-236~1.0.6(Fri-Dec-20-10:50:14-2019-)~release-30812300";
	char terminalSn[22] = {'\0'};
	char de59[200] = {'\0'};
	char temp[256] = {'\0'};
	char buff[124] = {'\0'};
	char dt[15] = {'\0'};

	getTerminalSn(terminalSn);
	Sys_GetDateTime(dt);

	strncpy(temp, echoData, strlen(echoData));
	sprintf(buff, "%s%s%s%s|%s|%s(%.4s-%.2s-%.2s-%.2s:%.2s)", 
			temp, temp[0] ? "~" :"", 
			TERMINAL_MANUFACTURER, 
			APP_MODEL, 
			terminalSn, 
			APP_VER, 
			&dt[0], 
			&dt[4], 
			&dt[6], 
			&dt[8], 
			&dt[10]
	);

	if (others && *(others)) {
		sprintf(de59, "%s%s%s", others, "~", buff);
	} else {
		sprintf(de59, "%s", buff);
	}

	strncpy(echoData, de59, strlen(de59));
}

void copyMerchantParams(Eft *eft, const MerchantParameters *merchantParameters)
{
	strncpy(eft->merchantType, merchantParameters->merchantCategoryCode, sizeof(eft->merchantType));
	strncpy(eft->merchantId, merchantParameters->cardAcceptiorIdentificationCode, sizeof(eft->merchantId));
	strncpy(eft->merchantName, merchantParameters->merchantNameAndLocation, sizeof(eft->merchantName));
	strncpy(eft->currencyCode, merchantParameters->currencyCode, sizeof(eft->currencyCode));
}

short autoReversalInPlace(Eft *eft, NetWorkParameters *netParam)
{
	int result = -1;
	unsigned char response[2048];
	unsigned char packet[2048];
	enum CommsStatus commsStatus = CONNECTION_FAILED;
	const int maxTry = 2;
	int i;
	struct HostType hostType;


	//copy original MTI first before setting transaction type to reversal.
	strncpy(eft->originalMti, transTypeToMti(eft->transType), sizeof(eft->originalMti));

	//copy original date time first before setting the current date time.
	strncpy(eft->originalYyyymmddhhmmss, eft->yyyymmddhhmmss, sizeof(eft->originalYyyymmddhhmmss)); //Date time when original mti trans was done
	// Sys_GetDateTime(eft->yyyymmddhhmmss);

	/*
    strncpy(eft.forwardingInstitutionIdCode, "557694", sizeof(eft.forwardingInstitutionIdCode));
    //strncpy(eft.authorizationCode, "", sizeof(eft.authorizationCode)); //add if present.
	*/

	eft->transType = EFT_REVERSAL;
	eft->reversalReason = TIMEOUT_WAITING_FOR_RESPONSE;

	//Here..
	result = createIsoEftPacket(packet, sizeof(packet), eft);
	if ( result <= 0)
	{
		fprintf(stderr, "Error creating reversal packet...\n");
		return -3;
	}

	netParam->packetSize = result;
	memcpy(netParam->packet, packet, netParam->packetSize);

	LOG_PRINTF("Last 2 bytes -> %02x%02X\n", netParam->packet[result - 2], netParam->packet[result - 1]);

	for (i = 0; i < maxTry; i++)
	{
		if (sendAndRecvPacket(netParam) == SEND_RECEIVE_SUCCESSFUL)
			break;
	}

	if (i == maxTry)
		return -1;

	if (result = getEftOnlineResponse(&hostType, eft, netParam->response, netParam->responseSize))
	{
		return -2;
	}

	if (handleDe39(eft->responseCode, eft->responseDesc))
	{
		return -3;
	}

	//no need to handle hostType for reversal, not sure.

	return 0;
}

static short autoReversal(Eft *eft, NetWorkParameters *netParam)
{
	if(eft->isVasTrans) {
		return 0;
	}
	
	return autoReversalInPlace(eft, netParam);
} 

void paycodeHandler(void)
{
	int option = -1;
	//MerchantData mParam = {'\0'};

	char *payment_list[] = {
		"Purchase (Paycode)",
		"Withdrawal (Paycode)"
	};

	/*
	readMerchantData(&mParam);
	if (mParam.trans_type != 1)
		return EFT_PURCHASE;
	*/

  while (1) 
  {
		switch (option = gui_select_page_ex("Select Paycode Type", payment_list, 2, 30000, 0)) // if exit : -1, timout : -2
		{
		case -1:
		case -2:
			return;
		case 0:
			eftTrans(EFT_PURCHASE, SUB_PAYCODE_CASHIN);
			break;

		case 1:
			eftTrans(EFT_CASHADVANCE, SUB_PAYCODE_CASHOUT);
			break;

		default: 
			continue;
  		}	
   }
}

enum TransType cardPaymentHandler()
{
	int option = -1;
	MerchantData mParam = {'\0'};

	char *payment_list[] = {
		"Purchase",
		"Pre Authorization",
		"Completion",
		"Cashback",
		"Cash Advance",
		"Reversal",
		"Refund",
		"Balance Inquiry"
	};

	readMerchantData(&mParam);
	if (mParam.trans_type != 1)
		return EFT_PURCHASE;

	switch (option = gui_select_page_ex("Select Payment Type", payment_list, 8, 30000, 0)) // if exit : -1, timout : -2
	{
	case -1:
	case -2:
		return EFT_TRANS_END;
	case 0:

		return EFT_PURCHASE;
	case 1:
		
		return EFT_PREAUTH;
	case 2:
		
		return EFT_COMPLETION;
	case 3:
		
		return EFT_CASHBACK;
	case 4:
		
		return EFT_CASHADVANCE;
	case 5:
		
		return EFT_REVERSAL;
	case 6:
		
		return EFT_REFUND;
	case 7:
		
		return EFT_BALANCE;
	default:
		return EFT_PURCHASE;
	}

}

static enum AccountType getAccountType(void)
{
	int option = -1;
	MerchantData mParam = {'\0'};

	char *account_type_list[] = {
		"Savings",
		"Current",
		"Default",
		"Credit",
		"Universal",
		"Investment"};

	readMerchantData(&mParam);
	if (mParam.account_selection != 1)
		return DEFAULT_ACCOUNT;

	switch (option = gui_select_page_ex("Select Account Type", account_type_list, 6, 10000, 0)) // if exit : -1, timout : -2
	{
	case -1:
	case -2:
		return ACCOUNT_END;
	case 0:

		return SAVINGS_ACCOUNT;
	case 1:
		
		return CURRENT_ACCOUNT;
	case 2:
		
		return DEFAULT_ACCOUNT;
	case 3:
		
		return CREDIT_ACCOUNT;
	case 4:
		
		return UNIVERSAL_ACCOUNT;
	case 5:
		
		return INVESTMENT_ACCOUNT;
	default:
		return DEFAULT_ACCOUNT;
	}

	return DEFAULT_ACCOUNT; //I need to be removed.
}

static short accountTypeRequired(const enum TransType transType)
{
	switch (transType)
	{
	//case EFT_BALANCE:
	case EFT_REVERSAL:
	case EFT_REFUND:
		return 0;

	default:
		return 1;
	}
}

static short iccUpdateRequired(const enum TransType transType)
{
	switch (transType)
	{
	//case EFT_BALANCE:
	case EFT_REVERSAL:
	case EFT_REFUND:
	case EFT_BALANCE:
	case EFT_PREAUTH: //Added because Icc update is currently failing for this, might need to fix it later.
		return 0;

	default:
		return 1;
	}
}

static short orginalDataRequired(const Eft *eft)
{
	if (isReversal(eft) || isRefund(eft) || isCompletion(eft))
		return 1;
	return 0;
}

static short uiGetRrn(char rrn[13])
{
	int result;

	// Timeout : -3
	// Cancel  : -2
	// Fail    : -1
	// success  : no of byte(character) entered

	gui_clear_dc();
	if ((result = Util_InputMethod(GUI_LINE_TOP(2), "Enter RRN", GUI_LINE_TOP(5), rrn, 12, 12, 1, 1000)) != 12)
	{
		LOG_PRINTF("rrn input failed ret : %d", result);
		LOG_PRINTF("rrn %s", rrn);
		return result;
	}

	LOG_PRINTF("rrn : %s", rrn);

	return 0;
}

static short confirmEft(const Eft * eft)
{
	char display[65];
	char dateTime[27] = {'\0'};
	int len = 0;

	beautifyDateTime(dateTime, sizeof(dateTime), eft->yyyymmddhhmmss);
	dateTime[strlen(dateTime) - 4] = 0;

	sprintf(display, "Amount: N %.2f\nRRN: %s\nDate: %s", atoll(eft->amount) / 100.0, eft->rrn, dateTime);
	//here
	if (gui_messagebox_show(transTypeToTitle(eft->transType), display, "No", "Yes", 0) != 1) return -1;
	
	return 0;
}

static short isEftAllowed(const enum TransType current, const Eft * eft) 
{
	if (current == eft->transType) 
	{
		const char * title = transTypeToTitle(eft->transType);

		if (eft->transType == EFT_REVERSAL) {
			gui_messagebox_show(title, "Already Reversed", "", "", 0);
		} else if(eft->transType == EFT_REFUND) {
			gui_messagebox_show(title, "Already Refunded", "", "", 0);
		} else if(eft->transType == EFT_COMPLETION) {
			gui_messagebox_show(title, "Already Completed", "", "", 0);
		} else {
			gui_messagebox_show(title, "Unknown Error", "", "", 0);
		}

		return 0;
	}

	return 1;
}

static short getOriginalDataFromDb(Eft *eft)
{
	enum TransType current = eft->transType;

	if (getEft(eft))
	{
		eft->transType = current;
		gui_messagebox_show(eft->rrn, "Couldn't find transaction", "", "", 0);
		return -1;
	}

	if (!isEftAllowed(current, eft)) {
		eft->transType = current;
		return -2;
	}

	//Confirm with the original transaction type.
	if (confirmEft(eft)) {
		eft->transType = current;
		return -3;
	} 

	//Restore current transaction type
	eft->transType = current;

	LOG_PRINTF("OriginalMti -> %s\nFISC -> %s\nOriginal Datetime -> %s\nAuthCode -> %s\n",
		   eft->originalMti, eft->forwardingInstitutionIdCode, eft->originalYyyymmddhhmmss, eft->authorizationCode);

	return 0; //Success																						   //strncpy(eft.authorizationCode, "", sizeof(eft.authorizationCode)); //add if present.
}

static short getReversalReason(Eft *eft)
{
	int option = -1;
	char *reversal_option_list[] = {
		"Response timeout",
		"Customer cancellation",
		"Change dispensed",
		"Issuer not available",
		"Underfloor limit",
		"Pin verification failure",
		"IOU Receipt printed",
		"Overfloor limit",
		"Negative Card",
		"Unspecified",
		"Completed partially"};

	switch (option = gui_select_page_ex("Reversal Reason", reversal_option_list, 11, 10000, 0)) // if exit : -1, timout : -2
	{
		LOG_PRINTF("\nReversal reason");
	case -1:
	case -2:
		return -1;
	case 0:
		eft->reversalReason = TIMEOUT_WAITING_FOR_RESPONSE;
		return 0;
	case 1:
		eft->reversalReason = CUSTOMER_CANCELLATION;
		return 0;
	case 2:
		eft->reversalReason = CHANGE_DISPENSED;
		return 0;
	case 3:
		eft->reversalReason = CARD_ISSUER_UNAVAILABLE;
		return 0;
	case 4:
		eft->reversalReason = UNDER_FLOOR_LIMIT;
		return 0;
	case 5:
		eft->reversalReason = PIN_VERIFICATION_FAILURE;
		return 0;
	case 6:
		eft->reversalReason = IOU_RECEIPT_PRINTED;
		return 0;
	case 7:
		eft->reversalReason = OVER_FLOOR_LIMIT;
		return 0;
	case 8:
		eft->reversalReason = NEGATIVE_CARD;
		return 0;
	case 9:
		eft->reversalReason = UNSPECIFIED_NO_ACTION_TAKEN;
		return 0;
	case 10:
		eft->reversalReason = COMPLETED_PARTIALLY;
		return 0;
	default:
		return -1;
	};

	return -1;
}

static void logNetworkParameters(NetWorkParameters *netWorkParameters)
{
	puts("\n\nEft.......\n");
	LOG_PRINTF("Host -> %s:%d, packet size -> %d\n", netWorkParameters->host, netWorkParameters->port, netWorkParameters->packetSize);
	LOG_PRINTF("IsSsl -> %s, IsHttp -> %s\n", netWorkParameters->isSsl ? "YES" : "NO", netWorkParameters->isHttp ? "YES" : "NO");
}

static int checkIsItexMerchant(MerchantData *mParam)
{
    char merchantName[24] = {'\0'};

	if(!mParam->name[0]) return 1;
	strncpy(merchantName, mParam->name, strlen(mParam->name));

    if (!strncmp(merchantName, "ITEX INTEGRATED", 15)) {
        return 1;
    } else if (!strncmp(merchantName, "ITEX INTERGRATED", 16)) {
        return 1;
    }

    return 0;
}

static short isPayCode(const enum SubTransType subTransType)
{
	switch (subTransType) {
		case SUB_PAYCODE_CASHOUT:
		case  SUB_PAYCODE_CASHIN:
			return 1;

			default: return 0;
	}
   
}

char * payCodeTypeToStr(const enum SubTransType subTransType)
{
	switch (subTransType) {
		case SUB_PAYCODE_CASHOUT: return "PAYCODE CASHOUT";
		case  SUB_PAYCODE_CASHIN: return "PAYCODE CASHIN";

		default : return NULL;
	}
}

void eftTrans(const enum TransType transType, const enum SubTransType subTransType)
{
	Eft eft;

	MerchantData mParam = {'\0'};
	MerchantParameters merchantParameters;
	char sessionKey[33] = {'\0'};
	NetWorkParameters netParam;
	pthread_t thread;

	memset(&netParam, 0x00, sizeof(NetWorkParameters));
	memset(&eft, 0x00, sizeof(Eft));

	if(transType == EFT_TRANS_END) 
	{
		LOG_PRINTF("Invalid Transactipn Type");
		return;
	}

	readMerchantData(&mParam);

	// if(checkIsItexMerchant(&mParam) && subTransType == SUB_NONE)
	// {
	// 	char msg[64] = {'\0'};
	// 	sprintf(msg, "%s is dissabled", transTypeToTitle(transType));
    //     gui_messagebox_show("MESSAGE", msg, "", "", 0);

	// 	return;
	// }

	eft.transType = transType;
	eft.fromAccount = DEFAULT_ACCOUNT; //perform eft will update it if needed
	eft.toAccount = DEFAULT_ACCOUNT;   //perform eft will update it if needed
	eft.isFallback = 0;

	if (getParameters(&merchantParameters))
	{
		LOG_PRINTF("Error getting parameters");
		return;
	}

	if (getSessionKey(sessionKey))
	{
		LOG_PRINTF("Error getting session key");
		return;
	}

	getNetParams(&netParam, CURRENT_PLATFORM, 0);

	if (mParam.tid[0])
	{
		strncpy(eft.terminalId, mParam.tid, strlen(mParam.tid));
	}

	if(!eft.isVasTrans) {
		if (orginalDataRequired(&eft))
		{
			if (isReversal(&eft) && getReversalReason(&eft))
				return;
			if (uiGetRrn(eft.rrn))
				return;
			if (getOriginalDataFromDb(&eft))
				return;
		}
	}
	

	copyMerchantParams(&eft, &merchantParameters);
	
	strncpy(eft.sessionKey, sessionKey, sizeof(eft.sessionKey));
	strncpy(eft.posConditionCode, POS_CONDITION_CODE, sizeof(eft.posConditionCode));
	strncpy(eft.posPinCaptureCode, PIN_CAPTURE_CODE, sizeof(eft.posPinCaptureCode));
	
	pthread_create(&thread, NULL, preDial, &mParam.gprsSettings);

	

	if (subTransType == SUB_NONE) 
	{
		strcpy(netParam.title, transTypeToTitle(transType));
		performEft(&eft, &netParam, (void*)&mParam, transTypeToTitle(transType));
	} else {
		if (isPayCode(subTransType)) {
			LOG_PRINTF("PayCode trans mode");
			strcpy(netParam.title, payCodeTypeToStr(subTransType));
			performPayCode(&eft, &netParam, subTransType);
		}
	}
}

static short amountRequired(const Eft *eft)
{
	if (isBalance(eft) || isReversal(eft))
		return 0;
	return 1;
}

static short generateDE54(Eft *eft, const long long cashbackAmount)
{
	if(cashbackAmount > 0)
	{
		sprintf(eft->additionalAmount, "%02X40%.3sC%012lld", accountTypeToCode(eft->fromAccount), eft->currencyCode, cashbackAmount);
		return 0;
	}

	return -1;
}

static long long getAmount(Eft *eft, const char *title)
{
	long long amount = 0L;
	char amountTitle[45] = {'\0'};
	const int maxLen = 9;

	if (orginalDataRequired(eft)) { //Amount already populated from DB
		return atoll(eft->amount);
	}

	if (!amountRequired(eft))
	{
		sprintf(eft->amount, "%012lld", amount);
		return 0;
	}

	sprintf(amountTitle, "%s Amount", isCashback(eft) ? "PURCHASE" : title);
	amount = inputamount_page(amountTitle, maxLen, 90000);
	if (amount <= 0)
		return -1;

	if (isCashback(eft))
	{
		long long cashbackAmount = 0L;

		cashbackAmount = inputamount_page("Cashback Amount", maxLen, 90000);
		if (cashbackAmount <= 0)
			return -1;
		
		generateDE54(eft, cashbackAmount);
		// sprintf(eft->additionalAmount, "%lld", cashbackAmount);
	}

	sprintf(eft->amount, "%012lld", amount);

	return amount;
}

short handleFailedComms(Eft *eft, const enum CommsStatus commsStatus, NetWorkParameters *netParam)
{
	switch (commsStatus)
	{
	case CONNECTION_FAILED:
		sprintf(eft->message, "%s", "SIM out of Data\nOR Host is down.");
		return -1;

	case SENDING_FAILED:
		sprintf(eft->message, "%s", "Sending Failed");
		return -2;

	case RECEIVING_FAILED:
		sprintf(eft->message, "%s", autoReversal(eft, netParam) ? "Manual Reversal Adviced(1)" : "Trans Reversed");
		return -3;

	default:
		sprintf(eft->message, "%s", "Unknown Comms Response"); 
		return -4;
	}
}

int tpduToNumber(const char *packet)
{
    return (packet[0] << 8) + packet[1];
}

int separatePayload(NetWorkParameters *netParam, Eft *eft)
{
	const char* response = netParam->response;
    int primaryLength = tpduToNumber(response + 2);
    int auxLength = tpduToNumber(response) - primaryLength - 2;
	int sizeofAuxRespBuff = sizeof(eft->vas.auxResponse);

    if (4 + primaryLength + auxLength != netParam->responseSize) {
        LOG_PRINTF("4 + primaryLength + auxLength (%d) != length (%d)", 4 + primaryLength + auxLength, netParam->responseSize);
        return -1;
    }

    memmove(netParam->response, netParam->response + 2, primaryLength);
	netParam->responseSize = primaryLength + 2;
	netParam->response[netParam->responseSize] = '\0';

    if ((auxLength < sizeofAuxRespBuff) && auxLength > 0) {
        memcpy(eft->vas.auxResponse, response + 4 + primaryLength, auxLength);
    } else {
        LOG_PRINTF("Aux buffer size (%zu) not enough for received data (%d)", sizeof(eft->vas.auxResponse), auxLength);
        return -3;
    }


    return 0;
}

/**
 * Function: processPacketOnline
 * Usage: processPacketOnline(...);
 * --------------------------------
 * @return 1 declined transtion.
 * @return negative error occured.
 * @return 0, approved.
 */

static int processPacketOnline(Eft *eft, struct HostType *hostType, NetWorkParameters *netParam)
{
	int result = -1;
	// unsigned char response[2048];
	enum CommsStatus commsStatus = CONNECTION_FAILED;

	if (eft->isVasTrans && eft->vas.genAuxPayload && !isReversal(eft) /* && !isManualReversal() */) {
		char auxPayload[4096];

        eft->vas.genAuxPayload(auxPayload, sizeof(auxPayload), eft);

		if (auxPayload[0]) { // prepend aux payload 
			size_t secLength = strlen(auxPayload);
			size_t newLength = secLength + netParam->packetSize;
			if (newLength + 2 /*tpdu length*/ >= sizeof(netParam->packet)) {
				// E don happen o, abort?
				return -98;
			}
			// append aux payload
			memcpy(&netParam->packet[netParam->packetSize], auxPayload, secLength);
			// prepend tpdu
			memmove(netParam->packet + 2, netParam->packet, newLength);
			netParam->packet[0] = (unsigned char)(newLength >> 8);
			netParam->packet[1] = (unsigned char) newLength;
			// then update reqLen
			netParam->packetSize = newLength + 2;
		}
    }

	commsStatus = sendAndRecvPacket(netParam);

	if (commsStatus != SEND_RECEIVE_SUCCESSFUL) {
		return handleFailedComms(eft, commsStatus, netParam);
	}

	if (eft->isVasTrans && strncmp(netParam->response + 2, "0210", 4)) {

		if (strncmp(netParam->response + 4, "0210", 4)) {

			gui_messagebox_show("Error", "Invalid Card Response!", "", "Ok", 0);
			LOG_PRINTF("Invalid Card response");

			return -98;
		} else if (separatePayload(netParam, eft) < 0) {
			LOG_PRINTF("E don happen\n");
			// E don happen again o
			// what do we do?
			return -99;
		}

	}

	if (result = getEftOnlineResponse(hostType, eft, netParam->response, netParam->responseSize))
	{
		//Shouldn't happen
		LOG_PRINTF("Critical Error");
		if (autoReversal(eft, netParam))
		{
			sprintf(eft->message, "%s", "Manual Reversal Adviced(2)");
		}
		return -7;
	}

	return 0;
}

/*
 ASCII2BCD(eft->responseCode, hostType->AuthResp, sizeof(hostType->AuthResp));
    hostType->Info_Included_Data[0] |= INPUT_ONLINE_AC;

    hostType->AuthorizationCodeLen = strlen(eft->authorizationCode) / 2;
    ASCII2BCD(eft->authorizationCode, hostType->AuthorizationCode, hostType->AuthorizationCodeLen);
    hostType->Info_Included_Data[0] |= INPUT_ONLINE_AUTHCODE;
*/

static int iccUpdate(const Eft *eft, const struct HostType *hostType)
{
	int result = -1;
	int onlineResult = -1;
	unsigned char iccDataBcd[256];
	char iccData[511] = {'\0'};
	short iccDataLen = hostType->iccDataBcdLen;

	onlineResult = isApprovedResponse(eft->responseCode) ? 0 : -1;
	memcpy(iccDataBcd, hostType->iccDataBcd, iccDataLen);

	if (hostType->Info_Included_Data[0] & INPUT_ONLINE_AC)
	{
		memcpy(&iccDataBcd[iccDataLen], hostType->AuthResp, sizeof(hostType->AuthResp));
		iccDataLen += sizeof(hostType->AuthResp);
	}

	logHex(iccDataBcd, iccDataLen, "ICC DATA");

	iccDataLen *= 2; //Convert to asc len;
	Util_Bcd2Asc(iccDataBcd, (char *)iccData, iccDataLen);

	//bcdToAsc(iccData, iccDataBcd, iccDataLen);
	//Now asc

	LOG_PRINTF("Onine result -> %d\nIcc Data -> %s\nresponse code -> %s\n", onlineResult, iccData, eft->responseCode);

	result = emv_online_resp_proc(onlineResult, eft->responseCode, iccData, iccDataLen);

	if (result != EMVAPI_RET_TC)
	{
		return result;
	}

	return result;
}

static void getPurchaseRequest(Eft *eft)
{
	//const char *expectedPacket = "0200F23C46D129E09220000000000000002116539983471913195500000000000000010012201232310000071232311220210856210510010004D0000000006539983345399834719131955D210822100116217990000317377942212070HE88FBP205600444741ONYESCOM VENTURES LTD  KD           LANG566AD456A8EAC9DA12E2809F2608CBAFAFBDB481085F9F2701809F10120110A040002A0000000000000000000000FF9F3704983160FC9F360200F0950500002488009A031912209C01009F02060000000001005F2A020566820239009F1A0205669F34034203009F3303E0F8C89F3501229F1E0834363233313233368407A00000000410109F090200029F03060000000000005F340101074V240m-3GPlus~346-231-236~1.0.6(Fri-Dec-20-10:50:14-2019-)~release-30812300015510101511344101BE97C61158EDB7955608F7238DBFD07DA74483E720F5B172F36FDC35DF68CC55";
	//unsigned char actualPacket[1024];
	//int result = 0;
	char iccData[] = "9F260842C0BA568B50C5BB9F2701809F10200FA501A03900140000000000000000000F0100000000000000000000000000009F3704137CD3729F3602016B950500000080009A032001289C01009F02060000000000005F2A020566820258009F3303E0E8F05F3401019F3501229F34034103029F1A020566";
	memset(eft, 0x00, sizeof(Eft));

	eft->transType = EFT_PURCHASE;
	eft->techMode = CHIP_MODE;
	eft->fromAccount = SAVINGS_ACCOUNT;
	eft->toAccount = DEFAULT_ACCOUNT;
	eft->isFallback = 0;

	strncpy(eft->sessionKey, "C13D4AC49BF7705B9D6EFD0B6ED0E351", sizeof(eft->sessionKey));

	strncpy(eft->pan, "5061050258560993933", sizeof(eft->pan));
	strncpy(eft->amount, "000000000100", sizeof(eft->amount));
	strncpy(eft->yyyymmddhhmmss, "20200113094122", sizeof(eft->yyyymmddhhmmss));
	strncpy(eft->stan, "110347", sizeof(eft->stan));
	strncpy(eft->expiryDate, "2104", sizeof(eft->expiryDate));
	strncpy(eft->merchantType, "5300", sizeof(eft->merchantType));
	strncpy(eft->cardSequenceNumber, "001", sizeof(eft->cardSequenceNumber));
	strncpy(eft->posConditionCode, "00", sizeof(eft->posConditionCode));
	strncpy(eft->posPinCaptureCode, "04", sizeof(eft->posPinCaptureCode));
	strncpy(eft->track2Data, "5061050258560993933D2104601013636041", sizeof(eft->track2Data));
	strncpy(eft->rrn, "200128110339", sizeof(eft->rrn));
	strncpy(eft->serviceRestrictionCode, "221", sizeof(eft->serviceRestrictionCode));
	strncpy(eft->terminalId, "2214KCE2", sizeof(eft->terminalId));
	strncpy(eft->merchantId, "2214LA391425013", sizeof(eft->merchantId));
	strncpy(eft->merchantName, "DARAMOLA MICHAEL ADE   LA           LANG", sizeof(eft->merchantName));
	strncpy(eft->currencyCode, "566", sizeof(eft->currencyCode));

	strncpy(eft->iccData, iccData, sizeof(eft->iccData));
	strncpy(eft->echoData, "PAX|32598757|7.8.18", sizeof(eft->echoData));
}

static int asc2bcd(char asc)
{
	if (asc >= '0' && asc <= '9')
		return asc - '0';
	if (asc >= 'A' && asc <= 'F')
		return asc - 'A' + 10;
	if (asc >= 'a' && asc <= 'f')
		return asc - 'a' + 10;

	return 0;
}

static void str2bcd(char *str, char *bcd, int size)
{
	int i;
	for (i = 0; i < size / 2; i++)
	{
		bcd[i] = (asc2bcd(str[2 * i]) << 4) + (asc2bcd(str[2 * i + 1]) & 0x0F);
	}
}

short getEmvTlvByTag(unsigned char *tag, const int tagSize, unsigned char *tlv)
{
	int length = 0;
	byte value[256];
	int bytes = 0;

	if (EMV_GetKernelData(tag, &length, value))
	{
		fprintf(stderr, "Error getting tag 84\n");
		return 0;
	}
	else
	{
		int i;
		char asc[21];
		unsigned char bcdLen[12];

		memcpy(tlv, tag, tagSize);
		bytes + tagSize;

		LOG_PRINTF("Tag 84 len -> %d\n", length);

		sprintf(asc, "%02X", length);
		str2bcd(asc, bcdLen, 2);

		tlv[bytes] = bcdLen[0];
		bytes += 1;

		memcpy(&tlv[bytes], value, length);
		bytes += length;

		for (i = 0; i < bytes; i++)
		{
			LOG_PRINTF("%02X", tlv[i]);
		}
		puts("\r\n");
	}

	return bytes;
}

short getServiceCodeFromTrack2(char *serviceCode, const char *track2)
{
	const int len = strlen(track2);
	int i;

	for (i = 0; i < len; i++)
	{
		if (track2[i] == '=' || track2[i] == 'D')
		{
			if (strlen(&track2[i]) < 7)
			{
				fprintf(stderr, "No service code present\n");
				return -1;
			}

			strncpy(serviceCode, &track2[i + 5], 3);
			return 0;
		}
	}

	return -2;
}

short getEmvTagValue(unsigned char *value, unsigned char *tag, const int tagSize)
{
	int length = 0;

	unsigned char tagName[] = {0x9F, 0x30};
	length = sizeof(tagName);

	if (EMV_GetKernelData(tagName, &length, value))
	{
		fprintf(stderr, "Error getting tag\n");
		return 0;
	}

	return length;
}

short getEmvTagValueAsc(unsigned char *tag, const int tagSize, char *value)
{
	char asc[511];
	unsigned char bcd[256];
	int length = 0;

	length = getEmvTagValue(bcd, tag, tagSize);

	if (!length)
		return 0;

	bcdToAsc(value, bcd, length);

	return length * 2;
}

short emvGetKernelData(char *ascValue, const unsigned char *tag)
{
	int length = 0;
	byte value[256];

	if (EMV_GetKernelData(tag, &length, value))
	{
		fprintf(stderr, "Error getting tag %02X...\n", tag[0]);
		return -1;
	}

	Util_Bcd2Asc((char *)value, ascValue, length * 2);

	return 0;
}

short addIccTagsToEft(Eft *eft)
{
	eft->cardSequenceNumber[0] = '0';
	if (emvGetKernelData(&eft->cardSequenceNumber[1], "\x5f\x34"))
	{
		return -3;
	}
	LOG_PRINTF("Pan seq asc %s\n", eft->cardSequenceNumber);

	if (emvGetKernelData(eft->tsi, "\x9B"))
	{
		return -4;
	}

	if (emvGetKernelData(eft->tvr, "\x95"))
	{
		return -5;
	}

	if (emvGetKernelData(eft->aid, "\x9F\x06"))
	{
		return -6;
	}

	return 0;
}

static short persistEft(const Eft * eft)
{
	if(eft->isVasTrans) {
		if (updateEft(eft))  {
			fprintf(stderr, "Error updating transaction transaction...\n");
			return -2;
		}

	} else {
		if (eft->atPrimaryIndex == 0) { 
			if (saveEft(eft)) {
				fprintf(stderr, "Error saving transaction...\n");
				return -1;
			} 
		} else {
			if (updateEft(eft))  {
				fprintf(stderr, "Error updating transaction transaction...\n");
				return -2;
			}
		}
	}
	

	return 0;
}

static void displayBalance(char * balance)
{
	gui_messagebox_show("BALANCE", balance, "", "", 0);
}



int performPayCode(Eft *eft, NetWorkParameters *netParam, const enum SubTransType subTransType)
{
	int ret;
	long long namt = 0;
	int result = -1;
	unsigned char packet[2048] = {0x00};
	struct HostType hostType;
	char display[65] = {'\0'};
	char transDate[7] = {'\0'};
	char payCode[13] = {'\0'};
	unsigned long payCodeAmount = 0L;
	const char * title = payCodeTypeToStr(subTransType);
	eft->techMode = CHIP_MODE;
	int year = 0;

    if (title == NULL) {
		LOG_PRINTF("PayCode title is null\n");
		return -1;
	}

   if (getPayCodeData(payCode, sizeof(payCode), &payCodeAmount)) {
       return -1;
   }

   sprintf(eft->amount, "%012ld", payCodeAmount);

    if (buildCardNumber(eft->pan, sizeof(eft->pan), payCode)) {
        return -2;
    }

	if (subTransType == SUB_PAYCODE_CASHOUT) {
		if (getPayCodePin(eft->pinDataBcd, eft->pan)) {
			return -3;
		}

		eft->pinDataBcdLen = 8;
	}

	eft->otherTrans = (int) subTransType;
	strncpy(eft->otherData, payCode, sizeof(eft->otherData));
	getPaycodeExpiry(eft->expiryDate);

	//if (!orginalDataRequired(eft))
	//{
		Sys_GetDateTime(eft->yyyymmddhhmmss);
		strncpy(eft->stan, &eft->yyyymmddhhmmss[8], sizeof(eft->stan));
		getRrn(eft->rrn);
		//strncpy(eft.serviceRestrictionCode, "221", sizeof(eft.serviceRestrictionCode)); //TODO: Get this card from kernel
	//}

	

	strncpy(transDate, &eft->yyyymmddhhmmss[2], 6);
	transDate[6] = 0;
    buildPaycodeIccData(eft->iccData, transDate, eft->amount);
	sprintf(eft->track2Data, "%sD%s", eft->pan, eft->expiryDate);
	strncpy(eft->posDataCode, "510101561344101", 15);
	strncpy(eft->cardSequenceNumber, "000", sizeof(eft->cardSequenceNumber));
	strncpy(eft->serviceRestrictionCode, "501", sizeof(eft->serviceRestrictionCode));
	strncpy(eft->echoData, PTAD_CODE, strlen(PTAD_CODE));


	if ((result = createIsoEftPacket(packet, sizeof(packet), eft)) <= 0)
	{
		return -3;
	}

	netParam->packetSize = result;
	memcpy(netParam->packet, packet, netParam->packetSize);
	
	result = processPacketOnline(eft, &hostType, netParam);

	if (result < 0)
	{ //Error occured,
		return -5;
	}

	persistEft(eft);
	printPaycodeReceipts(eft, 0);

	return result;
}

int performEft(Eft *eft, NetWorkParameters *netParam, void* merchantData, const char *title)
{
	int ret;
	int result = -1;
	unsigned char packet[2048] = {0x00};
	struct HostType hostType;
	unsigned char packedIcc[256] = {'\0'};
	char pinblock[32] = {'\0'};
	char display[65] = {'\0'};
	char notifId[32] = {'\0'};

	st_read_card_in *card_in = 0;
	st_read_card_out *card_out = 0;
	st_card_info card_info = {0};
	TERMCONFIG termconfig = {0};
	TERMINALAPPLIST TerminalApps = {0};

	MerchantData *mParam = (MerchantData*)merchantData;
	readMerchantData(mParam);
	strncpy(notifId, mParam->notificationIdentifier, sizeof(notifId) - 1);
	// 	CAPUBLICKEY pkKey={0};

	// strncpy(eft->responseCode, "06", 2);	// set response code to Error

	card_in = (st_read_card_in *)malloc(sizeof(st_read_card_in));
	memset(card_in, 0x00, sizeof(st_read_card_in));
	//Set card_in

	//EMV_SetInputPin( m_InputPin );//Set offline PIN verification interface
	//EMV_SetDispOffPin( m_DispOffPin );//Set offline PIN prompt interface
	card_in->trans_type = transTypeToCode(eft->transType);

	if (card_in->trans_type == -1)
	{
		LOG_PRINTF("\nUnknow transaction type");
		eft->vas.abortTrans = 1;
		free(card_in);
		return -1;
	}


	card_in->pin_input = 0x03;
	card_in->pin_max_len = 12;
	card_in->key_pid = 1;		  //1 KF_MKSK 2 KF_DUKPT
	card_in->pin_mksk_gid = 0;	//The key index of MKSK; -1 is not encrypt
	card_in->pin_dukpt_gid = -1;  //The key index of DUKPT PIN KEY
	card_in->des_mode = 0;		  //0 ECB, 1 CBC
	card_in->data_dukpt_gid = -1; //The key index of DUPKT Track data KEY
	card_in->ic_online_resp = 1;	//0:not support; 1:chip card reading support online response processing
	card_in->pin_timeover = 60000;

	puts("==================> 2");

	strcpy(card_in->title, title);
	strcpy(card_in->card_page_msg, "Please insert/swipe"); //Swipe interface prompt information, a line of 20 characters, up to two lines, automatic branch.

	puts("==================> 3");

	if (accountTypeRequired(eft->transType))
	{
		eft->fromAccount = getAccountType();
		if (eft->fromAccount == ACCOUNT_END) {
			eft->vas.abortTrans = 1;
		    free(card_in);
			return -1;
		}
	}

	puts("==================> 4");

	//ret = input_num_page(card_in.amt, "input the amount", 1, 9, 0, 0, 1);		// Enter the amount
	//if(ret != INPUT_NUM_RET_OK) return -1;

	if (eft->isVasTrans) {
		
		strncpy(card_in->amt, eft->amount, sizeof(card_in->amt));

	} else {
		long long namt = getAmount(eft, title);
		if (namt < 0) {
			free(card_in);
			eft->vas.abortTrans = 1;
			return -1;
		}

		sprintf(card_in->amt, "%lld", namt);

	}
	
	puts("==================> 5");

	card_in->card_mode = READ_CARD_MODE_MAG | READ_CARD_MODE_IC | READ_CARD_MODE_RF; // Card reading method
	card_in->card_timeover = 60000;													 // Card reading timeout ms

	if (first == 0)
	{
		first = 1;
		TestSetTermConfig(&termconfig);
		EMV_TermConfigInit(&termconfig); //Init EMV terminal parameter
		TestDownloadAID(&TerminalApps);
		EMV_PrmClearAIDPrmFile();
		EMV_PrmSetAIDPrm(&TerminalApps); //Set AID

		if (injectCapks()) { //error

		}
	}

	APP_TRACE("emv_read_card");
	card_out = (st_read_card_out *)malloc(sizeof(st_read_card_out));
	
	while (1)
	{
		memset(card_out, 0, sizeof(st_read_card_out));
		ret = emv_read_card(card_in, card_out);

		if(EMVAPI_RET_FALLBACk == ret)
		{
			card_in->card_mode = READ_CARD_MODE_MAG;
			card_in->forceIC=0;
			memset(card_in->card_page_msg,0x00,sizeof(card_in->card_page_msg));
			strcpy(card_in->card_page_msg,"please try to swipe");
			continue;
		}
		else if(EMVAPI_RET_FORCEIC == ret)
		{
			if(card_in->card_mode == READ_CARD_MODE_MAG)
			{
				ret = EMVAPI_RET_ARQC;
				break;
			}
			else
			{
				card_in->card_mode = READ_CARD_MODE_IC | READ_CARD_MODE_RF;
				memset(card_in->card_page_msg,0x00,sizeof(card_in->card_page_msg));
				strcpy(card_in->card_page_msg,"don't swipe,please tap/insert the card");
				continue;
			}
		}
		else if(EMVAPI_RET_OTHER == ret)
		{
			card_in->card_mode = READ_CARD_MODE_IC;
			memset(card_in->card_page_msg,0x00,sizeof(card_in->card_page_msg));
			strcpy(card_in->card_page_msg,"please insert card");
			continue;
		}

		break;
	}

	puts("==================> 6");

	if (EMVAPI_RET_ARQC == ret)
	{
		// gui_messagebox_show("", "Online Request", "", "ok", 0);
	}
	else if (EMVAPI_RET_TC == ret)
	{
		gui_messagebox_show("", "Approved", "", "ok", 0);
	}
	else if (EMVAPI_RET_AAC == ret)
	{
		gui_messagebox_show("", "Declined", "", "ok", 0);
		free(card_in);
		free(card_out);
		return -1;
	}
	else if (EMVAPI_RET_AAR == ret)
	{
		gui_messagebox_show("", "Terminate", "", "ok", 0);
		free(card_in);
		free(card_out);
		eft->vas.abortTrans = 1;
		return -1;
	}
	else
	{
		gui_messagebox_show("", "Cancel", "", "ok", 0);
		free(card_in);
		free(card_out);
		eft->vas.abortTrans = 1;
		return -1;
	}

	puts("==================> 7");

	if ((eft->techMode = cardTypeToTechMode(card_out->card_type, card_out->nEmvMode)) == UNKNOWN_MODE)
	{ //will never happen
		free(card_in);
		free(card_out);
		eft->vas.abortTrans = 1;
		return -2;
	}

	puts("==================> 8");

	if (addIccTagsToEft(eft))
	{
		return -3;
		eft->vas.abortTrans = 1;
	}

	{
		int length = 0;
		byte value[3];

		if (EMV_GetKernelData("\x9F\x08", &length, value))
		{
			fprintf(stderr, "Error getting tag 9F08\n");
		} else {
			char ascBuff[12] = {'\0'};
			
			Util_Bcd2Asc(value, ascBuff, length * 2);
			fprintf(stdout, "Tag 9F08 is : %s\n", ascBuff);
		}

	}

	{
		int length = 0;
		char value[17] = {'\01'};

		if (EMV_GetKernelData("\x50", &length, value))
		{
			fprintf(stderr, "Error getting tag 50\n");
		} else {
			strcpy(eft->cardLabel, value);
			fprintf(stdout, "Tag 50 is : %s\n", value);
		}

	}

	{
		char tag84[1] = {0x84};
		int length = 0;
		byte value[256];

		if (EMV_GetKernelData(tag84, &length, value))
		{
			fprintf(stderr, "Error getting tag 84\n");
		}
		else
		{
			char tag84Tlv[34];
			int bytes;
			int i;
			char asc[21];
			unsigned char bcdLen[12];

			LOG_PRINTF("Tag 84 len -> %d", length);

			tag84Tlv[0] = tag84[0];
			bytes = 1;

			sprintf(asc, "%02X", length);
			str2bcd(asc, bcdLen, 2);

			tag84Tlv[1] = bcdLen[0];
			//memcpy(&tag84Tlv[1], bcdLen, 1);
			bytes += 1;

			memcpy(&tag84Tlv[2], value, length);
			bytes += length;

			memcpy(&card_out->ic_data[card_out->ic_data_len], tag84Tlv, bytes);
			card_out->ic_data_len += bytes;

			for (i = 0; i < bytes; i++)
			{
				LOG_PRINTF("%02X", tag84Tlv[i]);
			}
			puts("\r\n");
		}
	}

	eft->iccDataBcdLen = normalizeIccData(eft->iccDataBcd, card_out->ic_data, card_out->ic_data_len, nibssIccData, sizeof(nibssIccData) / sizeof(IccDataT));

	if (eft->iccDataBcdLen <= 0)
	{
		fprintf("Error building Icc data\n", eft->iccDataBcdLen);
		eft->vas.abortTrans = 1;
		return -3;
	}

	if (card_out->pin_len) {
		if (eft->vas.switchMerchant) {
			unsigned char pinDataBcd[16];
			memset(pinDataBcd, 0x00, sizeof(pinDataBcd));
			
			getVirtualPinBlock(pinDataBcd, (unsigned char*)card_out->pin_block, mParam->pKey);
			eft->pinDataBcdLen = 8;
			memcpy(eft->pinDataBcd, pinDataBcd, eft->pinDataBcdLen);
			
		} else {
			eft->pinDataBcdLen = 8;
			memcpy(eft->pinDataBcd, card_out->pin_block, eft->pinDataBcdLen);
		}
	}

	strncpy(eft->pan, card_out->pan, sizeof(eft->pan));
	strncpy(eft->track2Data, card_out->track2, sizeof(eft->track2Data));
	strncpy(eft->expiryDate, card_out->exp_data, sizeof(eft->expiryDate));
	getServiceCodeFromTrack2(eft->serviceRestrictionCode, card_out->track2);

	if (!orginalDataRequired(eft))
	{
		Sys_GetDateTime(eft->yyyymmddhhmmss);
		strncpy(eft->stan, &eft->yyyymmddhhmmss[8], sizeof(eft->stan));

		if (!eft->isVasTrans) {
			getRrn(eft->rrn);
		}
		//strncpy(eft.serviceRestrictionCode, "221", sizeof(eft.serviceRestrictionCode)); //TODO: Get this card from kernel
	}

	strncpy(eft->cardHolderName, card_out->vChName, strlen(card_out->vChName));

	LOG_PRINTF("This is card expiry date : %s", card_out->exp_data);

	strcpy(card_info.amt, card_in->amt);
	strcpy(card_info.title, card_in->title);
	strcpy(card_info.pan, card_out->pan);
	strcpy(card_info.expdate, card_out->exp_data);
	populateEchoData(eft->echoData, notifId);

	// strncpy(eft->securityRelatedControlInformation, "FFB5BFFDE4D8E2D7E3A0F28F641FF2EC0000C819BEE020F2A0B6483B27AF72B25305FFFFFFFFFFFFFFFFFFFFFFFFFFFF", sizeof(eft->securityRelatedControlInformation) - 1);
	// strncpy(eft->paymentInformation, "upsl-direct~010085C24300148041Meter Number=12.87001001.Acct=1022643026.Phone=07038085747~upsl-withdrawal", sizeof(eft->paymentInformation) - 1);
	// strncpy(eft->managementData1, "managementdatehere", sizeof(eft->managementData1) - 1);
	

	// getPurchaseRequest(eft);

	if ((result = createIsoEftPacket(packet, sizeof(packet), eft)) <= 0)
	{
		free(card_in);
		free(card_out);
		fprintf(stderr, "Error creating iso packet\n");
		eft->vas.abortTrans = 1;
		return -3;
	}

	netParam->packetSize = result;
	memcpy(netParam->packet, packet, netParam->packetSize);
	
	if(eft->isVasTrans) {
		if (saveEft(eft)) {
			fprintf(stderr, "Error saving transaction...\n");
			return -1;
		} 
	}
	
	result = processPacketOnline(eft, &hostType, netParam);

	LOG_PRINTF("\nResult Before IccUpdate -> %d, Icc data len -> %d", result, hostType.iccDataBcdLen);

	free(card_in);
	free(card_out);

	if (result < 0)
	{ //Error occured,
		return -5;
	}

	if (!result && isApprovedResponse(eft->responseCode) && iccUpdateRequired(eft->transType)) //Successfull
	{
		int status = 0;

		status = iccUpdate(eft, &hostType);

#if defined(DEV_MODE)
		{

			//Calling auto reversal will change transType to EFT_REVERSAL, hence reversal receipt will be printed
			//Also reversal leg will be saved on the db and Reversal Adviced shall be displayed on the receipt.

			result = status;
			if (result)
			{
				printf("Icc Update failed with %d\ndoing auto reversal...\n", result);
				sprintf(eft->message, "%s", autoReversal(eft, netParam) ? "Reversal Adviced(ICC)" : "Trans Reversed(ICC)");
				result = -6;
			}
		}
#endif
	}

	
	persistEft(eft);
	

	if (!result && isApprovedResponse(eft->responseCode) && isBalance(eft)) {
		displayBalance(eft->balance);
		LOG_PRINTF("Balance detail : %s", eft->balance);
	}
	
	if(!eft->isVasTrans)
	{
		// Print card payment receipt alone here
		printReceipts(eft, 0);
	}
	

	LOG_PRINTF("Result After IccUpdate -> %d", result);

	return result;
}
