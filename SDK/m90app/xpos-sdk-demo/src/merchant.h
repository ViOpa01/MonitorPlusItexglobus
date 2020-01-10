#ifndef MERCHANT_DATA__
#define MERCHANT_DATA__

#include "Nibss8583.h"

#define MERCHANT_DETAIL_FILE    "merchant.json"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MerchantData
{
    char address[80];
    char rrn[13];
    int status;
    char stamp_label[12];
    int stamp_duty_threshold;
    int stamp_duty;
    char nibss_platform[12];    // POSVAS, EPMS
    char nibss_ip[22];
    int  nibss_port;
    int  nibss_plain_port;
    char port_type[12];
    char tid[14];
    char phone_no[14];
    int account_selection;
    int is_prepped;

} MerchantData;


int readMerchantData(MerchantData* merchant);
int saveMerchantData(const MerchantData* merchant);
int saveMerchantDataXml(const char* merchantXml);
int getMerchantData();


#ifdef __cplusplus
}
#endif

#endif