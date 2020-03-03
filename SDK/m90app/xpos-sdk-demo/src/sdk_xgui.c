#include "sdk_http.h"
#include "sdk_showqr.h"
#include "sdk_file.h"

#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_security.h"
#include "libapi_xpos/inc/libapi_emv.h"

#include "appInfo.h"
#include "util.h"
#include "network.h"
#include "menu_list.h"
#include "merchant.h"
#include "remoteLogo.h"
#include "vas/vasbridge.h"

#define LOGOIMG "xxxx\\logo.bmp"

#include "nibss.h"
#include "Nibss8583.h"
#include "EmvEft.h"

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


// Define the menu array, the first parameter is the name of the parent menu,
// the second parameter is the name of the current menu,
// and the second parameter is set when the name is duplicated.
static const st_gui_menu_item_def _menu_def[] = {

	{MAIN_MENU_PAGE, UI_CARD_PAYMENT, ""},
	{MAIN_MENU_PAGE, UI_VAS,          ""},

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
	{UI_SETTINGS, "TimeSet", ""},

	/*
	* Demo menus
	{"Others", "View AID", ""},
	{"Others", "View CAPK", ""},
	*/

	{SUPERVISION, UI_REPRINT, ""},
	{SUPERVISION, UI_EOD, ""},
	{SUPERVISION, UI_NETWORK, ""},
	{SUPERVISION, UI_DOWNLOAD_LOGO, ""},
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

void aboutTerminal(char *tid)
{
	int key = UUTIL_TIMEOUT;

	MerchantData mParam = {'\0'};
	readMerchantData(&mParam);

	while (key != GUI_KEY_QUIT)
	{
		//TODO: Start your application
		char data[32] = {0};
		gui_begin_batch_paint();
		gui_clear_dc();
		if (key == UUTIL_TIMEOUT)
		{
			sprintf(data, "Terminal SN:");
		}

		gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(1), data);

		getTerminalSn(data);
		gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(2), data);
		sprintf(data, "TID: %s", tid);
		gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(3), data);
		gui_text_out((gui_get_width() - gui_get_text_width(mParam.platform_label)) / 2, GUI_LINE_TOP(4), mParam.platform_label);
		
		gui_end_batch_paint();

		key = Util_WaitKey(1);

		switch (key)
		{
		case GUI_KEY_QUIT:
			break;
		default:
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

static short hanshakeHandler(const char *pid)
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


//gui_get_message()
// The menu callback function, as long as all the menu operations of this function are registered,
// this function will be called, and the selected menu name will be returned.
// It is mainly determined in this function that the response menu name is processed differently.
static int _menu_proc(char *pid)
{
	int ret;
	char buff[20];
	int pos = 0;
	char msg[256];
	int acctTypeValue = -1;

	printf("pid -> %s\n", pid);

	if(!strcmp(pid, UI_CARD_PAYMENT)) {
		eftTrans(cardPaymentHandler(), SUB_NONE);
		return 0;
	}
	else if(!strcmp(pid, UI_PAYCODE)) {
		paycodeHandler();
		return 0;
	}
	if(!strcmp(pid, UI_VAS)) {
		vasTransactionTypesBridge();
		return 0;
	}
	else if (!hanshakeHandler(pid))
	{
		return 0;
	}
	else if (strcmp(pid, "Version") == 0)
	{
		sprintf(msg, "app:%s\r\n", APP_VER);
		sprintf(msg + strlen(msg), "hardware:%s\r\n", sec_get_hw_ver());
		sprintf(msg + strlen(msg), "fireware:%s\r\n", sec_get_fw_ver());
		sprintf(msg + strlen(msg), "emv:%s\r\n", EMV_GetVersion());
		gui_messagebox_show("Version", msg, "", "confirm", 0);
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
	else if (strcmp(pid, "TimeSet") == 0)
	{
		time_set_page();
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
	else if (strcmp(pid, "TMSTest") == 0)
	{
		argot_action("#1#");
	}
	else if (strcmp(pid, "View AID") == 0)
	{
		EMV_ShowAID_Prm();
	}
	else if (strcmp(pid, "View CAPK") == 0)
	{
		EMV_ShowCAPK_Prm();
	}
	else if (strcmp(pid, "M1 Test") == 0)
	{
		sdk_M1test();
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
	else if (strcmp(pid, "My Plain") == 0)
	{
		//sendAndReceiveDemoRequest(0, 80);
	}
	else if (strcmp(pid, "My Ssl") == 0)
	{
		//sendAndReceiveDemoRequest(1, 443);
	}
	else if (!strcmp(pid, UI_REPRINT_BY_DATE))
	{
		Eft eft;
		strcpy(eft.tableName, "Transactions");
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
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, ALL_TRX_TYPES);
	}
	else if (!strcmp(pid, UI_EOD_CASHADVANCE))
	{
		Eft eft = {0};
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, CASH_ADV);
	}
	else if (!strcmp(pid, UI_EOD_CASHBACK))
	{
		Eft eft = {0};
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, PURCHASE_WC);
	}
	else if (!strcmp(pid, UI_EOD_PURCHASE))
	{
		Eft eft = {0};
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, PURCHASE);
	}
	else if (!strcmp(pid, UI_EOD_PREAUTH))
	{
		Eft eft = {0};
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, PRE_AUTH);
	}
	else if (!strcmp(pid, UI_EOD_COMPLETION))
	{
		Eft eft = {0};
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, PRE_AUTH_C);
	}
	else if (!strcmp(pid, UI_EOD_REVERSAL))
	{
		Eft eft = {0};
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, REVERSAL);
	}
	else if (!strcmp(pid, UI_EOD_REFUND))
	{
		Eft eft = {0};
		strcpy(eft.tableName, "Transactions");
		getListOfEod(&eft, REFUND);
	}
	else if (!strcmp(pid, UI_ACCNT_SELECTION))
	{
		int ret = enableAndDisableAccountSelection();
		printf("Enable / Disable ret : %d\n", ret);
	} 
	else if (!strcmp(pid, UI_TRANS_TYPE))
	{
		int ret = enableAndDisableOtherTrans();
		printf("Enable / Disable ret : %d\n", ret);
	}
	else if (!strcmp(pid, UI_NOTIF_ID))
	{
		MerchantData mParam = {'\0'};
		readMerchantData(&mParam);

		if(mParam.notificationIdentifier[0]) {
			gui_messagebox_show("Notification ID", mParam.notificationIdentifier, "", "", 3000);
		} else {
			gui_messagebox_show("Notification ID", "Empty notification identifier", "", "", 3000);
		}

	}
	else if(!strcmp(pid, UI_REBOOT))
	{
		Sys_Reboot();
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
				printf("Image donloaded successfully\n");
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



static short validateUsersPin()
{
	char pin[5] = {'\0'};
	int result = 0;

	gui_clear_dc();
	if((result = Util_InputText(GUI_LINE_TOP(0), "ENTER PIN", GUI_LINE_TOP(2), pin, 4, 4, 0, 2, 10000)) == 4)
	{
		printf("Password : %s, ret : %d\n", pin, result);
		if(!strncmp(pin, "4839", 4)) 
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
	int logowidth;
	int logoheight;
	int logoleft;
	int logotop;
	char *pbmp;
	char f1Msg[] = "Operator";
	char f2Msg[] = "Admin";
	char spaceRequired[35] = {'\0'};

	gui_begin_batch_paint();
	gui_clear_dc();

	logoleft = 10;
	logotop = 36;

	// pbmp = gui_load_bmp("data//logo.bmp"/*LOGOIMG*/, &logowidth, &logoheight);
	// pbmp = gui_load_bmp("itex//bank.bmp"/*BANKLOGO*/, &logowidth, &logoheight);

	
	/*
	if (pbmp != 0)
	{
		// printf("==============\n");
		gui_out_bits(logoleft, logotop, pbmp, logowidth, logoheight, 0);
		free(pbmp);
	}
	*/
	
	get_yyyymmdd_str(data);
	data[10] = ' ';
	data[11] = ' ';
	get_hhmmss_str(&data[12]);
	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(3), data);
	
	sprintf(data, "Version:%s", APP_VER);
	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(4), data);

	pos = 32 - strlen(f1Msg) - strlen(f2Msg);
	memset(spaceRequired, ' ', pos);

	pos = sprintf(data, "%s%s%s", f1Msg, spaceRequired, f2Msg);
	data[pos] = 0;

	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(5), data);
	
	gui_end_batch_paint();
}

void sdk_main_page()
{
	st_gui_message pmsg;
	static int xgui_init_flag = 0;
	char time_cur[20];
	char time_last[20];
	int i;

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
				standby_pagepaint();
			}
			else if (pmsg.message_id == GUI_KEYPRESS)
			{
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

					gui_main_menu_show(MAIN_MENU_PAGE, 0);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				// else if (pmsg.wparam == GUI_KEY_1)
				// {
				// 	sdk_simple_page();
				// 	gui_post_message(GUI_GUIPAINT, 0, 0);
				// }
				else if (pmsg.wparam == GUI_KEY_F1)
				{
				
					if(validateUsersPin()) goto BEGIN;

					gui_main_menu_show(SUPERVISION, 0);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				else if (pmsg.wparam == GUI_KEY_F2)
				{
					if(validateUsersPin()) goto BEGIN;

					gui_main_menu_show(MAINTENANCE, 0);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
			}
			else
			{
				gui_proc_default_msg(&pmsg);
			}
		}

		Sys_GetDateTime(time_cur);
		if (strcmp(time_last, time_cur) != 0)
		{
			strcpy(time_last, time_cur);
			gui_post_message(GUI_GUIPAINT, 0, 0);
		}
	}
}
