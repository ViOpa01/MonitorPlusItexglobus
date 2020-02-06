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
    printf("Card Pan -> %s\n", cardPan);
    cardPan[18] = getLuhnDigit(cardPan, len - 1) + '0';
    result = checkLuhn(cardPan, len) ? 0 : -1;

    if (result) {
        printf("Card Pan -> %s\n", cardPan);
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
        printf("PayCode -> %s\nBIN -> %s\nCard -> %s", payCode, ASSIGNED_BANK_BIN, cardNumber);
    }

    return result;
}

static short uiGetPayCode(char * payCode, const int payCodeSize)
{
    int result = -1;

    if (payCodeSize < MAX_PAYCODE_LEN) {
        printf("PayCode buffer size is too small!\n");
        return -1;
    }

    gui_clear_dc();

    printf("Before InputText function\n");
    result = Util_InputText(GUI_LINE_TOP(0), "ENTER PAYCODE", GUI_LINE_TOP(2), payCode, MIN_PAYCODE_LEN, MAX_PAYCODE_LEN, 1, 0 ,120000);
    printf("After input text function\n");

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

#if 0

void generateIccData(char* iccData, char* amount)
{

    char time[21] = { '\0' };
    char year[5], month[3], day[3], hour[3], minute[3], second[3];
    char transactionDate[21] = { '\0' };

    memset(time, 0, sizeof(time));
    memset(year, 0, sizeof(year));
    memset(month, 0, sizeof(month));
    memset(day, 0, sizeof(day));
    memset(hour, 0, sizeof(hour));
    memset(minute, 0, sizeof(minute));

    read_clock(time);

    time[15] = 0;
    strncpy(year, time, 4);
    strncpy(month, &time[4], 2);
    strncpy(day, &time[6], 2);
    strncpy(hour, &time[8], 2);
    strncpy(minute, &time[10], 2);
    strncpy(second, &time[12], 2);

    sprintf(transactionDate, "%s%s%s", year, month, day);

    //date: 161116
    //sprintf(iccData, "9F2608F15DC488EE91F4B49F2701809F3303E0F8E85F3401019F3501229F34034403029F10120110A78003020000329C00000000000000FF9F3704F2806EC29F36020656950500000080009A03%s9C01009F0206%s5F2A020566820238009F1A020566", amount, transactionDate);
    sprintf(iccData, "9F2608062C90D2D3EE72309F2701809F3303E0F8E85F3401009F3501229F34034203009F10200FA501A203F8060000000000000000000F0100000000000000000000000000009F3704586E1CEA9F3602001A950540802480009A031611039C01009F02060000000100005F2A020566820258009F1A020566");
    LOG_PRINTF(("Amount ===> '%s', TransactionDate ====> '%s'", amount, transactionDate));
}

/**
  * Function: vervePaycode
  * ------------------------
  * @author Ajani Opeyemi Sunday.
  */

void vervePaycode(short type)
{
    char paycode[22] = { '\0' };
    char pan[22] = { '\0' };
    EftTransactions eft = { '\0' };
    char buffer[23] = { '\0' };
    short hostDecision;
    int ret = -1;

    memset(&eft, '\0', sizeof(eft));

    eft.cTranType = type;
    eft.fromAccount = eft.toAccount = 0;
    eft.cCard = 'C';

    if (!getPaycodeData(paycode, &eft.lPurchaseAmount))
        return;

    if (eft.cTranType == TRAN_TYPE_PAYCODE_CASH_OUT) { //Withdrawal, get pin
        clearScreen();

        ret = inOnlinePINFunc();
        oLFlag = 0;

        if (ret != SUCCESS)
            return;

        sprintf(eft.pinBlock, "%s", onlinePINBlock);
    }

    sprintf(eft.cRefCode, "%s", paycode);

    generatePan(pan, ASSIGNED_BIN, paycode, PTAD_CODE);
    sprintf(eft.track2, "%sD%s601022420343", pan, PAYCODE_EXPIRY);
    sprintf(eft.CardExpDate, "%s", PAYCODE_EXPIRY);

    sprintf(eft.cRefCode, "%s", PTAD_CODE);

    generateIccData(eft.iccData, buffer);

    sprintf(eft.CardHoldername, "%s", paycode); //CardHoldername reused for paycode

    totalAmountEntered = eft.lPurchaseAmount;
    hostDecision = processIsoEft(&eft);
    totalAmountEntered = 0L;

    if (hostDecision == HOST_AUTHORISED || hostDecision == HOST_DECLINED) {
        printEftReceipt(CUSTOMER_COPY, &eft);

        clearScreen();
        textView(1, 1, LAYOUT_CENTER, LAYOUT_INVERSE, "PRINT MERCHANT COPY");
        textView(1, screenHeight / 2, LAYOUT_CENTER, LAYOUT_DEFAULT, "PRESS ANY KEY");
        textView(1, screenHeight, LAYOUT_CENTER, LAYOUT_INVERSE, unsafe_localizedString(TXO_POWERED_BY_ITEX));
        getChar(10);
        printEftReceipt(MERCHANT_COPY, &eft);

        sendEftNotification(&eft);
    }
}

void purchaseAndReversalRequest(void)
{
    Eft eft;
    const char *expectedPacket = "0200F23C46D129E09220000000000000002116539983471913195500000000000000010012201232310000071232311220210856210510010004D0000000006539983345399834719131955D210822100116217990000317377942212070HE88FBP205600444741ONYESCOM VENTURES LTD  KD           LANG566AD456A8EAC9DA12E2809F2608CBAFAFBDB481085F9F2701809F10120110A040002A0000000000000000000000FF9F3704983160FC9F360200F0950500002488009A031912209C01009F02060000000001005F2A020566820239009F1A0205669F34034203009F3303E0F8C89F3501229F1E0834363233313233368407A00000000410109F090200029F03060000000000005F340101074V240m-3GPlus~346-231-236~1.0.6(Fri-Dec-20-10:50:14-2019-)~release-30812300015510101511344101BE97C61158EDB7955608F7238DBFD07DA74483E720F5B172F36FDC35DF68CC55";
    unsigned char actualPacket[1024];
    int result = 0;

    memset(&eft, 0x00, sizeof(Eft));

    eft.transType = EFT_PURCHASE;
    eft.techMode = CHIP_MODE;
    eft.fromAccount = DEFAULT_ACCOUNT;
    eft.toAccount = DEFAULT_ACCOUNT;
    eft.isFallback = 0;

    strncpy(eft.sessionKey, "7A91384AD04C8A85CD7919C1803D8FCD", sizeof(eft.sessionKey));

    strncpy(eft.pan, "5399834719131955", sizeof(eft.pan));
    strncpy(eft.amount, "000000000100", sizeof(eft.amount));
    strncpy(eft.yyyymmddhhmmss, "20191220123231", sizeof(eft.yyyymmddhhmmss));
    strncpy(eft.stan, "000007", sizeof(eft.stan));
    strncpy(eft.expiryDate, "2108", sizeof(eft.expiryDate));
    strncpy(eft.merchantType, "5621", sizeof(eft.merchantType));
    strncpy(eft.cardSequenceNumber, "001", sizeof(eft.cardSequenceNumber));
    strncpy(eft.posConditionCode, "00", sizeof(eft.posConditionCode));
    strncpy(eft.posPinCaptureCode, "04", sizeof(eft.posPinCaptureCode));
    strncpy(eft.track2Data, "5399834719131955D21082210011621799", sizeof(eft.track2Data));
    strncpy(eft.rrn, "000031737794", sizeof(eft.rrn));
    strncpy(eft.serviceRestrictionCode, "221", sizeof(eft.serviceRestrictionCode));
    strncpy(eft.terminalId, "2070HE88", sizeof(eft.terminalId));
    strncpy(eft.merchantId, "FBP205600444741", sizeof(eft.merchantId));
    strncpy(eft.merchantName, "ONYESCOM VENTURES LTD  KD           LANG", sizeof(eft.merchantName));
    strncpy(eft.currencyCode, "566", sizeof(eft.currencyCode));
    strncpy(eft.pinData, "AD456A8EAC9DA12E", sizeof(eft.pinData));
    strncpy(eft.iccData, "9F2608CBAFAFBDB481085F9F2701809F10120110A040002A0000000000000000000000FF9F3704983160FC9F360200F0950500002488009A031912209C01009F02060000000001005F2A020566820239009F1A0205669F34034203009F3303E0F8C89F3501229F1E0834363233313233368407A00000000410109F090200029F03060000000000005F340101", sizeof(eft.iccData));
    strncpy(eft.echoData, "V240m-3GPlus~346-231-236~1.0.6(Fri-Dec-20-10:50:14-2019-)~release-30812300", sizeof(eft.echoData));

    result = createIsoEftPacket(actualPacket, sizeof(actualPacket), &eft);

    if (result <= 0)
    {
        fprintf(stderr, "Error creating purchase request\n");
    }
    else
    {
        reportResult(expectedPacket, (char *) &actualPacket[2], result - 2, "Purchase Reqeust");
    }

    //Reversal
    expectedPacket = "0420F23C46D1A9E09320000000420000002116539983471913195500000000000000010012201232310000071232311220210856210510010004D000000000653998306557694345399834719131955D210822100116217990000317377942212070HE88FBP205600444741ONYESCOM VENTURES LTD  KD           LANG566AD456A8EAC9DA12E2809F260800000000000000009F2701009F10120110A040002A0000000000000000000000FF9F3704DEC71E6A9F360200F0950504002488009A031912209C01FE9F02060000000001005F2A020566820239009F1A0205669F34034203009F3303E0F8C89F3501229F1E0834363233313233368407A00000000410109F090200029F03060000000000005F3401010044021074V240m-3GPlus~346-231-236~1.0.6(Fri-Dec-20-10:50:14-2019-)~release-30812300020000000712201232310000053998300000557694000000000100000000000100D00000000D00000000015510101511344101E426831F273F0B6992E9DD9B3FDAEB1D0FD3ABF9CF89E1BC63EE056B91796EB2";

    eft.transType = EFT_REVERSAL;
    eft.reversalReason = TIMEOUT_WAITING_FOR_RESPONSE;
    strncpy(eft.originalMti, "0200", sizeof(eft.originalMti)); //The mti of the equivalent(original) purchase transaction.
    strncpy(eft.forwardingInstitutionIdCode, "557694", sizeof(eft.forwardingInstitutionIdCode));
    strncpy(eft.originalYyyymmddhhmmss, "20191220123231", sizeof(eft.originalYyyymmddhhmmss)); //Date time when original mti trans was done
    //strncpy(eft.authorizationCode, "", sizeof(eft.authorizationCode)); //add if present.

    //ICC data changed, this means that Joel did manual reversal
    strncpy(eft.iccData, "9F260800000000000000009F2701009F10120110A040002A0000000000000000000000FF9F3704DEC71E6A9F360200F0950504002488009A031912209C01FE9F02060000000001005F2A020566820239009F1A0205669F34034203009F3303E0F8C89F3501229F1E0834363233313233368407A00000000410109F090200029F03060000000000005F340101", sizeof(eft.iccData));

    result = createIsoEftPacket(actualPacket, sizeof(actualPacket), &eft);

    if (result <= 0)
    {
        fprintf(stderr, "Error creating financial request\n");
    }
    else
    {
        reportResult(expectedPacket, (char *) &actualPacket[2], result - 2, "Reversal Request");
    }
}

#endif 


#if 0
int main(void) 
{
    puts("It works!");

    return 0;
}
#endif 
