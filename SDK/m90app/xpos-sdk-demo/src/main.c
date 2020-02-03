

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <stdio.h>
#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "sdk_xgui.h"
#include "nibss.h"
#include "merchant.h"

//#include "platform/inc/driver/mf_misc.h"

void app_main()
{
	//xgui_default_msg_func_add((void*)sdk_power_proc_page);	// Default message processing
	Sys_Init(0,0,"itex");	// Directory that contains you file
	//mf_buzzer_control(0);
	gprsInit(); //net link will have enough time to initialize in the background, we can also add little delay.				
	EMV_iKernelInit();//Init EMV
	autoHandshake();
	sdk_main_page(); 
}

#ifndef WIN32
void main(){

	app_main();

}
#endif

