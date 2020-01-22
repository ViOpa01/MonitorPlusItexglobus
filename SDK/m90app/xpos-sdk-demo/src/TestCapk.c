/**
 * File: TestCapk.c 
 * ----------------
 * Implements TestCapk.h's interface.
 */

#include <stdio.h>

#include "TestCapk.h"
#include "libapi_xpos/inc/def.h"
#include "emvapi/inc/emv_api.h"
//#include "libapi_pub.h"


//TODO: Visit https://www.eftlab.com/knowledge-base/243-ca-public-keys/ to get the test keys
//Clone CapkHelper project on git lab to create all test test keys for Visa, Master card and CUP, you'll see the ones for live keys that I did there.
//Add the generated CAPK structures here.
//Note that you can use the radio button at eftlab to select each scheme e.g master card.
//Please check LiveCapk.c for more details.

struct{
	const char * label;
	CAPUBLICKEY * keyValue;
}AllTestEmvKey[]={
    //TODO: Install all the CAPK generated above here, see LiveCapk.c for more details.
    {NULL, NULL}, //TODO: I'm a time bomb, please remove me asap.
    //Visa card

    //Master card

    //China Union Pay

    //Verve card
    //{"capk_verve_t05", &capk_verve_t05},
};


#define ALL_LIVE_CAPK_SIZE sizeof(AllTestEmvKey) / sizeof(AllTestEmvKey[0])

void injectTestCapks(void)
{
	int i;
    const int size = ALL_LIVE_CAPK_SIZE;

	for(i = 0; i < size; i++)
	{
        if (EMV_PrmSetCAPK(AllTestEmvKey[i].keyValue)) {
            fprintf(stderr, "Error injecting test key %s\n", AllTestEmvKey[i].label);
        }
	}
}


