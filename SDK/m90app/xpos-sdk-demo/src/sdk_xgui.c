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

#define LOGOIMG "xxxx\\logo2.bmp"

#include "nibss.h"
#include "Nibss8583.h"

// Define the menu array, the first parameter is the name of the parent menu,
// the second parameter is the name of the current menu,
// and the second parameter is set when the name is duplicated.
static const st_gui_menu_item_def _menu_def[] = {

	{MAIN_MENU_PAGE, UI_PURCHASE, ""},
	{MAIN_MENU_PAGE, UI_CASHBACK, ""},
	{MAIN_MENU_PAGE, UI_PREAUTH, ""},
	{MAIN_MENU_PAGE, UI_COMPLETION, ""},
	{MAIN_MENU_PAGE, UI_REVERSAL, ""},
	{MAIN_MENU_PAGE, UI_REFUND, ""},
	{MAIN_MENU_PAGE, UI_CASHADVANCE, ""},
	{MAIN_MENU_PAGE, UI_BALANCE, ""},

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

	{UI_SETTINGS, "Net Select", ""},
	{UI_SETTINGS, "WIFI Settings", "WIFI Menu"},
	{UI_SETTINGS, "TimeSet", ""},

	{"Others", "View AID", ""},
	{"Others", "View CAPK", ""},

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

	{UI_EOD, UI_EOD_ALL_TRANS, ""},
	{UI_EOD, UI_EOD_PURCHASE, ""},
	{UI_EOD, UI_EOD_CASHBACK, ""},
	{UI_EOD, UI_EOD_PREAUTH, ""},
	{UI_EOD, UI_EOD_COMPLETION, ""},
	{UI_EOD, UI_EOD_CASHADVANCE, ""},
	{UI_EOD, UI_EOD_REFUND, ""},
	{UI_EOD, UI_EOD_REVERSAL, ""},

	{MAINTAINANCE, UI_PREP_TERMINAL, ""},
	{MAINTAINANCE, UI_GET_PARAMETER, ""},
	{MAINTAINANCE, UI_CALL_HOME, ""},
	{MAINTAINANCE, UI_ACCNT_SELECTION, ""},
	{MAINTAINANCE, UI_TRANS_TYPE, ""},
	{MAINTAINANCE, UI_NOTIF_ID, ""},

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
		sprintf(data, "TID:");
		gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(3), data);
		//sprintf(data, "%s", mParam.tid);
		gui_text_out((gui_get_width() - gui_get_text_width(tid)) / 2, GUI_LINE_TOP(4), tid);
		// I can Print the Terminal ID here
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
	char *menuOption[] = {
		msg};

	option = gui_select_page_ex("Select", menuOption, 1, 3000, 0); // if it timesout it return -ve no else index on the menu list
	// printf("Option : %d\n", option);

	if (!option)
	{
		option = gui_messagebox_show("Message", msg, "Exit", "Confrim", 0); // Exit : 2, Confirm : 1
		if (option == 1)
		{
			mParam.account_selection = mParam.account_selection ? 0 : 1;
			saveMerchantData(&mParam);
		}
	}

	return option;
}

static short eftHandler(const char *pid)
{
	if (strcmp(pid, UI_PURCHASE) == 0)
	{
		eftTrans(EFT_PURCHASE);
	}
	else if (strcmp(pid, UI_CASHBACK) == 0)
	{
		eftTrans(EFT_CASHBACK);
	}
	else if (strcmp(pid, UI_PREAUTH) == 0)
	{
		eftTrans(EFT_PREAUTH);
	}
	else if (strcmp(pid, UI_COMPLETION) == 0)
	{
		eftTrans(EFT_COMPLETION);
	}
	else if (strcmp(pid, UI_REVERSAL) == 0)
	{
		eftTrans(EFT_REVERSAL);
	}
	else if (strcmp(pid, UI_REFUND) == 0)
	{
		eftTrans(EFT_REFUND);
	}
	else if (strcmp(pid, UI_CASHADVANCE) == 0)
	{
		eftTrans(EFT_CASHADVANCE);
	}
	else if (strcmp(pid, UI_BALANCE) == 0)
	{
		eftTrans(EFT_BALANCE);
	}
	else
	{
		return -1;
	}

	return 0;
}

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
		gui_messagebox_show("", "Merchat Record", "", "Cleared", 2000);
	}
	else
	{
		gui_messagebox_show("", "Merchat Record", "", "Error", 2000);
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

	if (!eftHandler(pid))
	{
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
	else if (!strcmp(pid, UI_ACCNT_SELECTION))
	{
		int ret = -1;
		ret = enableAndDisableAccountSelection();
		printf("Enable / Disable ret : %d\n", ret);
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

	logoleft = 30;
	logotop = 16;

	pbmp = gui_load_bmp(LOGOIMG, &logowidth, &logoheight);

	if (pbmp != 0)
	{
		gui_out_bits(logoleft, logotop, pbmp, logowidth, logoheight, 0);
		free(pbmp);
	}

	get_yyyymmdd_str(data);
	data[10] = ' ';
	data[11] = ' ';
	get_hhmmss_str(&data[12]);
	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(3), data);
	//get_hhmmss_str(data);
	//gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(4), data);

	sprintf(data, "Version:%s", APP_VER);
	gui_text_out((gui_get_width() - gui_get_text_width(data)) / 2, GUI_LINE_TOP(4), data);

	//TODO: don't hardcode 32, get it at run time
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
				else if (pmsg.wparam == GUI_KEY_1)
				{
					sdk_simple_page();
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				else if (pmsg.wparam == GUI_KEY_F1)
				{
					gui_main_menu_show(SUPERVISION, 0);
					gui_post_message(GUI_GUIPAINT, 0, 0);
				}
				else if (pmsg.wparam == GUI_KEY_F2)
				{
					gui_main_menu_show(MAINTAINANCE, 0);
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
