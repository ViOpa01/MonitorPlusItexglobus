

#include <string.h>
#include <stdlib.h>
#include <pthread.h>


#include <stdio.h>
#include <sys/stat.h>
#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "sdk_xgui.h"
#include "nibss.h"
#include "merchant.h"
#include "callhome.h"
#include "appInfo.h"



void app_main()
{
	int checkTms = 0;
	pthread_t thread;
	MerchantData merchantData;

	memset(&merchantData, 0x00, sizeof(MerchantData));

	//xgui_default_msg_func_add((void*)sdk_power_proc_page);	// Default message processing
	Sys_Init(0,0,"itex");	// Directory that contains you file
	Sys_SetAppVer(APP_VER);

	//mf_buzzer_control(0);
	gprsInit(); //net link will have enough time to initialize in the background, we can also add little delay.	
	checkToPrepOnDownload();
	autoHandshake();

	readMerchantData(&merchantData);
	checkTms = merchantData.is_prepped ? 0 : 1;
	printf("checkTms val : %d\n", checkTms);

	Sys_tms_AppBusy(checkTms);
	EMV_iKernelInit();//Init EMV


    // pthread_create(&thread, NULL, processCallHomeAsync, NULL);
	sdk_main_page(); 

}

#ifndef WIN32
void main(){

	app_main();
}
#endif

