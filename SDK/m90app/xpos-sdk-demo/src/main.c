#include "libapi_xpos/inc/libapi_system.h"
#include "sdk_xgui.h"

#include "merchant.h"


static short autoHandshake(void)
{
	MerchantData merchantData;

	memset(&merchantData, 0x00, sizeof(MerchantData));
	readMerchantData(&merchantData);

	if (merchantData.is_prepped) return 0;
	return uiHandshake();
}

void app_main()
{
	//xgui_default_msg_func_add((void*)sdk_power_proc_page);	// Default message processing
	Sys_Init(0,0,"test");					// Application initializations
	EMV_iKernelInit();//Init EMV
	autoHandshake();
	sdk_main_page(); 

	// if(readMerchantData(&mParam)) return;

	// if(mParam.is_prepped == 0)
	// {
	// 	 if(getMerchantData())
	// 	{
	// 		gui_messagebox_show("MERCHANT" , "Incomplete merchant data", "" , "" , 0);
	// 		return -1; 
	// 	} 
	// }
}
#ifndef WIN32
void main(){
	app_main();

}
#endif

