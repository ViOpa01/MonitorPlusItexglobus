

#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "sdk_xgui.h"
#include "nibss.h"
#include "merchant.h"

static short autoHandshake(void)
{
	MerchantData merchantData;

	memset(&merchantData, 0x00, sizeof(MerchantData));
	readMerchantData(&merchantData);

	if (merchantData.is_prepped) return 0;
	return uiHandshake();
}
#define DB_AMOUNT "amount"
#define DB_ADDITIONAL_AMOUNT "additional amount"
#define DB_DATE "date"
#define DB_MTI "mti"
#define DB_RESP "resp"
void app_main()
{

	
	
	//xgui_default_msg_func_add((void*)sdk_power_proc_page);	// Default message processing
	Sys_Init(0,0,"test");					// Application initializations
	EMV_iKernelInit();//Init EMV
	autoHandshake();
	sdk_main_page(); 
}
#ifndef WIN32
void main(){
	app_main();

}
#endif

