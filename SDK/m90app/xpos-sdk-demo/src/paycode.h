/**
 * File: paycode.h
 * ---------------
 * Defines a new interface for verve paycode.
 * @author Opeyemi Ajani.
 */


#ifndef _VERVE_PAYCODE_INCLUDED_
#define _VERVE_PAYCODE_INCLUDED_

#ifdef __cplusplus
extern "C"
{
#endif

#define PTAD_CODE "10"
#define PAYCODE_EXPIRY "2102"
#define ASSIGNED_BANK_BIN "506179"
#define MIN_PAYCODE_LEN 9
#define MAX_PAYCODE_LEN 12

/* I'll treat this as purchase and cashadvance respectively
#define PAYCODE_CASHIN_TRANSCODE "00"
#define PAYCODE_CASHOUT_TRANSCODE "01"
*/


short buildCardNumber(char * cardNumber, const int cardNumberSize, const char * payCode);
short getPayCodePin(unsigned char pinblock[8], const char * pan);
short getPayCodeData(char* payCode, const int payCodeSize, unsigned long* amount);
void buildPaycodeIccData(char * iccData, const char transDate[7], const char transAmount[13]);

#ifdef __cplusplus
}
#endif

#endif 
