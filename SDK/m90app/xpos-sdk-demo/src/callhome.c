#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "callhome.h"
#include "nibss.h"
#include "merchant.h"
#include "libapi_xpos/inc/libapi_system.h"

static void sendCallHome()
{
    NetworkManagement networkMangement;
    NetWorkParameters netParam;
    MerchantData mParam;
    char tid[9] = {'\0'};
    int maxRetry = 2;
    int i;
    char sessionKey[33] = {'\0'};

    memset(&networkMangement, 0x00, sizeof(NetworkManagement));
    memset(&mParam, 0x00, sizeof(MerchantData));
    memset(&netParam, 0x00, sizeof(NetWorkParameters));

    if(readMerchantData(&mParam)) {
        fprintf(stderr, "Error reading parameters\n");
        return;
    }

    if (!mParam.is_prepped) { //terminal not preped, parameter not allowed
        fprintf(stderr, "Terminal has not been prepped\n");
        return;
    }

    if(!mParam.tid[0]) {
        fprintf(stderr, "Terminal ID is empty\n");
        return;
    }

    strncpy(tid, mParam.tid, strlen(mParam.tid));
    strncpy(networkMangement.terminalId, tid, sizeof(networkMangement.terminalId));

    getSessionKey(sessionKey);
    memcpy(networkMangement.sessionKey.clearKey, sessionKey, strlen(sessionKey));

    getNetParams(&netParam, CURRENT_PLATFORM, 0);
    netParam.async = 1;

    addCallHomeData(&networkMangement);
        
    for (i = 0; i < maxRetry; i++)
    {
        if (!sCallHomeAsync(&networkMangement, &netParam))
            break;
    }

    if (i == maxRetry) {
        fprintf(stderr, "Call Home failed\ns");
        return;
    }

    Util_Beep(2);
    return 0;

}

int static getCallhomeTime()
{
    MerchantParameters parameters;
    int tm = 0;
    int callhomeTime = 1 * 60 * 60 * 1000;

    memset(&parameters, 0x00, sizeof(MerchantParameters));
    if (getParameters(&parameters))
	{
		printf("Error getting parameters\n");
		return callhomeTime;
	}

    tm = atoi(parameters.callHomeTime) * 60 * 60 * 1000;

    return tm ? callhomeTime : tm;
}

void processCallHomeAsync(pthread_t *thread)
{
    MerchantData mParam;
    // pthread_t dialThread;
    ssize_t* status = (ssize_t*)0;


    memset(&mParam, 0x00, sizeof(MerchantData));
	readMerchantData(&mParam);

    // pthread_create(&dialThread, NULL, preDial, &mParam.gprsSettings);
    // pthread_join(&dialThread, (void**)&status);

    // if(status != 0) {
    //     // predial fail
    //     return;
    // }

	unsigned int tick = Sys_TimerOpen((unsigned int)getCallhomeTime());

    while (1)
    {
        if (Sys_TimerCheck(tick) <= 0)	{
			// 1. send call home data
            pthread_create(thread, NULL, sendCallHome, NULL);

            // 2. reset time
            tick = Sys_TimerOpen((unsigned int)getCallhomeTime());
		}

        if((Sys_TimerCheck(tick) % 1000) == 0)
        {
            printf("callhome remaining time : %d sec.\n", Sys_TimerCheck(tick) / 1000);
        }
    }
    

}