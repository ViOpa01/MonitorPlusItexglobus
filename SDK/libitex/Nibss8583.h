/**
 * File: Nibss8583Generic.h
 * ------------------------
 * Defines a new interface for building Nibss Iso8583 packets.
 * @author Opeyemi Ajani.
 */

#ifdef __cplusplus
extern "C"
{
#endif


#ifndef _NIBSS_ISO_8583_INCLUDED
#define _NIBSS_ISO_8583_INCLUDED

#include "Nibss8583Data.h"

/* Byte 1, Index 0 */
#define  INPUT_ONLINE_RESP                0x01  // OnlineResult
#define  INPUT_ONLINE_AC                  0x02  // AuthResp, DE 39
#define  INPUT_ONLINE_AUTHDATA            0x04  // AuthData, tag91
#define  INPUT_ONLINE_SCRIPTCRIT          0x08  // ScriptCritData, tag 71
#define  INPUT_ONLINE_SCRIPTUNCRIT        0x10  // ScriptUnCritData, tag 72
#define  INPUT_ONLINE_AUTHCODE            0x20  // AuthorizationCode, 
#define  INPUT_ONLINE_RESULT_REFERRAL     0x40  // Result_referral
#define  INPUT_ONLINE_ARC_REFERRAL        0x80  // AuthResp_Referral

typedef struct HostType
{
  char             OnlineResult;           ///< Shows whether or not an online dialogue was successful.  If there is no connection or the message MAC is wrong or the response contains a format error, then @c FALSE must be entered.  The kernel then performs a TAC-IAC-Default check and the second GenerateAC.  Other data is only then relevant, when the online dialogue was successful (value @c TRUE). @n mandatory @n Validity bit: #INPUT_ONL_ONLINE_RESP @n TLV tag #TAG_DF50_ONL_RES
  unsigned char    AuthResp[2];            ///< Authorisation Response Code. The response code from the host. Note: The format must be converted from numeric to alphanumeric @n mandatory @n Can also be changed by EMV_CT_updateTxnTags() @n Validity bit: #INPUT_ONL_ONLINE_AC @n TLV tag #TAG_8A_AUTH_RESP_CODE
  unsigned char    LenAuth;                ///< Length of @c AuthData @n Validity bit: #INPUT_ONL_AUTHDATA @n TLV length for #TAG_DF52_AUTH_DATA
  unsigned char    AuthData[512];               ///< Issuer Authentication Data (EMVCo tag 91) @n "91xx" must be included @n Can also be changed by EMV_CT_updateTxnTags() @n Validity bit: #INPUT_ONL_AUTHDATA @n TLV length for #TAG_DF52_AUTH_DATA
  unsigned short   LenScriptCrit;          ///< Length of @c ScriptCritData @n Validity bit: #INPUT_ONL_SCRIPTCRIT @n TLV length for #TAG_DF53_SCRIPT_CRIT
  unsigned char*  ScriptCritData[512];         ///< Issuer Script Template 1 (critical scripts to be performed prior to 2nd Generate AC) @n Data must consist of one or several scripts encapsulated in EMVCo tag @c 71 @n Validity bit: #INPUT_ONL_SCRIPTCRIT @n TLV length for #TAG_DF53_SCRIPT_CRIT
  unsigned short   LenScriptUnCrit;        ///< Length of @c ScriptUnCritData @n Validity bit: #INPUT_ONL_SCRIPTUNCRIT @n TLV length for #TAG_DF54_SCRIPT_UNCRIT
  unsigned char   ScriptUnCritData[512];       ///< Issuer Script Template 2 (non critical scripts to be performed after 2nd Generate AC) @n Data must consist of one or several scripts encapsulated in EMVCo tag @c 72 @n Validity bit: #INPUT_ONL_SCRIPTUNCRIT @n TLV length for #TAG_DF54_SCRIPT_UNCRIT
  unsigned char    AuthorizationCode[6];   ///< Value given by the card issuer for an accepted transaction. @n mandatory @n Validity bit: #INPUT_ONL_AUTHCODE @n TLV tag #TAG_89_AUTH_CODE
  char             Result_referral;        ///< Merchant response in case of voice referral. @n @c True ... bank approved @n @c False ... bank declined @n Validity bit: #INPUT_ONL_RESULT_REFERRAL @n TLV tag #TAG_DF51_ISS_REF_RES
  char             AuthResp_Referral[2];   ///< host AC for an issuer referral. @n Validity bit: #INPUT_ONL_ARC_REFERRAL @n TLV tag #TAG_DF55_AC_ISS_REF
  unsigned char    unKnownData[512];
  unsigned short unKnownDataLen;

  unsigned char iccData[511]; //asc
  unsigned short iccDataLen;

  unsigned char    Info_Included_Data[1];  ///< Which data is included in the message, see @ref DEF_INPUT_ONLINE
} HostType;

     enum TransType
    {
        EFT_TRANS_START,
#define TRANS_ENTRY(transType, transCode, requestType, transMti, transCategory, transTitle) transType,
        TRANSACTION_DETAILS_INCLUDE
#undef TRANS_ENTRY
            EFT_TRANS_END
    };

      enum AccountType
    {
        ACCOUNT_START,
#define ACCOUNT_ENTRY(accountType, accountCode, acountTitle) accountType,
        ACCOUNT_DETAILS_INCLUDE
#undef ACCOUNT_ENTRY
            ACCOUNT_END
    };

        enum TechMode
    {
#define TECHNOLOGY_MODE_ENTRY(techMode, entryMode, dataCode) techMode,
        TECHNOLOGY_MODE_INCLUDE
#undef TECHNOLOGY_MODE_ENTRY
    };

    enum NetworkManagementType
    {
#define MANAGEMENT_ENTRY(type, mti, code, title) type,
        NETWORK_MANAGEMENT_INCLUDE
#undef MANAGEMENT_ENTRY
    };

    enum ReversalReason
    {
#define REVERSAL_REASON_ENTRY(type, code) type,
        REVERSAL_REASONS_INCLUDE
#undef REVERSAL_REASON_ENTRY
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
        char iccData[511];
        char echoData[256];
        short isFallback;
        char secondaryMessageHashValue[64];
        short batchNumber;
        short sequenceNumber;

        //For reversal, completion
        enum ReversalReason reversalReason;
        char originalMti[5];
        char originalStan[7];
        char originalAmount[12];
        char originalYyyymmddhhmmss[15];

        //Others
        char sessionKey[33];

        char tableName[30];
        char dbName[30];

    } Eft;


    typedef struct NetworkKey
    {
        char clearKey[33];
        char encryptedKey[33];
        char checkValue[7];
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

        char clearMasterKey[33]; //output

        //Xor or component key one and two
        char clearPtadKey[33]; //Key for decrypting encrypted master key, clear master key will be return if this key is present, otherwise encrypted will be returned.

    } NetworkManagement;

    enum CommsStatus
    {
        SEND_RECEIVE_SUCCESSFUL,
        CONNECTION_FAILED,
        SENDING_FAILED,
        RECEIVING_FAILED,

        //Extended Response
        MANUAL_REVERSAL_NEEDED,
        AUTO_REVERSAL_SUCCESSUL, //Returns when the device initiate auto reversal due to
    };

    int createIsoEftPacket(unsigned char *isoPacket, const int size, const Eft *eft);
    int createIsoNetworkPacket(unsigned char *isoPacket, const int size, const NetworkManagement *networkMangement);
    int extractNetworkManagmentResponse(NetworkManagement *networkMangement, const char *response);
    int getEftOnlineResponse(HostType * hostType, Eft * eft, unsigned char * response, const int size);

#ifdef __cplusplus
}
#endif

#endif
