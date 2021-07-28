#include "sdk_http.h"
#include "sdk_showqr.h"
#include "sdk_file.h"

#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_security.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "libapi_xpos/inc/libapi_version.h"

#include "appInfo.h"
#include "util.h"
#include "network.h"
#include "menu_list.h"
#include "merchant.h"
#include "remoteLogo.h"
#include "vas/vasbridge.h"
#include "ussd/ussdmenu.h"

#include "nibss.h"
#include "Nibss8583.h"
#include "EmvEft.h"
#include "EftDbImpl.h"

#include "unistarwrapper.h"


int isIdleState = 1;

#define LOGOIMG "xxxx\\logo.bmp"

//Used By EOD RECIEPT PRINTING DECLARED IN EMVDB//
#ifndef TXTYPE
    typedef enum
    {
        ALL_TRX_TYPES = 999,
        REVERSAL = 0420,
        PURCHASE = 0,
        PURCHASE_WC = 9,
        PRE_AUTH = 60,
        PRE_AUTH_C = 61,
        CASH_ADV = 1,
        REFUND = 20,
        DEPOSIT = 21
    } TrxType;
#define TXTYPE
#endif

static int rowlup;
static int nMainTotal;

typedef struct __st_gui_menu_key_def{
	char *name;	
	char *id;	
	unsigned int uKey;
}st_gui_key_menu;

static st_gui_key_menu _main_menu_def_n[5] = {'\0'};

// Define the menu array, the first parameter is the name of the parent menu,
// the second parameter is the name of the current menu,
// and the second parameter is set when the name is duplicated.
static const st_gui_menu_item_def _menu_def[] = {

	{MAIN_MENU_PAGE, UI_CARD_PAYMENT, ""},
	{MAIN_MENU_PAGE, UI_CARDLESS_PAYMENT, ""},
	{MAIN_MENU_PAGE, UI_VAS,          ""},
	{MAIN_MENU_PAGE, SMART_CARD_TEST,          ""},

	// {MAIN_MENU_PAGE, "TMSTest", ""},
	// {MAIN_MENU_PAGE, "Version", ""},



	// {MAIN_MENU_PAGE, UI_PAYCODE,          ""},
	/*
	{MAIN_MENU_PAGE, UI_PURCHASE,   ""},
	{MAIN_MENU_PAGE, UI_PREAUTH,    ""},
	{MAIN_MENU_PAGE, UI_COMPLETION, ""},
	{MAIN_MENU_PAGE, UI_CASHBACK,   ""},
	{MAIN_MENU_PAGE, UI_CASHADVANCE, ""},
	{MAIN_MENU_PAGE, UI_REVERSAL,   ""},
	{MAIN_MENU_PAGE, UI_REFUND,     ""},
	{MAIN_MENU_PAGE, UI_BALANCE,    ""},
	*/

	/*
	* Demo menus
	{MAIN_MENU_PAGE, "Sales", ""},
	{MAIN_MENU_PAGE, "My Plain", ""},
	{MAIN_MENU_PAGE, "My Ssl", ""},
	{MAIN_MENU_PAGE, "CodePay", ""},
	{MAIN_MENU_PAGE, "Version", ""},
	{MAIN_MENU_PAGE, "Test", ""},
	{MAIN_MENU_PAGE, UI_SETTINGS, ""},
	{MAIN_MENU_PAGE, "Others", ""},
	
	{"Test", "Print", ""},
	{"Test", "Security", ""},
	{"Test", "Http", ""},
	{"Test", "Https", ""},
	{"Test", "ShowQr", ""},
	{"Test", "File", ""},
	{"Test", "Led", ""},
	{"Test", "ShowString", ""},
	{"Test", "TMSTest", ""},
	{"Test", "M1 Test", ""},

	{"Security", "InitDukpt", ""},
	{"Security", "SetMainKey", ""},
	{"Security", "PinTest", ""},
	{"Security", "RsaTest", ""},
	*/

	
	{UI_SETTINGS, "Net Select", ""},
	{UI_SETTINGS, "WIFI Settings", "WIFI Menu"},
	{UI_SETTINGS, UI_SET_TIME, ""},

	/*
	* Demo menus
	{"Others", "View AID", ""},
	{"Others", "View CAPK", ""},
	*/

	{SUPERVISION, UI_REPRINT, ""},
	{SUPERVISION, UI_EOD, ""},
	{SUPERVISION, UI_NETWORK, ""},
	{SUPERVISION, UI_DOWNLOAD_LOGO, ""},
	{SUPERVISION, UI_SET_TIME, ""},
	{SUPERVISION, UI_ABOUT, ""},

	{UI_NETWORK, UI_NET_SELECT, ""},
	{UI_NETWORK, UI_WIFI_SETTINGS, UI_WIFI_MENU},

	{UI_REPRINT, UI_REPRINT_TODAY, ""},
	{UI_REPRINT, UI_REPRINT_BY_DATE, ""},
	{UI_REPRINT, UI_REPRINT_BY_RRN, ""},
	{UI_REPRINT, UI_REPRINT_LAST,   ""},


	{UI_EOD, UI_EOD_ALL_TRANS, ""},
	{UI_EOD, UI_EOD_PURCHASE, ""},
	{UI_EOD, UI_EOD_CASHBACK, ""},
	{UI_EOD, UI_EOD_PREAUTH, ""},
	{UI_EOD, UI_EOD_COMPLETION, ""},
	{UI_EOD, UI_EOD_CASHADVANCE, ""},
	{UI_EOD, UI_EOD_REFUND, ""},
	{UI_EOD, UI_EOD_REVERSAL, ""},

	{MAINTENANCE, UI_PREP_TERMINAL, ""},
	{MAINTENANCE, UI_GET_PARAMETER, ""},
	{MAINTENANCE, UI_CALL_HOME, ""},
	{MAINTENANCE, UI_ACCNT_SELECTION, ""},
	{MAINTENANCE, UI_TRANS_TYPE, ""},
	{MAINTENANCE, UI_NOTIF_ID, ""},
	{MAINTENANCE, UI_CHECK_UPDATE, ""},
	{MAINTENANCE, UI_HWR_VERSION, ""},
	{MAINTENANCE, UI_REBOOT},

};

int sdk_power_proc_page(void *pval)
{
	int msgret;
	st_gui_message *pmsg = (st_gui_message *)pval;

	if (pmsg->message_id == 0x00000001)
	{
		msgret = gui_messagebox_show("Prompt", "shutdown?", "cancel", "confirm", 5000);

		if (msgret == 1)
		{
			osl_power_off();
		}
		return 1;
	}
	return 0;
}

static void ShowString()
{
	st_gui_message pmsg;
	int msg_ret;

	gui_post_message(GUI_GUIPAINT, 0, 0); // Send a paint message

	while (1)
	{
		msg_ret = gui_get_message(&pmsg, 500); // Get the message
		if (msg_ret == 0)
		{
			if (pmsg.message_id == GUI_GUIPAINT)
			{ // 	If it is a paint message, draw the page
				gui_begin_batch_paint();
				gui_clear_dc();
				gui_text_out_ex(0, GUI_LINE_TOP(0), "welcome");
				gui_text_out_ex(0, GUI_LINE_TOP(1), "\xE6\xAC\xA2\xE8\xBF\x8E");
				gui_text_out_ex(0, GUI_LINE_TOP(2), "\xD8\xAA\xD8\xB1\xD8\xAD\xD9\x8A\xD8\xA8");
				gui_text_out_ex(0, GUI_LINE_TOP(3), "\xED\x99\x98\xEC\x98\x81\xED\x95\xA9\xEB\x8B\x88\xEB\x8B\xA4");
				gui_text_out_ex(0, GUI_LINE_TOP(4), "\xD8\xAE\xD9\x88\xD8\xB4\x20\xD8\xA2\xD9\x85\xD8\xAF\xDB\x8C\xD8\xAF");
				//gui_text_out_ex(0, GUI_LINE_TOP(5), "�ا֧ݧѧߧߧ���");
				gui_end_batch_paint();
			}
			else if (pmsg.message_id == GUI_KEYPRESS)
			{ // Handling key messages
				if (pmsg.wparam == GUI_KEY_OK)
				{
					break;
				}
				else if (pmsg.wparam == GUI_KEY_QUIT)
				{
					break;
				}
			}
			gui_proc_default_msg(&pmsg); //  Let the system handle some common messages
		}
	}
}

static int getversions( char *buff)
{
	int i = 0;

	i += sprintf(buff + i, "api:%s\r\n", libapi_version());
	i += sprintf(buff + i, "emv:%s\r\n", EMV_GetVersion());
	i += sprintf(buff + i, "apppub:%s\r\n", apppub_version());
	i += sprintf(buff + i, "atc:%s\r\n", atc_version());
	i += sprintf(buff + i, "json:%s\r\n", json_version());
	i += sprintf(buff + i, "net:%s\r\n", net_version());
	i += sprintf(buff + i, "power:%s\r\n", power_version());
	i += sprintf(buff + i, "producttest:%s\r\n", producttest_version());
	i += sprintf(buff + i, "pub:%s\r\n", pub_version());
	i += sprintf(buff + i, "sqlite:%s\r\n", sqlite_version());
	i += sprintf(buff + i, "switchcheck:%s\r\n", switchcheck_version());
	i += sprintf(buff + i, "tms:%s\r\n", tms_version());
	i += sprintf(buff + i, "wifi:%s\r\n", wifi_version());
	i += sprintf(buff + i, "xgui:%s\r\n", xgui_version());

	return i;
}

static void keyZeroHandler(const char *tid)
{
	int key = UUTIL_TIMEOUT;

	MerchantData mParam = {'\0'};
	readMerchantData(&mParam);

	while (key != GUI_KEY_QUIT)
	{
		char data[32] = {'\0'};
		gui_begin_batch_paint();
		gui_clear_dc();

		sprintf(data, "Terminal SN:");
		gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(1), data);

		memset(data, 0x00, sizeof(data));
		getTerminalSn(data);
		gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(2), data);

		memset(data, 0x00, sizeof(data));
		sprintf(data, "TID: %s", tid);
		gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(3), data);
		gui_text_out((gui_get_width() - gui_get_text_width(mParam.platform_label)) / 2, GUI_LINE_TOP(4), mParam.platform_label);
		
		gui_end_batch_paint();

		if((key = Util_WaitKey(1)) == GUI_KEY_QUIT)
		{
			break;
		}
		
	}
}

static void aboutTerminal(char *tid)
{
	int key = UUTIL_TIMEOUT;

	while (key != GUI_KEY_QUIT)
	{
		gui_begin_batch_paint();
		gui_clear_dc();
		
		gui_text_out((gui_get_width() - gui_get_text_width(APP_NAME)) / 2, GUI_LINE_TOP(0), APP_NAME);
		gui_text_out((gui_get_width() - gui_get_text_width(APP_VER)) / 2, GUI_LINE_TOP(1), APP_VER);
		gui_text_out((gui_get_width() - gui_get_text_width(POWERED_BY)) / 2, GUI_LINE_TOP(2), POWERED_BY);
		gui_text_out((gui_get_width() - gui_get_text_width(PTAD_WEBSITE)) / 2, GUI_LINE_TOP(3), PTAD_WEBSITE);
		gui_text_out((gui_get_width() - gui_get_text_width(PTAD_PHONE)) / 2, GUI_LINE_TOP(4), PTAD_PHONE);
		
		gui_end_batch_paint();

		if((key = Util_WaitKey(1)) == GUI_KEY_QUIT)
		{
			break;
		}
	}
}

static unsigned int insertTpdu(unsigned char *packet, const int size)
{

    packet[0] = size >> 8;
    packet[1] = size;

    return size + 2;
}

static int enableAndDisableAccountSelection()
{
	int option = -1;
	MerchantData mParam = {'\0'};

	char msg[12] = {'\0'};

	readMerchantData(&mParam);

	if (mParam.account_selection == 1)
	{
		sprintf(msg, "%s", "Disable");
	}
	else if (mParam.account_selection == 0)
	{
		sprintf(msg, "%s", "Enable");
	}
	char *menuOption[] = {msg};

	option = gui_select_page_ex("Select", menuOption, 1, 3000, 0); // if it timesout it return -ve no else index on the menu list
	// printf("Option : %d\n", option);

	if (!option)
	{
		option = gui_messagebox_show("Message", msg, "Exit", "Confrim", 0); // Exit : 2, Confirm : 1
		if (option == 1)
		{
			mParam.account_selection = !mParam.account_selection;
			saveMerchantData(&mParam);
		}
	}

	return option;
}

static int enableAndDisableOtherTrans()
{
	int option = -1;
	MerchantData mParam = {'\0'};

	char msg[12] = {'\0'};

	readMerchantData(&mParam);

	if (mParam.trans_type == 1)
	{
		sprintf(msg, "%s", "Disable");
	}
	else if (mParam.trans_type == 0)
	{
		sprintf(msg, "%s", "Enable");
	}
	char *menuOption[] = {msg};

	option = gui_select_page_ex("Select", menuOption, 1, 3000, 0); // if it timesout it return -ve no else index on the menu list

	if (!option)
	{
		option = gui_messagebox_show("Message", msg, "Exit", "Confrim", 0); // Exit : 2, Confirm : 1
		if (option == 1)
		{
			mParam.trans_type = !mParam.trans_type;
			
			saveMerchantData(&mParam);
		}
	}

	return option;
}


#include "sdk_security.h"

static short adminHandler(const char *pid)
{
	if (strcmp(pid, UI_PREP_TERMINAL) == 0)
	{
		
		if (uiHandshake())
		{
			gui_messagebox_show("ERROR", "Prepping failed.", "", "", 3000);
			//TODO: display prepping failed on screen
		}
		
		
	}
	else if (strcmp(pid, UI_GET_PARAMETER) == 0)
	{
		uiGetParameters();
	}
	else if (strcmp(pid, UI_CALL_HOME) == 0)
	{
		uiCallHome();
	}
	else if (strcmp(pid, UI_ACCNT_SELECTION) == 0)
	{
		int ret = enableAndDisableAccountSelection();
		printf("Enable / Disable ret : %d\n", ret);
	} 
	else if (strcmp(pid, UI_TRANS_TYPE) == 0)
	{
		int ret = enableAndDisableOtherTrans();
		printf("Enable / Disable ret : %d\n", ret);
	}
	else if (strcmp(pid, UI_NOTIF_ID) == 0)
	{
		MerchantData mParam = {'\0'};
		readMerchantData(&mParam);

		if(mParam.notificationIdentifier[0]) {
			gui_messagebox_show("Notification ID", mParam.notificationIdentifier, "", "", 3000);
		} else {
			gui_messagebox_show("Notification ID", "Empty notification identifier", "", "", 3000);
		}

	}
	else if (strcmp(pid, UI_CHECK_UPDATE) == 0)
	{
		argot_action("#1#");
	}
	else if (strcmp(pid, UI_HWR_VERSION) == 0)
	{
		char msg[1024] = {'\0'};

		sprintf(msg , "app:%s\r\n", Sys_GetAppVer());
		sprintf(msg+ strlen(msg) , "Device Type:%s\r\n", Sys_GetDeviceType() == SYS_DEVICE_TYPE_H9G ? "H9G":"MP70G");
		sprintf(msg + strlen(msg), "hardware:%s\r\n", sec_get_hw_ver());
		sprintf(msg + strlen(msg), "fireware:%s\r\n", sec_get_fw_ver());
		getversions(msg+ strlen(msg));
		gui_messagebox_show( "Version" , msg , "" , "confirm" , 0);
	}
	else if(strcmp(pid, UI_REBOOT) == 0)
	{
		Sys_Reboot();
	}
	else
	{
		return -1;
	}

	return 0;
}

static short operatorHandler(const char *pid)
{ 
	if (!strcmp(pid, UI_REPRINT_BY_DATE))
	{
		Eft eft;
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, ALL_TRX_TYPES);  
	}
	else if (!strcmp(pid, UI_REPRINT_BY_RRN))
	{
		reprintByRrn();
		return 0;
	}
	else if (!strcmp(pid, UI_REPRINT_TODAY))
	{
		getListofEftToday();
	}
	else if (!strcmp(pid, UI_REPRINT_LAST))
	{
		reprintLastTrans();
		return 0;
	}
	else if (!strcmp(pid, UI_EOD_ALL_TRANS))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, ALL_TRX_TYPES);
	}
	else if (!strcmp(pid, UI_EOD_CASHADVANCE))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, CASH_ADV);
	}
	else if (!strcmp(pid, UI_EOD_CASHBACK))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, PURCHASE_WC);
	}
	else if (!strcmp(pid, UI_EOD_PURCHASE))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, PURCHASE);
	}
	else if (!strcmp(pid, UI_EOD_PREAUTH))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, PRE_AUTH);
	}
	else if (!strcmp(pid, UI_EOD_COMPLETION))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, PRE_AUTH_C);
	}
	else if (!strcmp(pid, UI_EOD_REVERSAL))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, REVERSAL);
	}
	else if (!strcmp(pid, UI_EOD_REFUND))
	{
		Eft eft = {0};
		strcpy(eft.tableName, EFT_DEFAULT_TABLE);
		getListOfEod(&eft, REFUND);
	}
	else if (!strcmp(pid, UI_DOWNLOAD_LOGO))
	{
		MerchantData mParam = {0};
		int nTry = 3;
		int i = 0;

		if(readMerchantData(&mParam)) return -1;

		if (!mParam.is_prepped) { //terminal not preped, parameter not allowed
			gui_messagebox_show("ERROR" , "Please Prep first", "" , "" , 0);
			return -1;
    	}

		if(!mParam.tid[0])
		{
			printf("Empty TID\n");
			return -1;
		}

		for(i = 0; i < nTry; i++)
		{
			if(!downloadRemoteLogo(mParam.tid))
			{
				printf("Image downloaded successfully\n");
				gui_messagebox_show("MESSAGE" , "Logo downloaded", "" , "" , 0);
				break;
			}
		}
		
		if(i == nTry)
		{
			gui_messagebox_show("ERROR", "Logo download failed!!!\n Try again", "", "", 0);
			return -1;
		}
	}
	else if (strcmp(pid, UI_SET_TIME) == 0)
	{
		time_set_page();
	}
	else if (strcmp(pid, UI_ABOUT) == 0)
	{
		MerchantData mParam = {0};

		if (readMerchantData(&mParam))
		{
			gui_messagebox_show("MERCHANT", "Error getting merchant details", "", "", 3000);
			return -1;
		}
		aboutTerminal(mParam.tid);
	}
	else
	{
		return -1;
	}

	return 0;
}


static void removeMerchantData(void)
{
	if (!UFile_Clear(MERCHANT_DETAIL_FILE, FILE_PRIVATE))
	{
		gui_messagebox_show("", "Merchant Record", "", "Cleared", 2000);
	}
	else
	{
		gui_messagebox_show("", "Merchant Record", "", "Error", 2000);
	}
}


static void smartCardTestMenu(void)
{
	int option = -1;

	char *payment_list[] = {
		"Read Customer Card",
		"Write Customer Card",
	};


	switch (option = gui_select_page_ex("Select Option", payment_list, 2, 30000, 0)) // if exit : -1, timout : -2
	{
	case 0:
		customerCardInfo();
		break;
	case 1:
		
		writeUserCardTest();
	
	default:
		return 0;
	}

}

//gui_get_message()
// The menu callback function, as long as all the menu operations of this function are registered,
// this function will be called, and the selected menu name will be returned.
// It is mainly determined in this function that the response menu name is processed differently.
static int _menu_proc(char *pid)
{
	int ret;
	char buff[20];
	int pos = 0;
	char msg[1024];
	int acctTypeValue = -1;

	printf("pid -> %s\n", pid);

	if(!strcmp(pid, UI_CARD_PAYMENT)) {
		eftTrans(cardPaymentHandler(), SUB_NONE);
		return 0;
	}
	else if(!strcmp(pid, UI_CARDLESS_PAYMENT)) {
		ussdTransactionsMenu();
		return 0;
	}else if(!strcmp(pid, SMART_CARD_TEST)) {
		smartCardTestMenu();
		return 0;
	}
	if(!strcmp(pid, UI_VAS)) {
		vasTransactionTypesBridge();
		return 0;
	}
	else if (!adminHandler(pid))
	{
		return 0;
	}
	else if(!operatorHandler(pid))
	{
		return 0;
	} else if(!strcmp(pid, UI_CASHIN))
	{
		performCashIn();
		return 0;
	} else if(!strcmp(pid, UI_CASHOUT))
	{
		performCashOut();
		return 0;
	}
	else if (strcmp(pid, "Sales") == 0)
	{
		upay_consum();
	}
	else if (strcmp(pid, "CodePay") == 0)
	{
		upay_barscan();
	}
	else if (strcmp(pid, "Print") == 0)
	{
		sdk_print();
	}
	else if (strcmp(pid, "InitDukpt") == 0)
	{
		dukptTest();
	}
	else if (strcmp(pid, "SetMainKey") == 0)
	{
		mkskTest();
	}
	else if (strcmp(pid, "PinTest") == 0)
	{
		PinTest();
	}
	else if (strcmp(pid, "RsaTest") == 0)
	{
		RsaTest();
	}
	else if (strcmp(pid, "Http") == 0)
	{
		sdk_http_test();
	}
	else if (strcmp(pid, "Https") == 0)
	{
		//test2();
		sdk_https_test();
	}
	else if (strcmp(pid, "ShowQr") == 0)
	{
		showQrTest();
	}
	else if (strcmp(pid, "File") == 0)
	{
		fileTest();
	}
	else if (strcmp(pid, "Led") == 0)
	{
		sdk_driver_led();
	}
	else if (strcmp(pid, "Open Log") == 0)
	{
		//LogOutSet_Show();
	}
	else if (strcmp(pid, "ShowString") == 0)
	{
		ShowString();
	}
	else if (strcmp(pid, "View AID") == 0)
	{
		EMV_ShowAID_Prm();
	}
	else if (strcmp(pid, "View CAPK") == 0)
	{
		EMV_ShowCAPK_Prm();
	}
	else if (strcmp(pid , "View emv") == 0){
		sprintf(msg + strlen(msg), "%s\r\n", EMV_GetVersion());
		gui_messagebox_show( "View emv" , msg , "" , "confirm" , 0);
	}
	else if(strcmp(pid , "View PRMacqKey") == 0){
		EMV_ShowRuPayPRMacqKey();
	}
	else if(strcmp(pid , "View Service") == 0){
		EMV_ShowRuPayService();
	}
	else if(strcmp(pid , "RP SrData DlTest") == 0)
	{
		init_service_prmacqkey(1);
	}
	else if(strcmp(pid , "RP SrData Clear") == 0)
	{
		clear_service_prmacqkey();
	}
	else if (strcmp(pid, "M1 Test") == 0)
	{
		sdk_M1test();
	}
	else if (strcmp(pid, "My Plain") == 0)
	{
		//sendAndReceiveDemoRequest(0, 80);
	}
	else if (strcmp(pid, "My Ssl") == 0)
	{
		//sendAndReceiveDemoRequest(1, 443);
	}
	
	return 0;
}

void get_yyyymmdd_str(char *buff)
{
	char d[32] = {0};
	Sys_GetDateTime(d);
	sprintf(buff, "%c%c%c%c-%c%c-%c%c", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
}

void get_hhmmss_str(char *buff)
{
	char d[32] = {0};
	Sys_GetDateTime(d);
	sprintf(buff, "%c%c:%c%c:%c%c", d[8], d[9], d[10], d[11], d[12], d[13]);
}

static short validateUsersPin(const char* validPin)
{
	char pin[5] = {'\0'};
	int result = 0;

	gui_clear_dc();
	if((result = Util_InputText(GUI_LINE_TOP(0), "ENTER PIN", GUI_LINE_TOP(2), pin, 4, 4, 0, 2, 10000)) == 4)
	{
		printf("Password : %s, ret : %d\n", pin, result);
		if(!strncmp(pin, validPin, 4)) 
		{
			return 0;
		}
		else 
		{
			gui_clear_dc();
			gui_messagebox_show("ERROR", "Invalid PIN", "", "", 3000);
			return -1;
		}
	}
	
	return -1;
}

void standby_pagepaint()
{
	int pos;
	char data[32] = {0};
	char f1Msg[] = "Operator";
	char f2Msg[] = "Admin";
	char spaceRequired[35] = {'\0'};
	int widthlength = gui_get_width();
	int pixelperchar = gui_get_text_width(f1Msg) / strlen(f1Msg);

	gui_begin_batch_paint();
	gui_clear_dc();

	char msg[32] = {'\0'};
	char sn[16] = {'\0'};


	strcpy(msg, "DEVICE NOT READY");
	gui_text_out((gui_get_width() - gui_get_text_width(msg)) / 2, GUI_LINE_TOP(0), msg);

	getTerminalSn(sn);
	memset(msg, 0x00, sizeof(msg));
	sprintf(msg, "SN : %s", sn);
	gui_text_out((gui_get_width() - gui_get_text_width(msg)) / 2, GUI_LINE_TOP(2), msg);

	get_yyyymmdd_str(data);
	data[10] = ' ';
	data[11] = ' ';
	get_hhmmss_str(&data[12]);
	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(3), data);
	
	sprintf(data, "Version:%s", APP_VER);
	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(4), data);

	pos = (widthlength - gui_get_text_width(f1Msg) - gui_get_text_width(f2Msg)) / pixelperchar;

	memset(spaceRequired, ' ', pos);
	pos = sprintf(data, "%s%s%s", f1Msg, spaceRequired, f2Msg);
	data[pos] = 0;

	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(5), data);
	
	gui_end_batch_paint();
}

// static const st_gui_key_menu _main_menu_def_n[] = 
// {
// 	{UI_CARD_PAYMENT,		"",	GUI_KEY_1},
// 	{UI_CARDLESS_PAYMENT,	"",	GUI_KEY_2},
// 	{UI_VAS,				"",	GUI_KEY_3},
// 	// {SMART_CARD_TEST,		"",	GUI_KEY_4},
// };

void display_menu(void)
{
	MerchantData merchant;

	memset(&merchant, 0x00, sizeof(MerchantData));

	readMerchantData(&merchant);

	if (strcmp(merchant.app_type, "AGENCY") == 0) {
		//TODO present 3 menus, 
		// CASH IN CASH OUT
		// CARDLESS
		// VAS

		if (isAgent()) {

			_main_menu_def_n[0].name = UI_CASHOUT;
			_main_menu_def_n[0].id = NULL;
			_main_menu_def_n[0].uKey = GUI_KEY_1;

			_main_menu_def_n[1].name = UI_CASHIN;
			_main_menu_def_n[1].id = NULL;
			_main_menu_def_n[1].uKey = GUI_KEY_2;

			_main_menu_def_n[2].name = UI_VAS;
			_main_menu_def_n[2].id = NULL;
			_main_menu_def_n[2].uKey = GUI_KEY_3;

			_main_menu_def_n[3].name = NULL;
			_main_menu_def_n[3].id = NULL;
			_main_menu_def_n[3].uKey = NULL;

			_main_menu_def_n[4].name = NULL;
			_main_menu_def_n[4].id = NULL;
			_main_menu_def_n[4].uKey = NULL;

		} else {

			_main_menu_def_n[0].name = UI_VAS;
			_main_menu_def_n[0].id = NULL;
			_main_menu_def_n[0].uKey = GUI_KEY_1;

			_main_menu_def_n[1].name = NULL;
			_main_menu_def_n[1].id = NULL;
			_main_menu_def_n[1].uKey = NULL;

			_main_menu_def_n[2].name = NULL;
			_main_menu_def_n[2].id = NULL;
			_main_menu_def_n[2].uKey = NULL;

			_main_menu_def_n[3].name = NULL;
			_main_menu_def_n[3].id = NULL;
			_main_menu_def_n[3].uKey = NULL;

			_main_menu_def_n[4].name = NULL;
			_main_menu_def_n[4].id = NULL;
			_main_menu_def_n[4].uKey = NULL;
		}



	} else if (strcmp(merchant.app_type, "CONVERTED") == 0) {
		//TODO present 4 menus, 
		// CASH IN CASH OUT
		// CARD PAYMENT
		// CARDLESS
		// VAS

		if (isAgent()) {

			_main_menu_def_n[0].name = UI_CARD_PAYMENT;
			_main_menu_def_n[0].id = NULL;
			_main_menu_def_n[0].uKey = GUI_KEY_1;

			_main_menu_def_n[1].name = UI_CARDLESS_PAYMENT;
			_main_menu_def_n[1].id = NULL;
			_main_menu_def_n[1].uKey = GUI_KEY_2;

			_main_menu_def_n[2].name = UI_CASHOUT;
			_main_menu_def_n[2].id = NULL;
			_main_menu_def_n[2].uKey = GUI_KEY_3;

			_main_menu_def_n[3].name = UI_CASHIN;
			_main_menu_def_n[3].id = NULL;
			_main_menu_def_n[3].uKey = GUI_KEY_4;

			_main_menu_def_n[4].name = UI_VAS;
			_main_menu_def_n[4].id = NULL;
			_main_menu_def_n[4].uKey = GUI_KEY_5;

		} else {

			_main_menu_def_n[0].name = UI_CARD_PAYMENT;
			_main_menu_def_n[0].id = NULL;
			_main_menu_def_n[0].uKey = GUI_KEY_1;

			_main_menu_def_n[1].name = UI_CARDLESS_PAYMENT;
			_main_menu_def_n[1].id = NULL;
			_main_menu_def_n[1].uKey = GUI_KEY_2;

			_main_menu_def_n[2].name = UI_VAS;
			_main_menu_def_n[2].id = NULL;
			_main_menu_def_n[2].uKey = GUI_KEY_3;

			_main_menu_def_n[3].name = NULL;
			_main_menu_def_n[3].id = NULL;
			_main_menu_def_n[3].uKey = NULL;

			_main_menu_def_n[4].name = NULL;
			_main_menu_def_n[4].id = NULL;
			_main_menu_def_n[4].uKey = NULL;

		}

	} else {

		//TODO present 3 menus, 
		// CARD PAYMENT
		// CARDLESS
		// VAS

		_main_menu_def_n[0].name = UI_CARD_PAYMENT;
		_main_menu_def_n[0].id = NULL;
		_main_menu_def_n[0].uKey = GUI_KEY_1;

		_main_menu_def_n[1].name = UI_CARDLESS_PAYMENT;
		_main_menu_def_n[1].id = NULL;
		_main_menu_def_n[1].uKey = GUI_KEY_2;

		_main_menu_def_n[2].name = UI_VAS;
		_main_menu_def_n[2].id = NULL;
		_main_menu_def_n[2].uKey = GUI_KEY_3;

		_main_menu_def_n[3].name = NULL;
		_main_menu_def_n[3].id = NULL;
		_main_menu_def_n[3].uKey = NULL;

		_main_menu_def_n[4].name = NULL;
		_main_menu_def_n[4].id = NULL;
		_main_menu_def_n[4].uKey = NULL;

	}
}

void main_page_show()
{
	int i=0;
	char szLine1[16]={0};
	char szLine2[16]={0};
	char szLine3[16]={0};
	char szLine4[16]={0};
	char szLine5[16]={0};
	gui_begin_batch_paint();
	gui_clear_dc();
	gui_set_text_zoom(2);
	//gui_set_font(0);
	i = 0*5;

	char msg[] = "SELECT OPTION";
	display_menu();	// present the appropriate menu

	gui_text_out((gui_get_width() - gui_get_text_width(msg)) / 2, GUI_LINE_TOP(0), msg);

	sprintf(szLine1,"%d.%s",++i,_main_menu_def_n[i-1].name);
	gui_text_out(0, GUI_LINE_TOP(1), szLine1);

	if(i!=nMainTotal && _main_menu_def_n[i].name != NULL){
		sprintf(szLine2,"%d.%s",++i,_main_menu_def_n[i-1].name);
		gui_text_out(0, GUI_LINE_TOP(2), szLine2);
	}
	
	if(i!=nMainTotal && _main_menu_def_n[i].name != NULL){
		sprintf(szLine3,"%d.%s",++i,_main_menu_def_n[i-1].name);
		gui_text_out(0, GUI_LINE_TOP(3), szLine3);
	}
	
	if(i!=nMainTotal && _main_menu_def_n[i].name != NULL){
		sprintf(szLine4,"%d.%s",++i,_main_menu_def_n[i-1].name);
		gui_text_out(0, GUI_LINE_TOP(4), szLine4);
	}

	if(i!=nMainTotal && _main_menu_def_n[i].name != NULL){
		sprintf(szLine5,"%d.%s",++i,_main_menu_def_n[i-1].name);
		gui_text_out(0, GUI_LINE_TOP(5), szLine5);
	}
	
	gui_end_batch_paint();
}

void sdk_main_page()
{
	st_gui_message pmsg;
	static int xgui_init_flag = 0;
	int i;
	int nMainPages;
	MerchantData merchantData;
	memset(&merchantData, 0x00, sizeof(MerchantData));

	readMerchantData(&merchantData);

	unsigned int tick = Sys_TimerOpen(getCallhomeTime(90/*merchantData.callhome_time*/));

BEGIN :

	rowlup=0;

	
	if(xgui_init_flag == 0){
		xgui_init_flag = 1;
		gui_main_menu_func_add((void *)_menu_proc);	// Registration menu callback processing
		for(i = 0; i < sizeof(_menu_def)/sizeof(st_gui_menu_item_def); i ++){	// Add menu items cyclically
			gui_main_menu_item_add(_menu_def + i);	
		}
	}
	nMainTotal = sizeof(_main_menu_def_n)/sizeof(st_gui_key_menu);
	nMainPages = (nMainTotal+3) / 4;// 4 menu line for one page

	gui_post_message(GUI_GUIPAINT, 0 , 0);


	while(1){

		if (gui_get_message(&pmsg, 300) == 0) {

			if (pmsg.message_id == GUI_GUIPAINT) {

				readMerchantData(&merchantData);

				if (merchantData.is_prepped == 1)
				{
					main_page_show();
				} else {
					standby_pagepaint();
				}

			}
			else if (pmsg.message_id == GUI_KEYPRESS) {
				
				readMerchantData(&merchantData);

				if (merchantData.is_prepped == 1)
				{
					for(i=0; i < nMainTotal; i++){
						if (pmsg.wparam == _main_menu_def_n[i].uKey){
							gui_main_menu_show(_main_menu_def_n[i].name , 0);	
							gui_post_message(GUI_GUIPAINT, 0 , 0);
						}
					}
				}
				
				if ( pmsg.wparam == GUI_KEY_0)
				{
					//processing of KEY 0
					keyZeroHandler(merchantData.tid);
					gui_post_message(GUI_GUIPAINT, 0 , 0);
				}
				else if(pmsg.wparam	==	GUI_KEY_F1)
				{
					//processing of KEY F1
					if(validateUsersPin("4839")) goto BEGIN;

					gui_main_menu_show(SUPERVISION, 30000);
					gui_post_message(GUI_GUIPAINT, 0 , 0);
				}
				else if(pmsg.wparam == GUI_KEY_F2)
				{
					//processing of KEY F2
					if(validateUsersPin("4839")) goto BEGIN;

					gui_main_menu_show(MAINTENANCE, 30000);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				else if(pmsg.wparam == GUI_KEY_QUIT)
				{
					readMerchantData(&merchantData);

					if (merchantData.is_prepped == 1)
					{
						main_page_show();
					} else {
						standby_pagepaint();
					}

				}
				else if(pmsg.wparam == GUI_KEY_DOWN)
				{
					rowlup++;
					if(rowlup == nMainPages)
					rowlup=0;

					readMerchantData(&merchantData);

					if (merchantData.is_prepped == 1)
					{
						main_page_show();
					} else {
						standby_pagepaint();
					}
				}
				else if(pmsg.wparam == GUI_KEY_UP)
				{
					rowlup--;
					if(rowlup<0)
					rowlup=nMainPages-1;

					readMerchantData(&merchantData);

					if (merchantData.is_prepped == 1)
					{
						main_page_show();
					} else {
						standby_pagepaint();
					}

				}
			}
			else{
				gui_proc_default_msg(&pmsg);
			}
		}	

		// sending callhome
		if (Sys_TimerCheck(tick) <= 0)	{

			// 1. send call home data
			uiCallHome();

			// 2. reset time
			tick = Sys_TimerOpen(getCallhomeTime(90/*merchantData.callhome_time*/));
			goto BEGIN;
		}
	}
}


/*
void sdk_main_page()
{
	st_gui_message pmsg;
	static int xgui_init_flag = 0;
	char time_cur[20];
	char time_last[20];
	int i;

	unsigned int tick = Sys_TimerOpen(getCallhomeTime(5));

BEGIN :
	
	if (xgui_init_flag == 0)
	{
		xgui_init_flag = 1;
		gui_main_menu_func_add((void *)_menu_proc); // Registration menu callback processing
		for (i = 0; i < sizeof(_menu_def) / sizeof(st_gui_menu_item_def); i++)
		{ // Add menu items cyclically
			gui_main_menu_item_add(_menu_def + i);
		}
	}

	gui_post_message(GUI_GUIPAINT, 0, 0);

	while (1)
	{
		if (gui_get_message(&pmsg, 300) == 0)
		{

			if (pmsg.message_id == GUI_GUIPAINT)
			{
				isIdleState = 1;
				standby_pagepaint();
			}
			else if (pmsg.message_id == GUI_KEYPRESS)
			{
				isIdleState = 0;

				if (pmsg.wparam == GUI_KEY_OK || pmsg.wparam == GUI_KEY_QUIT)
				{
					MerchantData merchantData;

					memset(&merchantData, 0x00, sizeof(MerchantData));

					if (readMerchantData(&merchantData))
					{
						//TODO: error reading merchant params
						continue;
					}

					if (!merchantData.is_prepped)
					{
						gui_messagebox_show("Notification", "Device not ready for Eft", "", "", 7000);
						continue;
					}

					gui_main_menu_show(MAIN_MENU_PAGE, 30000);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				// else if (pmsg.wparam == GUI_KEY_1)
				// {
				// 	sdk_simple_page();
				// 	gui_post_message(GUI_GUIPAINT, 0, 0);
				// }
				else if (pmsg.wparam == GUI_KEY_F1)
				{
				
					if(validateUsersPin("4839")) goto BEGIN;

					gui_main_menu_show(SUPERVISION, 30000);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				else if (pmsg.wparam == GUI_KEY_F2)
				{
					if(validateUsersPin("4839")) goto BEGIN;

					gui_main_menu_show(MAINTENANCE, 30000);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				else if (pmsg.wparam == GUI_KEY_0)
				{
					MerchantData mParam = {0};

					if (readMerchantData(&mParam))
					{
						gui_messagebox_show("MERCHANT", "Error getting merchant details", "", "", 3000);
						return -1;
					}
					keyZeroHandler(mParam.tid);
				}
			}
			else
			{
				gui_proc_default_msg(&pmsg);
			}
		}

		// sending callhome
		if (Sys_TimerCheck(tick) <= 0)	{

			// 1. send call home data
			uiCallHome();

			// 2. reset time
			tick = Sys_TimerOpen(getCallhomeTime(5));
    	}

		Sys_GetDateTime(time_cur);
		if (strcmp(time_last, time_cur) != 0)
		{
			strcpy(time_last, time_cur);
			gui_post_message(GUI_GUIPAINT, 0, 0);
		}
	}
}
*/
