#ifndef VAS_VIRTUALTID_H
#define VAS_VIRTUALTID_H


#ifdef __cplusplus
extern "C" {
#endif

int getVirtualPinBlock(unsigned char* virtualPinBlock, const unsigned char* actualPinBlock, const char* pinkey);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
typedef int bool;
#endif

bool virtualConfigurationIsSet();
int  resetVirtualConfiguration();
void resetVirtualConfigurationAsync();

int itexIsMerchant();
int switchMerchantToVas(void* merchant);
// short getUpWithrawalField53(/*char vtSecurity[97]*/char (&vtSecurity)[97]);
short getUpWithrawalField53(char vtSecurity[97]);

#endif
