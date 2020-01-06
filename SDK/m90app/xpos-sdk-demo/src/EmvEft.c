#include <string.h>
#include <stdlib.h>

//#include "upay_print.h" //TODO:
#include "emvapi/inc/emv_api.h"
#include "libapi_xpos/inc/def.h"
#include "libapi_xpos/inc/libapi_emv.h"

#include "EmvEft.h"

typedef enum
{
	NO='0',
	YES='1'
}YESORNO;


#if 0

#include "network.h"


//#define TEST_MAGTEK







int upay_consum( void );

int tt_upay_consum(void)
{	
	int nChoice;
	upay_consum();
	return 0;
}

static void _pack_8583()
{
	return;
}
//#define COUNTRYCODE "\x01\x56"//CNY
//#define COUNTRYCODE "\x03\x56"//INR
//#define COUNTRYCODE "\x09\x78"//EUR

//#define COUNTRYCODE "\x08\x40"//USD




#endif

#define COUNTRYCODE "\x05\x66"//NGN

typedef struct __st_card_info{
	char title[32];
	char pan[32];
	char amt[32];
	char expdate[32];
}st_card_info;

static int first = 0;

#define APP_TRACE(...) 
#define APP_TRACE_BUFF_LOG(...) 



static void TestSetTermConfig(TERMCONFIG *termconfig)
{
	//char szBuf[100] = {0};

	APP_TRACE( "TestSetTermConfig" );

	memset(termconfig,0x00,sizeof(TERMCONFIG));	
	memcpy( termconfig->TermCap, "\xE0\xF8\xC8", 3);	/*Terminal performance '9F33'*/
	memcpy( termconfig->AdditionalTermCap,"\xFF\x80\xF0\x00\x01", 5);/*Terminal additional performance*/	
	memcpy( termconfig->IFDSerialNum,"mf90_01",8);		/*IFD serial number '9F1E'*/ 
	memcpy(termconfig->TermCountryCode, COUNTRYCODE, 2);	/*Terminal country code '9F1A'*/
	memcpy( termconfig->TermID, "12312312", 8);			/*Terminal identification '9F1C'*/
	termconfig->TermType = 0x22;						/*Terminal type '9F35'*/
	memcpy( termconfig->TransCurrCode,COUNTRYCODE, 2);	/*Transaction currency '5F2A'*/
	termconfig->TransCurrExp = 0x02;					/*Transaction currency index '5F36'*/

	termconfig->bPSE = YES;					/*Whether to support the choice PSE 1*/
	termconfig->bCardHolderConfirm = YES;	/*Whether to support cardholder confirmation 1*/
	termconfig->bPreferedOrder = YES;		/*Whether to support the preferred display 1*/
	termconfig->bPartialAID = YES;			/*Whether to support partial AID matching 1*/
	termconfig->bMultiLanguage  = YES;		/*Whether to support multiple languages 0*/
	termconfig->bCommonCharset = YES;		/*Whether to support public character sets 0*/	
	termconfig->bIPKCValidtionCheck = YES;	/*Whether to check the validity of the issuer's public key authentication 1*/
	termconfig->bContainDefaultDDOL = YES;	/*Whether to include the defaultDDOL 1*/
	termconfig->bCAPKFailOperAction = NO;	/*CAPK Is operator intervention required when reading an error? 1*/
	termconfig->bCAPKChecksum = YES;		/*Whether to perform CAPK check 1*/
	/**<Cardholder Verification Method*/
	termconfig->bBypassPIN = YES;			/*Whether to support skipping PIN input (requires change process, to be determined)*/
	termconfig->bGetDataForPINCounter =YES;	/*PIN Try to check if the counter is supportedGetData 1*/
	termconfig->bFailCVM = YES;				/*Whether to support the wrong CVM (must be Yes)*/
	termconfig->bAmountBeforeCVM = YES;		/*CVM Whether the amount is known before 1*/
	/**<Term Risk Management*/
	termconfig->bLimitFloorCheck = YES;		/*Whether to perform a minimum check 1*/
	termconfig->bRandomTransSelect = YES;	/*Whether to make random trading choices 1*/
	termconfig->bValocityCheck = YES;		/*Whether to check the frequency 1*/
	termconfig->bTransLog = YES;			/* Whether to record the transaction log 1*/
	termconfig->bExceptionFile = YES;		/*Whether to support card blacklist 1*/
	termconfig->bTRMBaseOnAIP = NO;			/*Whether terminal risk management is based on application interaction characteristics 1*/
	/**<Terminal Action Analysis*/
	termconfig->bTerminalActionCodes = YES;	/*Whether to support terminal behavior code 1*/
	termconfig->bDefActCodesBefore1stGenAC = NO;/*Is the default behavior code prior to FirstGenerateAC ?*/
	termconfig->bDefActCodesAfter1stGenAC = NO;/*Is the default behavior code after FirstGenerateAC ?*/
	/**<Completion Processing*/
	termconfig->bForceOnline= NO;			/*Whether to allow forced online 1*/
	termconfig->bForceAccept = NO;			/*Whether to allow forced acceptance of transactions 1*/
	termconfig->bAdvices = YES;				/*Whether to support notification 0*/
	termconfig->bIISVoiceReferal = YES;		/*Whether to support the voice reference initiated by the card issuer ?*/
	termconfig->bBatchDataCapture = YES;	/*Whether to support batch data collection*/
	termconfig->bDefaultTDOL = YES;			/*Is there a default? TDOL*/
	termconfig->bAccountSelect = YES;		/*Whether to support account selection*/
}


static void TestDownloadAID(TERMINALAPPLIST *TerminalApps)
{
	int i = 0;
	int count = 21;

	APP_TRACE( "TestDownloadAID" );
	memset(TerminalApps,0x00,sizeof(TERMINALAPPLIST));	
	TerminalApps->bTermAppCount = 21;//AID length
	memcpy(TerminalApps->TermApp[0].AID, "\xA0\x00\x00\x00\x01\x10\x10", 7);//AID
	TerminalApps->TermApp[0].AID_Length = 7;
	memcpy(TerminalApps->TermApp[1].AID, "\xA0\x00\x00\x00\x03\x10\x10", 7);
	TerminalApps->TermApp[1].AID_Length = 7;
	memcpy(TerminalApps->TermApp[2].AID, "\xA0\x00\x00\x00\x03\x20\x10", 7);
	TerminalApps->TermApp[2].AID_Length = 7;
	memcpy(TerminalApps->TermApp[3].AID, "\xA0\x00\x00\x00\x03\x20\x20", 7);
	TerminalApps->TermApp[3].AID_Length = 7;
	memcpy(TerminalApps->TermApp[4].AID, "\xA0\x00\x00\x00\x03\x80\x10", 7);
	TerminalApps->TermApp[4].AID_Length = 7;
	memcpy(TerminalApps->TermApp[5].AID, "\xA0\x00\x00\x00\x04\x10\x10", 7);
	TerminalApps->TermApp[5].AID_Length = 7;
	memcpy(TerminalApps->TermApp[6].AID, "\xA0\x00\x00\x00\x04\x99\x99", 7);
	TerminalApps->TermApp[6].AID_Length = 7;
	memcpy(TerminalApps->TermApp[7].AID, "\xA0\x00\x00\x00\x04\x30\x60", 7);
	TerminalApps->TermApp[7].AID_Length = 7;
	memcpy(TerminalApps->TermApp[8].AID, "\xA0\x00\x00\x00\x04\x60\x00", 7);
	TerminalApps->TermApp[8].AID_Length = 7;
	memcpy(TerminalApps->TermApp[9].AID, "\xA0\x00\x00\x00\x05\x00\x01", 7);
	TerminalApps->TermApp[9].AID_Length = 7;
	memcpy(TerminalApps->TermApp[10].AID, "\xA0\x00\x00\x00\x25\x01", 6);
	TerminalApps->TermApp[10].AID_Length = 6;
	memcpy(TerminalApps->TermApp[11].AID, "\xA0\x00\x00\x00\x29\x10\x10", 7);
	TerminalApps->TermApp[11].AID_Length = 7;
	memcpy(TerminalApps->TermApp[12].AID, "\xA0\x00\x00\x00\x42\x10\x10", 7);
	TerminalApps->TermApp[12].AID_Length = 7;
	memcpy(TerminalApps->TermApp[13].AID, "\xA0\x00\x00\x00\x42\x20\x10", 7);
	TerminalApps->TermApp[13].AID_Length = 7;
	memcpy(TerminalApps->TermApp[14].AID, "\xA0\x00\x00\x00\x65\x10\x10", 7);
	TerminalApps->TermApp[14].AID_Length = 7;
	memcpy(TerminalApps->TermApp[15].AID, "\xA0\x00\x00\x01\x21\x10\x10", 7);
	TerminalApps->TermApp[15].AID_Length = 7;
	memcpy(TerminalApps->TermApp[16].AID, "\xA0\x00\x00\x01\x21\x47\x11", 7);
	TerminalApps->TermApp[16].AID_Length = 7;
	memcpy(TerminalApps->TermApp[17].AID, "\xA0\x00\x00\x01\x41\x00\x01", 7);
	TerminalApps->TermApp[17].AID_Length = 7;
	memcpy(TerminalApps->TermApp[18].AID, "\xA0\x00\x00\x01\x52\x30\x10", 7);
	TerminalApps->TermApp[18].AID_Length = 7;
	memcpy(TerminalApps->TermApp[19].AID, "\xA0\x00\x00\x03\x33\x01\x01\x01", 8);
	TerminalApps->TermApp[19].AID_Length = 8;
	memcpy(TerminalApps->TermApp[20].AID, "\xA0\x00\x00\x03\x33\x01\x01", 7);
	TerminalApps->TermApp[20].AID_Length = 7;
	for(i=0; i<21; i++)
	{
		TerminalApps->TermApp[i].bTerminalPriority = 0x03;	//Terminal priority
		TerminalApps->TermApp[i].bMaxTargetPercentageInt = 0x00;/*Offset randomly selected maximum target percentage*/
		TerminalApps->TermApp[i].bTargetPercentageInt = 0x00;/*Randomly selected target percentage*/
		memcpy(TerminalApps->TermApp[i].abTFL_International, "\x00\x00\x3A\x98", 4);/* Terminal minimum */
		memcpy(TerminalApps->TermApp[i].abThresholdValueInt, "\x00\x00\x13\x88", 4);/*Offset randomly selected threshold*/
		memcpy(TerminalApps->TermApp[i].abTerminalApplVersion, "\x00\x96", 2);/* Terminal application version */
		memcpy(TerminalApps->TermApp[i].TAC_Default, "\x00\x00\x00\x00\x00", 5);/* TAC Default data format (n5) */ 
		memcpy(TerminalApps->TermApp[i].TAC_Denial, "\x00\x00\x00\x00\x00", 5);/* TAC Refuse: data format (n5) */
		memcpy(TerminalApps->TermApp[i].TAC_Online, "\x00\x00\x00\x00\x00", 5);/* TAC Online: data format (n5) */
		memcpy(TerminalApps->TermApp[i].abTrnCurrencyCode, COUNTRYCODE, 2);/* Currency code tag: 5F2A */
		memcpy(TerminalApps->TermApp[i].abTerminalCountryCode, COUNTRYCODE, 2);/* Country code terminal tag: 9F1A */
		TerminalApps->TermApp[i].abTrnCurrencyExp = 0x02;/* tag: 5F36 */
		memcpy(TerminalApps->TermApp[i].abEC_TFL, "\x00\x00\x00\x20\x00", 6);/* Terminal electronic cash transaction limit tag: 9F7B n12*/
		memcpy(TerminalApps->TermApp[i].abRFOfflineLimit, "\x00\x00\x00\x20\x00", 6);/*Contactless offline minimum :DF19*/
		memcpy(TerminalApps->TermApp[i].abRFTransLimit, "\x00\x00\x02\x00\x00", 6);/*Contactless transaction limit:DF20*/
		memcpy(TerminalApps->TermApp[i].abRFCVMLimit, "\x00\x00\x00\x10\x00", 6);/*Terminal performs CVM quota: DF21*/
		memcpy(TerminalApps->TermApp[i].abDDOL, "\x9F\x37\x04", 3);/* TDOL */
		TerminalApps->TermApp[i].DDOL_Length = 0x03;/* TDOL Length */
		TerminalApps->TermApp[i].TerminalType = 0x22;/* Terminal type: data format (n 3) */
		memcpy(TerminalApps->TermApp[i].TerminalCap, "\xE0\xE1\xC8", 3);/* Terminal capability: data format (n 3) */		
		TerminalApps->TermApp[i].cOnlinePinCap = 0x01;/* Terminal online pin capability */
	}

}


static void m_DispOffPin(int Count)	
{
	if (Count == 0)
	{
		gui_messagebox_show("", "PIN OK" , "" , "ok" , 2000);
	}
    else if (Count == 1 || Count == 2)
	{
		gui_messagebox_show("", "WRONG PIN!" , "" , "ok" , 4000);
	}
	else 
	{
		gui_messagebox_show("", "INCORRECT PIN!" , "" , "ok" ,4000);
	}
}

/*
 CONTACTLESS_MODE,
        CONTACTLESS_MAGSTRIPE_MODE,
        CHIP_MODE,
        MAGSTRIPE_MODE,
        FALLBACK_MODE,
*/

static enum TechMode cardTypeToTechMode(unsigned int cardType)
{
	switch (cardType) {
		case READ_CARD_RET_CANCEL:
			return UNKNOWN_MODE;  

		case READ_CARD_RET_MAGNETIC:
			return MAGSTRIPE_MODE;

		case READ_CARD_RET_IC:
			return CHIP_MODE;

		case READ_CARD_RET_RF:
			return CONTACTLESS_MODE;

		case READ_CARD_RET_TIMEOVER:
			return  UNKNOWN_MODE;

		case READ_CARD_RET_FAIL:
			return  UNKNOWN_MODE;

		default: 
			return  UNKNOWN_MODE;
	}
	
}

void populateEchoData(char echoData[256])
{
	//TODO: populate echo data	strncpy(eft.echoData, "V240m-3GPlus~346-231-236~1.0.6(Fri-Dec-20-10:50:14-2019-)~release-30812300", sizeof(eft.echoData));

}

static void copyMerchantParams(Eft * eft, const MerchantParameters * merchantParameters)
{
	strncpy(eft->merchantType, merchantParameters->merchantCategoryCode, sizeof(eft->merchantType));
	strncpy(eft->merchantId, merchantParameters->cardAcceptiorIdentificationCode, sizeof(eft->merchantId));
	strncpy(eft->merchantName, merchantParameters->merchantNameAndLocation, sizeof(eft->merchantName));
	strncpy(eft->currencyCode, merchantParameters->currencyCode, sizeof(eft->currencyCode));
}


static enum AccountType getAccountType(void) 
{
	//TODO: Display a menu to prompt for account type and return any of the following respectively.
	//Return ACCOUNT_END if the user cancel the operation
	/*
	SAVINGS_ACCOUNT,
        CURRENT_ACCOUNT,
        CREDIT_ACCOUNT,
        DEFAULT_ACCOUNT,
        UNIVERSAL_ACCOUNT,
        INVESTMENT_ACCOUNT,

        ACCOUNT_END
	*/

	return DEFAULT_ACCOUNT; //I need to be removed.
}

static short accountTypeRequired(const enum TransType transType)
{
	//TODO: we might need to check device configuration to know if account is required or not.

	switch (transType) {
		case EFT_BALANCE:
		case EFT_REVERSAL:
			return 0;

		default: return 1;
	}
}

static short orginalDataRequired(const enum TransType transType)
{
	switch (transType) {
		case EFT_REFUND:
		case EFT_REVERSAL:
			return 1;

		default: 
			return 0;
	}
}

void getRrn(char rrn[13])
{
	//TODO: create rrn
}

void eftTrans(const enum TransType transType)
{
	Eft eft;

	MerchantParameters merchantParameters;
	char sessionKey[33] = {'\0'};
	char tid[9] = {'\0'};

	memset(&eft, 0x00, sizeof(Eft));

	if (getParameters(&merchantParameters)) {
		printf("Error getting parameters\n");
		return;
	}

	if (getSessionKey(sessionKey)) {
		printf("Error getting session key");
		return;
	}

	//TODO: get tid, eft.terminalId

	if (orginalDataRequired(transType))
	{ 
		//TODO: Get old data and populate the struct e.g original rrn, original date time, original mti, original amount etc.
	}

	eft.transType = transType;
	eft.fromAccount = DEFAULT_ACCOUNT; //perform eft will update it if needed
	eft.toAccount = DEFAULT_ACCOUNT;  //perform eft will update it if needed
	eft.isFallback = 0;

	copyMerchantParams(&eft, &merchantParameters);
	populateEchoData(eft.echoData);
	strncpy(eft.sessionKey, sessionKey, sizeof(eft.sessionKey));
	strncpy(eft.posConditionCode, "00", sizeof(eft.posConditionCode));
	strncpy(eft.posPinCaptureCode, "04", sizeof(eft.posPinCaptureCode));

	performEft(&eft);
	
}



int performEft(Eft * eft)
{
	char *title = "SALE";
	int ret;
	long long namt = 0;

	
	st_read_card_in *card_in = 0;
	st_read_card_out *card_out = 0;
	st_card_info card_info={0};
	TERMCONFIG termconfig={0};
	TERMINALAPPLIST TerminalApps={0};
	// 	CAPUBLICKEY pkKey={0};


	card_in=(st_read_card_in *)malloc(sizeof(st_read_card_in));
	memset(card_in,0x00,sizeof(st_read_card_in));
	//Set card_in

	//EMV_SetInputPin( m_InputPin );//Set offline PIN verification interface
	//EMV_SetDispOffPin( m_DispOffPin );//Set offline PIN prompt interface
	card_in->trans_type= (int) eft->transType;
	card_in->pin_input=1;
	card_in->pin_max_len=12;
	card_in->key_pid = 1;//1 KF_MKSK 2 KF_DUKPT
	card_in->pin_key_index=-1;//-1:The returned PIN block is not encrypted (The key index number injected by the key injection tool, such as PIN KEY is 0, and LINE KEY is 1.)
	card_in->pin_timeover=60000;
	strcpy(card_in->title, title);
	strcpy(card_in->card_page_msg, "Please insert/swipe");//Swipe interface prompt information, a line of 20 characters, up to two lines, automatic branch.


	

	if (accountTypeRequired(eft->transType)) {
		eft->fromAccount = getAccountType();
		if (eft->fromAccount == ACCOUNT_END) return -1;
	}
	

	//ret = input_num_page(card_in.amt, "input the amount", 1, 9, 0, 0, 1);		// Enter the amount
	//if(ret != INPUT_NUM_RET_OK) return -1;
	namt = inputamount_page(title, 9, 90000);
	if(namt <= 0)
	{
		free(card_in);
		return -1;
	}

	sprintf(card_in->amt, "%lld" , namt);

	card_in->card_mode = READ_CARD_MODE_MAG | READ_CARD_MODE_IC | READ_CARD_MODE_RF;	// Card reading method
	card_in->card_timeover = 60000;	// Card reading timeout ms
	
	if (first == 0)
	{
		first = 1;
		TestSetTermConfig(&termconfig);
 		EMV_TermConfigInit(&termconfig);//Init EMV terminal parameter
		TestDownloadAID(&TerminalApps);
		EMV_PrmClearAIDPrmFile();
		EMV_PrmSetAIDPrm(&TerminalApps);//Set AID
	// 	EMV_PrmSetCAPK(&pkKey);//Set CAPK	
	}

	APP_TRACE( "emv_read_card" );
	card_out= (st_read_card_out *)malloc(sizeof(st_read_card_out));
	memset(card_out, 0, sizeof(st_read_card_out));
	ret = emv_read_card(card_in, card_out);

	//if(ret == READ_CARD_RET_MAGNETIC){					// Magnetic stripe cards
	//	sdk_log_out("trackb:%s\r\n", card_out.track2);
	//	sdk_log_out("trackc:%s\r\n", card_out.track3);
	//	sdk_log_out("pan:%s\r\n", card_out.pan);
	//	sdk_log_out("expdate:%s\r\n", card_out.exp_data);

	//	
	//}
	//else if(ret == INPUTCARD_RET_ICC){					// 
	//	
	//}
	//else if(ret == INPUTCARD_RET_RFID){
	//card_rf_emv_proc(0, 1, m_tmf_param,_pack_8583);
	//}
	//else{
	//	return ret;
	//}

	if(EMVAPI_RET_ARQC == ret)
	{
		gui_messagebox_show("", "Online Request" , "" , "ok" , 0);
	}
	else if(EMVAPI_RET_TC == ret)
	{
		gui_messagebox_show("", "Approved" , "" , "ok" , 0);
	}
	else if(EMVAPI_RET_AAC == ret)
	{
		gui_messagebox_show("", "Declined" , "" , "ok" , 0);
		free(card_in);
		free(card_out);
		return 0;
	}
	else if(EMVAPI_RET_AAR == ret)
	{
		gui_messagebox_show("", "Terminate" , "" , "ok" , 0);
		free(card_in);
		free(card_out);
		return 0;
	}
	else
	{
		gui_messagebox_show("", "Cancel" , "" , "ok" , 0);
		free(card_in);
		free(card_out);
		return 0;
	}

	if ((eft->techMode = cardTypeToTechMode(card_out->card_type)) == UNKNOWN_MODE) { //will never happen
		free(card_in);
		free(card_out);

		return -2;
	}

	
	if (card_out->ic_data_len) {
		//TODO: copy icc data
		   // strncpy(eft->iccData, card_out->ic_data, card_out->ic_data_len);
	}

	if (card_out->pin_len) {
		 strncpy(eft->pinData, card_out->pin_block, sizeof(eft->pinData));
	}

	strncpy(eft->pan, card_out->pan, sizeof(eft->pan));
	strncpy(eft->track2Data, card_out->track2, sizeof(eft->track2Data));
	strncpy(eft->expiryDate, card_out->exp_data, sizeof(eft->expiryDate));
	strncpy(eft->cardSequenceNumber, card_out->pan_sn, sizeof(eft->cardSequenceNumber));
	sprintf(eft->amount, "%lld", namt); //we won't get amount for balance, reveral, so this will be zero

	if (!orginalDataRequired(eft->transType)) { //this would have been populate if original data elements are required.
		Sys_GetDateTime(eft->yyyymmddhhmmss);
		strncpy(eft->stan, &eft->yyyymmddhhmmss[8], sizeof(eft->stan));

		getRrn(eft->rrn);
		//strncpy(eft.serviceRestrictionCode, "221", sizeof(eft.serviceRestrictionCode)); //TODO: Get this card from kernel
	}
	
	strncpy(eft->cardHolderName, card_out->vChName, strlen(card_out->vChName));
    
	printf("This is card expiry date : %s\n", card_out->exp_data);

	strcpy(card_info.amt, card_in->amt);
	strcpy(card_info.title, card_in->title);
	strcpy(card_info.pan, card_out->pan);
	strcpy(card_info.expdate, card_out->exp_data);

	free(card_in);
	free(card_out);

	//upay_print_proc(&card_info);	//TODO:		// Printout

	return 0;

}