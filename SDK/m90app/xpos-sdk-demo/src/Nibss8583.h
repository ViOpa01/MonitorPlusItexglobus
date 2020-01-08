/**
 * File: Nibss8583.h
 * ------------------------
 * Defines a new interface for nibss iso 8583
 * @author Opeyemi Ajani.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _NIBSS_ISO_8583_INCLUDED
#define _NIBSS_ISO_8583_INCLUDED

#define  INPUT_ONLINE_RESP                0x01  // OnlineResult
#define  INPUT_ONLINE_AC                  0x02  // AuthResp, DE 39
#define  INPUT_ONLINE_AUTHDATA            0x04  // AuthData, tag91
#define  INPUT_ONLINE_SCRIPTCRIT          0x08  // ScriptCritData, tag 71
#define  INPUT_ONLINE_SCRIPTUNCRIT        0x10  // ScriptUnCritData, tag 72
#define  INPUT_ONLINE_AUTHCODE            0x20  // AuthorizationCode, tag 8A
#define  INPUT_ONLINE_RESULT_REFERRAL     0x40  // Result_referral
#define  INPUT_ONLINE_ARC_REFERRAL        0x80  // AuthResp_Referral

    typedef struct HostType
    {
        char OnlineResult;
        unsigned char AuthResp[2];
        unsigned char LenAuth;
        unsigned char AuthData[512];
        unsigned short LenScriptCrit;
        unsigned char *ScriptCritData[512];
        unsigned short LenScriptUnCrit;
        unsigned char ScriptUnCritData[512];
        unsigned char AuthorizationCode[6];
        int AuthorizationCodeLen;
        char Result_referral;
        char AuthResp_Referral[2];
        unsigned char unKnownData[512];
        unsigned short unKnownDataLen;

        unsigned char iccData[511];
        unsigned short iccDataLen;

        unsigned char iccDataBcd[256];
        unsigned short iccDataBcdLen;

        unsigned char Info_Included_Data[1];
    } HostType;

    enum TransType
    {
        EFT_TRANS_START,

        EFT_PURCHASE,
        EFT_CASHADVANCE,
        EFT_BALANCE,
        EFT_PREAUTH,
        EFT_CASHBACK,
        EFT_REFUND,
        EFT_COMPLETION,
        EFT_REVERSAL,

        EFT_TRANS_END
    };

    enum AccountType
    {
        ACCOUNT_START,

        SAVINGS_ACCOUNT,
        CURRENT_ACCOUNT,
        CREDIT_ACCOUNT,
        DEFAULT_ACCOUNT,
        UNIVERSAL_ACCOUNT,
        INVESTMENT_ACCOUNT,

        ACCOUNT_END
    };

    enum TechMode
    {

        CONTACTLESS_MODE,
        CONTACTLESS_MAGSTRIPE_MODE,
        CHIP_MODE,
        MAGSTRIPE_MODE,
        FALLBACK_MODE,
        UNKNOWN_MODE,

    };

    enum NetworkManagementType
    {

        MASTER_KEY,
        SESSION_KEY,
        PARAMETERS_DOWNLOAD,
        CALL_HOME,
        PIN_KEY,
        UNKNOWN_NETWORK,

    };

    enum ReversalReason
    {

        TIMEOUT_WAITING_FOR_RESPONSE,
        CUSTOMER_CANCELLATION,
        CHANGE_DISPENSED,
        CARD_ISSUER_UNAVAILABLE,
        UNDER_FLOOR_LIMIT,
        PIN_VERIFICATION_FAILURE,
        IOU_RECEIPT_PRINTED,
        OVER_FLOOR_LIMIT,
        NEGATIVE_CARD,
        UNSPECIFIED_NO_ACTION_TAKEN,
        COMPLETED_PARTIALLY,

    };

    typedef struct Eft
    {
        enum TransType transType;
        enum AccountType fromAccount;
        enum AccountType toAccount;
        enum TechMode techMode;
        char transCategory[32];
        char cardHolderName[35];
        char cardLabel[35];
        char cryptogram[35];
        char aid[32];
        char pan[20];
        char amount[13];
        char additionalAmount[13];
        char yyyymmddhhmmss[15];
        char stan[7];
        char expiryDate[7];
        char tvr[12];
        char tsi[12];
        char merchantType[5];
        char cardSequenceNumber[4];
        char posConditionCode[3];
        char posPinCaptureCode[3];
        char forwardingInstitutionIdCode[12];
        char track2Data[39];
        char rrn[13];
        char authorizationCode[7];
        char responseCode[3];
        char responseDesc[256];
        char serviceRestrictionCode[4];
        char terminalId[9];
        char merchantId[16];
        char merchantName[41];
        char currencyCode[5];
        char pinData[17];
        unsigned char pinDataBcd[16];
	    int pinDataBcdLen;

        char iccData[511];
        unsigned char iccDataBcd[256];
        unsigned int iccDataBcdLen;
        char echoData[256];
        short isFallback;
        char secondaryMessageHashValue[64];
        short batchNumber;
        short sequenceNumber;


        char message[256];

        enum ReversalReason reversalReason;
        char originalMti[5];
        char originalStan[7];
        char originalAmount[12];
        char originalYyyymmddhhmmss[15];

        char sessionKey[33];

        char tableName[30];
        char dbName[30];

    } Eft;

    typedef struct NetworkKey
    {
        char clearKey[33];
        char encryptedKey[33];
        char checkValue[7];

        unsigned char clearKeyBcd[16];
        unsigned char encryptedKeyBcd[16];
    } NetworkKey;

    typedef struct MerchantParameters
    {
        char ctmsDateAndTime[15];
        char cardAcceptiorIdentificationCode[16];
        char timeout[3];
        char currencyCode[5];
        char countryCode[5];
        char callHomeTime[3];
        char merchantNameAndLocation[41];
        char merchantCategoryCode[5];
    } MerchantParameters;

    typedef struct NetworkManagement
    {
        enum NetworkManagementType type;
        char terminalId[9];
        char stan[7];
        char yyyymmddhhmmss[15];

        char serialNumber[22];
        char appVersion[32];
        char deviceModel[33];
        char callHOmeData[1000];
        char commsName[33];

        char responseCode[3];
        char responseDesc[256];

        NetworkKey masterKey;
        NetworkKey sessionKey;
        NetworkKey pinKey;
        MerchantParameters merchantParameters;

        char clearMasterKey[33];

        char clearPtadKey[33];

    } NetworkManagement;

    

    unsigned int transTypeToCode(const enum TransType transType);
    char * transTypeToTitle(const enum TransType transType);

    int createIsoEftPacket(unsigned char *isoPacket, const int size, const Eft *eft);
    int createIsoNetworkPacket(unsigned char *isoPacket, const int size, const NetworkManagement *networkMangement);
    int extractNetworkManagmentResponse(NetworkManagement *networkMangement, const char *response, const int size);
    int getEftOnlineResponse(HostType *hostType, Eft *eft, unsigned char *response, const int size);

#ifdef __cplusplus
}
#endif

#endif
