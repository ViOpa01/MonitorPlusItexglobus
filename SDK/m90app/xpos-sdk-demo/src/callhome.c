#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "callhome.h"
#include "nibss.h"
#include "merchant.h"
#include "libapi_xpos/inc/libapi_system.h"

extern isIdleState;

static int sendCallHome()
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
        return -1;
    }

    if (!mParam.is_prepped) { //terminal not prepped, parameter not allowed
        fprintf(stderr, "Terminal has not been prepped\n");
        return -1;
    }

    if(!mParam.tid[0]) {
        fprintf(stderr, "Terminal ID is empty\n");
        return -1;
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
        return -1;
    }

    return 0;

}

unsigned int static getCallhomeTime()
{
    MerchantParameters parameters;
    int tm = 0;
    unsigned int callhomeTime = 1 * 60 * 60 * 1000;

    memset(&parameters, 0x00, sizeof(MerchantParameters));
    if (getParameters(&parameters))
	{
		printf("Error getting parameters\n");
		return callhomeTime;
	}

    tm = atoi(parameters.callHomeTime) * 60 * 60 * 1000;

    return tm ? callhomeTime : tm;
}

void processCallHomeAsync()
{
    MerchantData mParam;
    pthread_t dialThread;
    ssize_t* status = (ssize_t*)0;


    memset(&mParam, 0x00, sizeof(MerchantData));
	readMerchantData(&mParam);

	unsigned int tick = Sys_TimerOpen(getCallhomeTime());

    while (1)
    {
        if (Sys_TimerCheck(tick) <= 0)	{

            // 1. send callhome only in idle state of the Terminal
            if(!isIdleState) {
                goto resetTimer;  
            }

			// 2. send call home data
            pthread_create(&dialThread, NULL, preDial, &mParam.gprsSettings);
            if(sendCallHome()) {
                printf("Callhome failed\n");
                Util_Beep(1);   
            } else {
                gui_messagebox_show("Callhome", "Success", "", "", 2000);
                printf("Callhome succesful\n");
                Util_Beep(2);
            }

            // 3. reset time
            resetTimer :
            tick = Sys_TimerOpen(getCallhomeTime());
		}

    }
    

}