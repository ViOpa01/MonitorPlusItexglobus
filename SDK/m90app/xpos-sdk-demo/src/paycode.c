/**
 * File: paycode.c 
 * ---------------
 * Implements paycode.h's interface.
 * @author Opeyemi Ajani.
 */


//std
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//morefun
#include "libapi_xpos/inc/libapi_security.h"
#include "libapi_xpos/inc/libapi_gui.h"


//internal
#include "luhn.h"
#include "paycode.h"
#include "log.h"


//Just for clarity, could have reused yyyymmddhhmmss in eft struct
//Add one to the current yymm
void getPaycodeExpiry(char * yymm)
{
    char yyyymmddhhmmss[15] = {'\0'};
    char buffer[7] = {'\0'};

    Sys_GetDateTime(yyyymmddhhmmss);
    strncpy(buffer, &yyyymmddhhmmss[2], 4);

    sprintf(yymm, "%ld", atol(buffer) + 1);
}

static short BuildCardNumber(char* cardNumber, const int cardNumberSize, const char* assignedBin, const char* payCode)
{
    int len;
    int result;
    const int assignedBinLen = strlen(assignedBin);
    const int payCodeLen = strlen(payCode);
    char cardPan[] = "0000000000000000000";

    if (assignedBinLen !=  6) return -1; //Invalid Bin
    if (payCodeLen > 12) return -2; // Invalid Paycode len

    memcpy(cardPan, ASSIGNED_BANK_BIN, 6);
    memcpy(&cardPan[6], payCode, payCodeLen);

    len = strlen(cardPan);
    LOG_PRINTF("Card Pan -> %s", cardPan);
    cardPan[18] = getLuhnDigit(cardPan, len - 1) + '0';
    result = checkLuhn(cardPan, len) ? 0 : -1;

    if (result) {
        LOG_PRINTF("Card Pan -> %s", cardPan);
        gui_messagebox_show(payCode, "Error Generating No!", "", "ok", 0);
        return result;
    } 

    strncpy(cardNumber, cardPan, cardNumberSize);

    return result;
}

short buildCardNumber(char * cardNumber, const int cardNumberSize, const char * payCode)
{
    short result = BuildCardNumber(cardNumber, cardNumberSize, ASSIGNED_BANK_BIN, payCode);
    
    if (!result) {
        LOG_PRINTF("PayCode -> %s\nBIN -> %s\nCard -> %s", payCode, ASSIGNED_BANK_BIN, cardNumber);
    }

    return result;
}

static short uiGetPayCode(char * payCode, const int payCodeSize)
{
    int result = -1;

    if (payCodeSize < MAX_PAYCODE_LEN) {
        LOG_PRINTF("PayCode buffer size is too small!");
        return -1;
    }

    gui_clear_dc();

    result = Util_InputText(GUI_LINE_TOP(0), "ENTER PAYCODE", GUI_LINE_TOP(2), payCode, MIN_PAYCODE_LEN, MAX_PAYCODE_LEN, 1, 0 ,120000);

    if (result <= 0) return -3;

    if (!(result >= MIN_PAYCODE_LEN && result <= MAX_PAYCODE_LEN)) {
        //TODO: display -> error invalid paycode len
        return -3;
    }

    return 0;
}

static short uiGetPayCodeAmount(unsigned long * amount)
{
    unsigned long amt = inputamount_page("PAYCODE AMOUNT", 10, 90000);
	if (amt <= 0) return -1;
    *amount = amt;

    return 0;
}

static short confirmPayCodeData(const char * payCode, const unsigned long amount)
{
    char message[90] = {'\0'};
    int option = 0;

    sprintf(message, "PayCode: %s\nAmount: NGN %.2f", payCode, amount / 100.0);
    option = gui_messagebox_show("CONFIRM PLEASE", message, "Exit", "Confrim", 0); // Exit : 2, Confirm : 1
	return option == 1 ? 0 : -1;
}

short getPayCodePin(unsigned char pinblock[8], const char * pan)
{
	char ksn[32]={0};
	int ret;
	int pinlen = 6;
	int timeover = 30000;

	sec_set_pin_mode(1, 6);
	ret = _input_pin_page("ENTER PIN", 0, pinlen, timeover);

	if(ret == 0){
		sec_encrypt_pin_proc(SEC_MKSK_FIELD, SEC_PIN_FORMAT0, 0, pan, pinblock, 0);
	} else {
        return -1;
    }
	sec_set_pin_mode(0, 0);

    return 0;
}

short getPayCodeData(char* payCode, const int payCodeSize, unsigned long* amount)
{

    while (1) {
        if (uiGetPayCode(payCode, payCodeSize)) return -1;
        if (!uiGetPayCodeAmount(amount) && !confirmPayCodeData(payCode, *amount)) break;
    }

    return 0;
}

/**
 * Function:
 * ---------
 * transDate: YYMMDD
 * transAmount: n12
 */

void buildPaycodeIccData(char * iccData, const char transDate[7], const char transAmount[13])
{
    sprintf(iccData, "9F260831BDCBC7CFF6253B9F2701809F10120110A50003020000000000000000000000FF9F3704F435D8A29F36020527950508800000009A03%s9C01009F0206%s5F2A020566820238009F1A0205669F34034103029F3303E0F8C89F3501229F0306000000000000", transDate, transAmount);
}

