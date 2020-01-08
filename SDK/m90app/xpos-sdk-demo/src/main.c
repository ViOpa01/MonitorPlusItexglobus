#include "libapi_xpos/inc/libapi_system.h"
#include "sdk_xgui.h"

// #include "merchant.h"



void app_main()
{
	//xgui_default_msg_func_add((void*)sdk_power_proc_page);	// Default message processing
	Sys_Init(0,0,"test");					// Application initialization
	EMV_iKernelInit();//Init EMV
	sdk_main_page(); 
}
#ifndef WIN32
void main(){
	app_main();

}
#endif

