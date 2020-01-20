#ifndef RECEIPT__H
#define RECEIPT__H

#include "merchant.h"

#ifdef __cplusplus
extern "C" {
#endif

/*typedef struct __st_card_info{
	char title[32];
	char pan[32];
	char amt[32];
	char expdate[32];
}st_card_info;
*/

int printEftReceipt(Eft *eft);
void printHandshakeReceipt(MerchantData *mParam);
void reprintByRrn(void);


#ifdef __cplusplus
}
#endif

#endif
