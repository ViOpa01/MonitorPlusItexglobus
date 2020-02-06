#ifndef MERCHANT_DATA__
#define MERCHANT_DATA__

#include "Nibss8583.h"

#define MERCHANT_DETAIL_FILE    "merchant.json"

#ifdef __cplusplus
extern "C" {
#endif

#include "network.h"

typedef struct MerchantData
{
    char address[80];
    char name[80];
    char rrn[13];
    int status;
    char stamp_label[12];
    int stamp_duty_threshold;
    int stamp_duty;
    int nibss_platform;    // 1 : EPMS, 2 : POSVAS
    char platform_label[14];
    char nibss_ip[22];
    int  nibss_ssl_port;
    int  nibss_plain_port;
    char port_type[12];
    char tid[14];
    char phone_no[14];
    short account_selection;
    short trans_type;
    short is_prepped;

    char pKey[33];
    
    Network gprsSettings;
} MerchantData;


int readMerchantData(MerchantData* merchant);
int saveMerchantData(const MerchantData* merchant);
int saveMerchantDataXml(const char* merchantXml);
int getMerchantData();


#ifdef __cplusplus
}
#endif

#endif