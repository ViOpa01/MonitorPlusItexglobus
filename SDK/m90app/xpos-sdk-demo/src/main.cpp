

#include <string.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
extern "C"{
#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "sdk_xgui.h"
#include "nibss.h"
#include "merchant.h"
}
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
	std::string table = "transaction";
	char minDateTrim[] = "2020-01-11";
	char maxDateTrim[] = "2020-01-12";
	std::string sql = "SELECT sum(" DB_AMOUNT " + " DB_ADDITIONAL_AMOUNT ") FROM " + table + " WHERE strftime('%Y-%m-%d', " DB_DATE ") BETWEEN '" + std::string(minDateTrim) + "' and '" + std::string(maxDateTrim) + "' and " DB_MTI " NOT LIKE '04%' and " DB_RESP " = '00' ";
	
		printf( "%s ",sql.c_str());
		
	
	
	
	//xgui_default_msg_func_add((void*)sdk_power_proc_page);	// Default message processing
	Sys_Init(0,0,"test");					// Application initializations
	EMV_iKernelInit();//Init EMV
	autoHandshake();
	sdk_main_page(); 
}
#ifndef WIN32
int main(){
	app_main();

}
#endif

