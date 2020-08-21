#include <stdlib.h>
#include <string.h>

#include "unistarwrapper.h"
#include <stdio.h>
#include "driver/mf_icc.h"

extern "C"
{
#include "util.h"
#include "libapi_xpos/inc/libapi_util.h"
#include "libapi_xpos/inc/libapi_iccard.h"
#include "libapi_xpos/inc/libapi_gui.h"
#include "libapi_xpos/inc/libapi_system.h"
#include "libapi_xpos/inc/libapi_emv.h"
#include "libapi_xpos/inc/libapi_print.h"
}

#include "Receipt.h"
#include "simpio.h"

int apduSendToBcd(unsigned char *bcd, const size_t bcdSize, const APDU_SEND *apduSend)
{
    const int headerSize = 4;
    int index = 0;

    // Unsigned short is at least 2 bytes, apdu standard is up to 3 bytes long
    // So I'll assume a single byte Lc
    if (!bcd || !apduSend || apduSend->Lc > 255)
    {
        return -1;
    }

    if (bcdSize < headerSize + apduSend->Lc + apduSend->Le)
    {
        return -1;
    }

    memcpy(bcd, apduSend->Command, 4);
    index += 4;

    if (apduSend->Lc > 0)
    {
        bcd[index++] = (unsigned char)apduSend->Lc;
        memcpy(&bcd[index], apduSend->DataIn, apduSend->Lc);
        index += apduSend->Lc;
    }

    bcd[index++] = (unsigned char)(apduSend->Le);

    return index;
}

static void bcdToAsc(char *asc, const unsigned char *bcd, const int size)
{
    int i;
    short pos;

    for (i = 0; i < size; i++)
    {
        pos += sprintf(&asc[pos], "%02X", bcd[i]);
    }
}

int bcdToApduResp(APDU_RESP *apduResp, const unsigned char *bcd, const size_t bcdLen, const unsigned char tag61Val)
{
    if (tag61Val && bcdLen != (size_t)tag61Val + 2)
    {
        return -1;
    }
    else if (bcdLen < 2)
    {
        return -1;
    }

    apduResp->SWA = bcd[bcdLen - 2];
    apduResp->SWB = bcd[bcdLen - 1];

    const int len = bcdLen - 2;
    if (len > 0)
    {
        apduResp->LenOut = len * 2;
        bcdToAsc((char *)apduResp->DataOut, bcd, len);
    }

    return 0;
}

int cardPostion2SlotCB(int *slot, const enum TCardPosition cardPos)
{
    int ret;

    if (!slot)
    {
        return -1;
    }

    switch (cardPos)
    {
    case CP_TOP:
        *slot = SLOT_ICC_SOCKET1;
        ret = 0;
        break;

    case CP_BOTTOM:
    case CP_BOTTOM2:
    case CP_BOTTOM3:
        *slot = SLOT_ICC_SOCKET2;
        ret = 0;
        break;

    default:
        ret = -1;
    }

    return ret;
}

int detectPsam(void)
{
    int ret = -1;

    ret = Icc_Open(SLOT_ICC_SOCKET2);

    if (ret)
    {
        printf("Error(%d): Can't Open card on slot %d\n", ret, SLOT_ICC_SOCKET2);
        return -1;
    }

    ret = Icc_GetCardStatus(SLOT_ICC_SOCKET2) == UICC_OK ? 0 : -1;

    Icc_Close(SLOT_ICC_SOCKET2);

    return 0;
}

int resetCardCB(int slot, unsigned char *adatabuffer)
{
    int len = 0;
    int result = -1;
    unsigned char ATR[64] = {0};
    int i;

    printf("%s slot -> %d\n", __FUNCTION__, slot);

    if (UICC_OK != Icc_GetCardStatus(slot))
    {
        printf("Customer card removed, Error(%d): Error resetting card...., slot -> %d\r\n", result, slot);
        result = Icc_Open(slot);
        if (UICC_OK != result)
        {
            Icc_Close(slot);
            result = Icc_Open(slot);
        }

        if (result)
        {
            printf("Error(%d): Can't Open card on slot %d\n", result, slot);
            return -1;
        }
    }

    result = Icc_GetCardATR(UICC_CPUCARD, slot, adatabuffer, &len);


    if (result)
    {
        printf("Error(%d): Error resetting card...., slot -> %d\r\n", result, slot);
        return -2;
    }

    adatabuffer[len] = 0;
    printf("%s slot success, atr(%d) -> %s\n", __FUNCTION__, len, adatabuffer);

    puts("\r\nART Return -> ");
    for (i = 0; i < len; i++)
    {
        printf("%02X", adatabuffer[i]);
    }
    puts("\r\n");

    return 0;
}

int closeSlotCB(int slot)
{
    if (/*Icc_Close(slot)*/ Icc_Close(slot))
    {
        printf("Error closing slot %d\n", slot);
        return -1;
    }

    return 0;
}

#if 0
typedef struct
{
	//	byte OperType;	/* See enum - OperationType */
	byte CLA;			/* Class byte of the command message*/
	byte INS;			/* Instrution*/
	byte P1;			/* Parameter1*/
	byte P2;			/* Parameter2*/
	byte Lc;			/* length of being sent data*/
	byte Le;			/* length of expected data,Actual return value*/
	byte SW1;			/* status word1*/
	byte SW2;			/* status word2*/
	byte C_Data[256];	/* command data*/
	byte R_Data[256];	/* response data*/
} ICCAPDU;

typedef struct
{
  unsigned char Command[4];
  unsigned short Lc;
  unsigned char DataIn[512];
  unsigned short Le;
} APDU_SEND;

typedef struct
{
  unsigned short LenOut;
  unsigned char DataOut[512];
  unsigned char SWA;
  unsigned char SWB;
} APDU_RESP;
#endif

void logIccApdu(const ICCAPDU *sdkApdu, const short isResponse)
{
    int i = 0;

    if (isResponse)
    {
        puts("APDU RESPONSE: ");
        printf("SW1 -> %02X\n", sdkApdu->SW1);
        printf("SW2 -> %02X\n", sdkApdu->SW2);
        printf("LE -> %02X:%d\n", sdkApdu->Le, sdkApdu->Le);

        printf("Response Data -> ");
        for (i = 0; i < sdkApdu->Le; i++)
        {
            printf("%02X", sdkApdu->R_Data[i]);
        }
    }
    else
    {
        puts("APDU REQUEST: ");

        printf("CLA -> %02X\n", sdkApdu->CLA);
        printf("INS -> %02X\n", sdkApdu->INS);
        printf("P1 -> %02X\n", sdkApdu->P1);
        printf("P2 -> %02X\n", sdkApdu->P2);
        printf("LC -> %02X\n", sdkApdu->Lc);
        printf("LE -> %02X\n", sdkApdu->Le);

        printf("Command -> ");
        for (i = 0; i < sdkApdu->Lc; i++)
        {
            printf("%02X", sdkApdu->C_Data[i]);
        }
    }
    puts("\n");
}

static short apduSendToSdkApdu(ICCAPDU *sdkApdu, const APDU_SEND *apduSend)
{
    memset(sdkApdu, 0x00, sizeof(ICCAPDU));

    sdkApdu->CLA = apduSend->Command[0];
    sdkApdu->INS = apduSend->Command[1];
    sdkApdu->P1 = apduSend->Command[2];
    sdkApdu->P2 = apduSend->Command[3];
    sdkApdu->Lc = apduSend->Lc;
    sdkApdu->Le = apduSend->Le;

    memcpy(sdkApdu->C_Data, apduSend->DataIn, apduSend->Lc);
}

/*
return: UICC_NORF         = -4,   // No Card
		UICC_FAIL        = -1,   // Fail
		UICC_OK            =  0    // Success
*/

char iccIsoCommandCB(int slot, APDU_SEND *apduSend, APDU_RESP *apduRecv)
{
    //LogManager log(LOG_CHANNEL);
    unsigned char apduBCD[512] = {0}; // Header and command
    int ret = 1;
    ICCAPDU sdkApdu;

    unsigned short responseLen = 0;
    unsigned char response[256 + 2] = {0}; // Data and response trailer (SW1-SW2)

    unsigned char tag61Val = 0;

    apduSendToSdkApdu(&sdkApdu, apduSend);

    logIccApdu(&sdkApdu, 0);
    ret = Icc_ICComm(UICC_CPUCARD, slot, &sdkApdu);

    if (ret < 0)
    {
        switch (ret)
        {
        case UICC_NORF:
            printf("No card\n");
            return -1;

        case UICC_FAIL:
            printf("Fail\n");
            return -2;

        default:
            printf("Unknow error %d\n", ret);
            return -3;
        }
    }

    if (sdkApdu.SW1 == 0x61)
    {
        sdkApdu.CLA = 0x00;
        sdkApdu.INS = 0xC0;
        sdkApdu.P1 = 0x00;
        sdkApdu.P2 = 0x00;
        sdkApdu.Le = sdkApdu.SW2; // Length of pending response

        ret = Icc_ICComm(UICC_CPUCARD, slot, &sdkApdu);

        if (ret < 0)
        {
            switch (ret)
            {
            case UICC_NORF:
                printf("No card\n");
                return -1;

            case UICC_FAIL:
                printf("Fail\n");
                return -2;

            default:
                printf("Unknow error %d\n", ret);
                return -3;
            }
        }

        /*
    apduLen = bcdToApduResp(apduRecv, response, responseLen, tag61Val);
    if (apduLen != 0) {
        LOGF_INFO(LogManager(LOG_CHANNEL).handle, "%s: %s", __FUNCTION__, ctStatusString(status));
        return '1';
    }
    */
    }

    //sdkApdu.Le = ret; //Just for logging.
    logIccApdu(&sdkApdu, 1);

    apduRecv->SWA = sdkApdu.SW1;
    apduRecv->SWB = sdkApdu.SW2;
    memcpy(apduRecv->DataOut, sdkApdu.R_Data, ret);
    apduRecv->LenOut = ret;
    /*
        apduRecv->LenOut = ret * 2;

        BCDToASC(apduRecv->DataOut, sdkApdu.R_Data, ret);
        printf("Raw Data -> %s\r\n", sdkApdu.R_Data);
        //bcdToAsc((char *)apduRecv->DataOut, sdkApdu.R_Data, ret);
        */

    return 0;
}
void getDateTimeBcdCB(unsigned char yymdhmsBcd[7])
{
    char yyyymmddhhmmss[15] = {'\0'};

    getDateAndTime(yyyymmddhhmmss);
    Util_Asc2Bcd(yyyymmddhhmmss, (char *)yymdhmsBcd, strlen(yyyymmddhhmmss));
}

int printCustomerCardInfo(void *smartCardInfo, const char *logoPath)
{
    unsigned char ret;
    char strTemp[512];
    char chrTemp;
    double dTemp;
    char buffer[512];
    La_Card_info *cardinfo = (La_Card_info *)smartCardInfo;

    gui_begin_batch_paint();
    gui_clear_dc();
    gui_text_out(0, GUI_LINE_TOP(0), "printing...");
    gui_end_batch_paint();

    ret = UPrint_Init();

    if (ret == UPRN_OUTOF_PAPER)
    {
        if (gui_messagebox_show("Print", "No paper \nDo you wish to reload Paper?", "cancel", "confirm", 0) != 1)
        {
            return -1;
        }
    }

    UPrint_SetDensity(3); //Set print density to 3 normal

    if (logoPath)
        printReceiptLogo(logoPath); // Print Logo

    UPrint_StrBold("CARD INFO", 1, 4, 1);
    UPrint_Feed(12);

    memset(strTemp, 0, sizeof(strTemp));
    memcpy(strTemp, cardinfo->CM_Card_SerNo, 16);
    printLine("Card Ser No:", strTemp);

    memset(strTemp, 0, sizeof(strTemp));
    //03-22
    memcpy(strTemp, cardinfo->CM_UserNo, 17);

    printLine("User No:", strTemp);

    chrTemp = cardinfo->CM_Card_Status;

    memset(strTemp, 0, sizeof(strTemp));

    switch (chrTemp)
    {
    case 1:
        sprintf(strTemp, "%s", "Creat Account");
        //prnStr("Creat Account");
        break;
    case 2:
        sprintf(strTemp, "%s", "Purchase");
        //prnStr("Purchase");
        break;
    case 3:
        sprintf(strTemp, "%s", "Load Power");
        //prnStr("Load Power");
        break;
    default:
        sprintf(strTemp, "%d", chrTemp);
        //prnStr("%d",chrTemp);
        break;
    }

    printLine("Card Status:", strTemp);

    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%d", cardinfo->CM_Purchase_Times);

    printLine("Purchase Times", buffer);

    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%0.9f", cardinfo->CM_Purchase_Power);

    printLine("Purchase Power", buffer);

    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%0.9f", cardinfo->CP_Remain_Auth);

    printLine("Remain Auth", buffer);

    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%0.9f", cardinfo->CM_Alarm_L1);

    printLine("Alarm1", buffer);

    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%0.9f", cardinfo->CM_Alarm_L2);

    printLine("Alarm2", buffer);

    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%0.9f", cardinfo->CM_Alarm_L3);

    printLine("Alarm3", buffer);

    printLine("Sale Date", (char *)cardinfo->CM_Purchase_DateTime);
    //printFooter();

    UPrint_Str((char *)"\n--------------------------------\n", 2, 1);

    UPrint_Str("--------------END---------------\n", 2, 1);

    UPrint_Str((char *)"--------------------------------\n", 2, 1);

    ret = UPrint_Start(); // Output to printer
    if (getPrinterStatus(ret) < 0)
    {
        return -2;
    }

    return 0;
}

void removeCustomerCard(const char *title, const char *desc)
{
    while (Icc_GetCardStatus(SLOT_ICC_SOCKET1) == UICC_OK)
    {
        Demo_SplashScreen(title, desc);
        Util_Beep(2);
    }

    Icc_Close(SLOT_ICC_SOCKET1);
}



static int DetectSmartCard(char *title, char *msg, int timeoverMs)
{
    st_gui_message pMsg;
    unsigned int quitTick = Sys_TimerOpen(timeoverMs);

    gui_post_message(GUI_GUIPAINT, 0, 0);

    while (1)
    {
        //puts("-----------------------------> in loop");
        if (Sys_TimerCheck(quitTick) == 0)
        { // Detection timeout
            gui_messagebox_show("ERROR", "Timeout!", "", "Ok", 0);
            return READ_CARD_RET_TIMEOVER;
        }
        if (gui_get_message(&pMsg, 10) == 0)
        {
            //puts("----------------------------------> get_message");
            if (pMsg.message_id == GUI_GUIPAINT)
            {
                gui_begin_batch_paint();
                gui_clear_dc();
                gui_text_out(0, 0, title);
                gui_text_out(0, GUI_LINE_TOP(2), msg);
                gui_end_batch_paint();
            }
            else if (pMsg.message_id == GUI_KEYPRESS)
            {
                //puts("----------------------------------> key press");
                if (pMsg.wparam == GUI_KEY_QUIT)
                {
                    gui_messagebox_show("ERROR", "Cancel!", "", "Ok", 0);
                    return READ_CARD_RET_CANCEL;
                }
            }
            else
            {
                //puts("------------------------> default message");
                gui_proc_default_msg(&pMsg); // Let the system handle some common messages
            }
        }
        else
        {
            //puts("------------------------> last else");
            if (Icc_GetCardStatus(SLOT_ICC_SOCKET1) == UICC_OK) //(emvapi_check_ic() == 1 )
            {
                //puts("------------------------> IC card returned");                            // Detecting ic card
                return READ_CARD_RET_IC;
            }
        }
    }
    gui_messagebox_show("ERROR", "Cancel!", "", "Ok", 0);
    return READ_CARD_RET_CANCEL;
}

static int detectSmartCard(char *title, char *msg, int timeoverMs)
{
    const int slot = Icc_Open(SLOT_ICC_SOCKET1);
    const int ret = DetectSmartCard(title, msg, timeoverMs);
    Icc_Close(slot);

    if (ret == READ_CARD_RET_IC) return 0;
    return -1;
}

int bindSmartCardApi(SmartCardInFunc *smartCardInFunc)
{
    smartCardInFunc->cardPostion2Slot = cardPostion2SlotCB;
    smartCardInFunc->resetCard = resetCardCB;
    smartCardInFunc->closeSlot = closeSlotCB;
    smartCardInFunc->exchangeApdu = iccIsoCommandCB;
    smartCardInFunc->getDateTimeBcd = getDateTimeBcdCB;
    smartCardInFunc->detectPsamCB = detectPsam;
    smartCardInFunc->printCustomerCardInfoCb = printCustomerCardInfo;
    smartCardInFunc->removeCustomerCardCb = removeCustomerCard;
    smartCardInFunc->detectSmartCardCb = detectSmartCard;
    //smartCardInFunc->isDebug = 1; // only for development purpose

    return 0;
}

int customerCardInserted(char *title, char *desc)
{
    int duration = 0;
    const int interval = 200000; // 5 times a second

    gui_begin_batch_paint();

    gui_clear_dc();
    gui_text_out_ex(0, GUI_LINE_TOP(0), title);
    gui_text_out((gui_get_width() - gui_get_text_width(desc)) / 2, gui_get_height() / 2, desc);
    gui_end_batch_paint();

    /* TODO
    Demo_SplashScreen(title, desc);
    while (1) {
        int ret = EMV_CT_SmartDetect(EMV_CT_CUSTOMER);
        if (ret == EMV_ADK_SMART_STATUS_OK) {
            break;
        } else if (duration >= 10000000) {
            return 0;
        }

        usleep(interval);
        duration += interval;
    }
    */

    return 1;
}

void logCardInfo(La_Card_info *userCardInfo)
{
    printf("\r\nCM_Card_SerNo: '%s', len: %d", (char *)userCardInfo->CM_Card_SerNo, strlen((char *)userCardInfo->CM_Card_SerNo));
    printf("\r\nCD_UserName: '%s', len: %d", (char *)userCardInfo->CD_UserName, strlen((char *)userCardInfo->CD_UserName));
    printf("\r\nCM_UserNo: '%s', len: %d", (char *)userCardInfo->CM_UserNo, strlen((char *)userCardInfo->CM_UserNo));
    printf("\r\nCM_Purchase_DateTime: '%s', len: %d", (char *)userCardInfo->CM_Purchase_DateTime, strlen((char *)userCardInfo->CM_Purchase_DateTime));
    printf("\r\nCM_Card_Status: '%c'", userCardInfo->CM_Card_Status);
    printf("\r\nCM_Purchase_Times: %d", userCardInfo->CM_Purchase_Times);
    printf("\r\nCM_Purchase_Power: %g", userCardInfo->CM_Purchase_Power);
    printf("\r\nCM_Alarm_L1: %g", userCardInfo->CM_Alarm_L1);
    printf("\r\nCM_Alarm_L2: %g", userCardInfo->CM_Alarm_L2);
    printf("\r\nCM_Alarm_L3: %g", userCardInfo->CM_Alarm_L3);
    printf("\r\nCM_Load_Thrashold: %f", userCardInfo->CM_Load_Thrashold);
    printf("\r\nCM_User_Demand: %g", userCardInfo->CM_User_Demand);
    printf("\r\nCM_Arrearage_Months: %d", userCardInfo->CM_Arrearage_Months);
    printf("\r\nCM_Arrearage_Amount: %g", userCardInfo->CM_Arrearage_Amount);
    printf("\r\nCF_MeterStatus: '%c'", userCardInfo->CF_MeterStatus);
    printf("\r\nCF_Remain_Power: %g", userCardInfo->CF_Remain_Power);
    printf("\r\nCF_TotalUsed_Power: %g", userCardInfo->CF_TotalUsed_Power);
    printf("\r\nCF_Load_Times: %d", userCardInfo->CF_Load_Times);

    //Other data from La_Card_info, which readUserCard isn't using.
    printf("\r\nCM_Card_Ver: '%c'", userCardInfo->CM_Card_Ver);
    printf("\r\nCF_Meter_ID: '%s', len: %d", (char *)userCardInfo->CF_Meter_ID, strlen((char *)userCardInfo->CF_Meter_ID));
    printf("\r\nCF_Refound_Power: %g", userCardInfo->CF_Refound_Power);

    printf("\r\nCF_TotalLoad_Power: %g", userCardInfo->CF_TotalLoad_Power);
    printf("\r\nCF_Unreg_TotalPower: %g", userCardInfo->CF_Unreg_TotalPower);
    printf("\r\nCF_Over_Power: %g", userCardInfo->CF_Over_Power);
    printf("\r\nCF_PreLoad_Power: %g", userCardInfo->CF_PreLoad_Power);
    printf("\r\nCF_PreLoad_DateTime: '%s', len: %d", (char *)userCardInfo->CF_PreLoad_DateTime, strlen((char *)userCardInfo->CF_PreLoad_DateTime));
    printf("\r\nCF_CurLoad_Power: %g", userCardInfo->CF_CurLoad_Power);
    printf("\r\nCF_CurLoad_DateTime: '%s', len: %d", (char *)userCardInfo->CF_CurLoad_DateTime, strlen((char *)userCardInfo->CF_CurLoad_DateTime));
    printf("\r\nCF_LoadYN: '%c'", userCardInfo->CF_LoadYN);
    printf("\r\nCD_SerCen_No: '%s', len: %d", (char *)userCardInfo->CD_SerCen_No, strlen((char *)userCardInfo->CD_SerCen_No));

    printf("\r\nCD_TarrNo: %d", userCardInfo->CD_TarrNo);
    printf("\r\nCD_Security_Deposit: %g", userCardInfo->CD_Security_Deposit);
    printf("\r\nCD_ConnectFee: %g", userCardInfo->CD_ConnectFee);
    printf("\r\nCD_ReConnectFee: %g", userCardInfo->CD_ReConnectFee);
    printf("\r\nCD_ProFund: %g", userCardInfo->CD_ProFund);
    printf("\r\nCD_UserOldAccNo: '%s', len: %d", (char *)userCardInfo->CD_UserOldAccNo, strlen((char *)userCardInfo->CD_UserOldAccNo));
    printf("\r\nCD_Pre_Amount: %g", userCardInfo->CD_Pre_Amount);
    printf("\r\nCD_Pre_Cash: %g", userCardInfo->CD_Pre_Cash);
    printf("\r\nCD_Meter_Pur: %g", userCardInfo->CD_Meter_Pur);
    printf("\r\nCD_Rec_Months: %d", userCardInfo->CD_Rec_Months);
    printf("\r\nCP_Remain_Auth: %g", userCardInfo->CP_Remain_Auth);
}

static short getCardDataFromUser(La_Card_info *userCardInfo, const short shouldGetAlarmFromUser)
{

    unsigned char inData[512] = "0004 ";
    unsigned char outData[512] = {'\0'};
    int ret;
    char credit[23] = "0.01";
    char vendingTimes[21] = {'\0'};

    /* int getText(char *val, size_t len, int display, int timeout, const char* title, const char* prompt, UI_DIALOG_TYPE dialogType)*/

    puts("Before get Text.......1");
    if (getText((char *)userCardInfo->CM_UserNo, sizeof(userCardInfo->CM_UserNo) - 1, 0, 30000, "User Number", "", UI_DIALOG_TYPE_NONE) < 0)
    {
        return -1;
    }

    puts("After get Text.......1");

    sprintf(vendingTimes, "%d", userCardInfo->CM_Purchase_Times);

    if (getNumeric(vendingTimes, sizeof(vendingTimes) - 1, 0, 30000, "Purchase Times", "Vending Times", UI_DIALOG_TYPE_NONE) < 0)
        return -2;

    puts("Location.......................2");

    userCardInfo->CM_Purchase_Times = atoi(vendingTimes);
    userCardInfo->CM_Purchase_Power = 0.01;

    puts("Location.......................3");

    if (shouldGetAlarmFromUser)
    {
        char nextAlarm[23] = {'\0'};
        sprintf(nextAlarm, "%.2f", userCardInfo->CM_Alarm_L1);

        if (getText((char *)nextAlarm, sizeof(nextAlarm) - 1, 0, 30000, "ALARM L1", "ALARM L1", UI_DIALOG_TYPE_NONE) < 0)
        {
            return -3;
        }
        userCardInfo->CM_Alarm_L1 = strtod(nextAlarm, NULL);

        memset(nextAlarm, '\0', sizeof(nextAlarm));
        sprintf(nextAlarm, "%.2f", userCardInfo->CM_Alarm_L2);
        if (getText((char *)nextAlarm, sizeof(nextAlarm) - 1, 0, 30000, "ALARM L2", "ALARM L1", UI_DIALOG_TYPE_NONE) < 0)
        {
            return -4;
        }
        userCardInfo->CM_Alarm_L2 = strtod(nextAlarm, NULL);

        memset(nextAlarm, '\0', sizeof(nextAlarm));
        sprintf(nextAlarm, "%.2f", userCardInfo->CM_Alarm_L3);
        if (getText((char *)nextAlarm, sizeof(nextAlarm) - 1, 0, 30000, "ALARM L2", "ALARM L1", UI_DIALOG_TYPE_NONE) < 0)
        {
            return -5;
        }

        userCardInfo->CM_Alarm_L3 = strtod(nextAlarm, NULL);
    }
    else
    {
        userCardInfo->CM_Alarm_L1 = 30.00;
        userCardInfo->CM_Alarm_L2 = 20.00;
        userCardInfo->CM_Alarm_L3 = 10.00;
    }

    puts("Location.......................4");

    return 0;
}



void writeUserCardTest(void)
{
    unsigned char inData[512] = "0004 ";
    unsigned char outData[512] = {'\0'};
    La_Card_info userCardInfo;
    SmartCardInFunc smartCardInFunc;
    int ret = -1;
    unsigned char psamBalance[35] = {'\0'};

    removeCustomerCard("CUSTOMER CARD", "REMOVE CARD!");

    memset(&userCardInfo, 0, sizeof(userCardInfo));
    bindSmartCardApi(&smartCardInFunc);

    if (detectPsam())
    {
        gui_messagebox_show("ERROR", "NO PSAM!", "", "Ok", 0);
        return;
    }

    ret = detectSmartCard("INSERT CARD", "CUSTOMER CARD", 3 * 60 * 1000);

    if (ret)
    {
        return;
    }

    ret = readUserCard(&userCardInfo, outData, inData, &smartCardInFunc);

    if (ret)
    {
        gui_messagebox_show("READ ERROR", unistarErrorToString(ret), "", "Ok", 0);
        return;
    }

    removeCustomerCard("CUSTOMER CARD", "REMOVE CARD!");
    logCardInfo(&userCardInfo);

    puts("Reading Card Ok!.......");

    ret = getCardDataFromUser(&userCardInfo, 1);

    if (ret)
    {
        return;
    }

    logCardInfo(&userCardInfo);

    ret = detectSmartCard("INSERT CARD", "CUSTOMER CARD", 3 * 60 * 1000);

    if (ret)
    {
        return;
    }

    userCardInfo.CM_Purchase_Times += 1; //very important, otherwise the meter will reject the engergy
    printf("Puchase Times -> %d\n", userCardInfo.CM_Purchase_Times);

    ret = updateUserCard(psamBalance, &userCardInfo, &smartCardInFunc);

    if (ret)
    {
        gui_messagebox_show("WRITE ERROR", unistarErrorToString(ret), "", "Ok", 0);
        return;
    }

    UI_ShowButtonMessage(2 * 60 * 1000, "Write Card", "Write Card Successful", "Ok", UI_DIALOG_TYPE_CONFIRMATION);

    removeCustomerCard("CUSTOMER CARD", "REMOVE CARD!");

    printf("Psam balance -> '%s'\n", (char *)psamBalance);
}

void customerCardInfo(void)
{
    unsigned char inData[512] = "0004 ";
    unsigned char outData[512] = {'\0'};
    La_Card_info userCardInfo;
    SmartCardInFunc smartCardInFunc = {0};
    int ret = -1;

    removeCustomerCard("CUSTOMER CARD", "REMOVE CARD!");

    memset(&userCardInfo, 0, sizeof(userCardInfo));
    bindSmartCardApi(&smartCardInFunc);

    if (detectPsam())
    {
        gui_messagebox_show("ERROR", "NO PSAM!", "", "Ok", 0);
        return;
    }

    ret = detectSmartCard("INSERT CARD", "CUSTOMER CARD", 3 * 60 * 1000);

    if (ret)
    {
        return;
    }

    ret = readUserCard(&userCardInfo, outData, inData, &smartCardInFunc);

    if (ret)
    {
        gui_messagebox_show("ERROR", unistarErrorToString(ret), "", "Ok", 0);
        return;
    }

    removeCustomerCard("CUSTOMER CARD", "REMOVE CARD!");

    printCustomerCardInfo(&userCardInfo, NULL);
    logCardInfo(&userCardInfo);
}
