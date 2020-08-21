#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "CpuCard_Fun.h"
#include "basefun.h"
//#include "Debug.h"
//#include "Applib.h"




#define MAX_R_W_BINFILE_LEN  0xb2


/********************** external reference declaration *********************/
extern signed int  Bcd2DoubleEXT(unsigned char *buf,unsigned char lbreverse,unsigned char length,unsigned char lidiglen,double *out);
extern int Double2BCD(double Dec, unsigned char *Bcd, int length,int iDig,int iSig);
extern unsigned char  AddCheckSum(unsigned int inLen,unsigned char *inData);
extern signed int WriteCard(int aiddev,unsigned int iFileID, unsigned int iOffset, unsigned int iLen,unsigned char iSig, unsigned char *strData);
extern signed int decrease(int aiddev,double adcharge, const SmartCardInFunc * smartCardInFunc);
extern void ShowHexString( int iRow, unsigned char *pBuf, int iLen );
/********************** Internal variables declaration *********************/


#if 0 //Variability
short getNextCardField(char * nextField, int expectedLen, const char * rawField, unsigned char * header)
{
	int len;
	char fieldStr[195] = {'\0'};

	//LOG_PRINTF(("%s:::::Raw Field => '%s', expected len %d", __FUNCTION__, rawField, expectedLen)); 

	strncpy(fieldStr, rawField, strlen(rawField));

	//TODO pad(nextField, fieldStr, ' ', expectedLen, LEFT);
	
	len = strlen(nextField);

	if (len != expectedLen) {
		error(header, "\nCan't Process %s\nExpected %d, got %d", rawField, expectedLen, len);
		return -1;
	}

	return 0;
}
#endif


#if 0 //Variability
int PrintRecord(La_Card_info * cardinfo)
{
	unsigned char ret;
	unsigned char strTemp[512];
	unsigned char chrTemp;
	double dTemp;
	
	char buffer[43];
    
    getPrnFontWidth(&prnWidth, &alignFont);

	//logEftType(__FUNCTION__, eftTrans);

	hPrinter = openPrinter();

	printScreen("EFT RECEIPT", "...PRINTING...");

	printEftSlipHeader(NULL);

	centerPrint(PRN_2X_HEIGHT_INVERSE, PRN_WIDTH_24, "CARD INFO");
	
	
	memset(strTemp,0,sizeof(strTemp));
	memcpy(strTemp,cardinfo->CM_Card_SerNo,16);
	alignPrint(alignFont, prnWidth, "Card Ser No:", strTemp);
	

	memset(strTemp,0,sizeof(strTemp));
	//03-22
	memcpy(strTemp,cardinfo->CM_UserNo,17);

	alignPrint(alignFont, prnWidth, "User No:", strTemp);
	
	chrTemp = cardinfo->CM_Card_Status;

	memset(strTemp,0,sizeof(strTemp));

	switch(chrTemp)
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

	alignPrint(alignFont, prnWidth, "Card Status:", strTemp);

	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%d", cardinfo->CM_Purchase_Times);

	alignPrint(alignFont, prnWidth, "Purchase Times", buffer);

	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%0.9f", cardinfo->CM_Purchase_Power);

	alignPrint(alignFont, prnWidth, "Purchase Power", buffer);

	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%0.9f", cardinfo->CP_Remain_Auth);

	alignPrint(alignFont, prnWidth, "Remain Auth", buffer);


	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%0.9f", cardinfo->CM_Alarm_L1);

	alignPrint(alignFont, prnWidth, "Alarm1", buffer);


	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%0.9f", cardinfo->CM_Alarm_L2);

	alignPrint(alignFont, prnWidth, "Alarm2", buffer);


	memset(buffer, '\0', sizeof(buffer));
	sprintf(buffer, "%0.9f", cardinfo->CM_Alarm_L3);

	alignPrint(alignFont, prnWidth, "Alarm3", buffer);


	alignPrint(alignFont, prnWidth, "Sale Date", cardinfo->CM_Purchase_DateTime);
	
	
	waitForPrinter(hPrinter);

	Print((char *)"\n--------------------------------\n");

	Print("--------------END---------------\n");

	Print((char *)"--------------------------------\n\n\n\n\n");
	
	closePrinter(hPrinter);
	
	return 0;
}
#endif

//int PrintRecord(La_Card_info * cardinfo);


//��������Դ���ֽ��˫�����ȿɶ���16���ƴ�,0x12AB-->"12AB"
//Convert BIN string to readable HEX string, which have double length of BIN string. 0x12AB-->"12AB"
void PubBcd2Asc(unsigned char *psIn, unsigned int uiLength, unsigned char *psOut)
{
    static const unsigned char ucHexToChar[17] = {"0123456789ABCDEF"};
    unsigned int   uiCnt;

    for(uiCnt = 0; uiCnt < uiLength; uiCnt++)
    {
        psOut[2*uiCnt]   = ucHexToChar[(psIn[uiCnt] >> 4)];
        psOut[2*uiCnt + 1] = ucHexToChar[(psIn[uiCnt] & 0x0F)];
    }
}

// ͬvOneTwo()����,����Ŀ�괮����һ '\0'
//Similar with function PubOne2Two(),add '\0' at the end of object string
void PubBcd2Asc0(unsigned char *psIn, unsigned int uiLength, unsigned char *pszOut)
{
    PubBcd2Asc(psIn, uiLength, pszOut);
    pszOut[2*uiLength] = 0;
}


int ReadUserCard(unsigned char *inData,unsigned char *outData, const SmartCardInFunc * smartCardInFunc)
{
	int liret;
	//unsigned long llret;
	//unsigned char rlen;
	//unsigned char ret;
	int aiddev;
	double dtemp;
	/*struct */La_Card_info LUserCi;
	char i;
	//unsigned char initbuf[30];
	unsigned char StrRetData[256]={0},StrTemp[128]={0},portname[5]={0},StrTemp2[12]={0},StrCardSerNo[256]={0},strCardReset[33]={1};

	double result; //Mod => to fix issue with compiler errors
	
	memset(outData,' ',226);
	memset(StrCardSerNo,0,sizeof(StrCardSerNo));
	memset(StrRetData,0,sizeof(StrRetData));
	//analysis inData

	memcpy(portname,inData,5);

	printf("%s -------------------------> 1\r\n", __FUNCTION__);

	liret=smartCardInFunc->detectPsamCB();
	if (liret!=0)
	{
	   return -11; //No Psam card
	}

	printf("%s -------------------------> 2\r\n", __FUNCTION__);


    liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
    if (liret!=0)
    	return -1;

	printf("%s -------------------------> 3\r\n", __FUNCTION__);

    liret = smartCardInFunc->resetCard(aiddev,strCardReset);
    if (liret!=0)
    	return -2;

	printf("%s -------------------------> 4\r\n", __FUNCTION__);
// #ifdef Debug
// 	ShowHexString(2,strCardReset,strCardReset[ 0 ] + 1);
// 	getkey();
// #endif

	asc2hex(strCardReset+5+1,8,StrCardSerNo,16); //Get Card seial no //StrRetDate[0] is length so is 5+1
 	StrCardSerNo[16]='\0';
	memcpy(LUserCi.CM_Card_SerNo,StrCardSerNo,32);
	memcpy(outData,LUserCi.CM_Card_SerNo,32);
//////////////////////////////////////////////////////////////////////////
	//03-22
	//To read out the psam card .No
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_BOTTOM);
	if (liret!=0)
	{
		return -1;
	}

	printf("%s -------------------------> 5\r\n", __FUNCTION__);
	
	liret = smartCardInFunc->resetCard(aiddev,StrRetData);
	if (liret!=0)
	{
		return -2;
	}

	printf("%s -------------------------> 6\r\n", __FUNCTION__);
	
	asc2hex(StrRetData+5+1,8,LUserCi.CD_UserName,16);
	LUserCi.CD_UserName[16] = '\0';

	printf("Username -> %s\n", LUserCi.CD_UserName);
	
	memcpy(outData+131,LUserCi.CD_UserName, 32);

	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
	if (liret!=0)
	{
		return -1;
	}

	printf("%s -------------------------> 7\r\n", __FUNCTION__);
	
//////////////////////////////////////////////////////////////////////////

	liret = IsSysCard(aiddev, smartCardInFunc);

	if (liret!=0)
		return -3;

	printf("%s -------------------------> 8\r\n", __FUNCTION__);

	liret = InternalAuth(aiddev, smartCardInFunc);

	if (liret!=0)
		return -4;

	printf("%s -------------------------> 9\r\n", __FUNCTION__);

	/**************************************************/
	/*             Read User Card File                */
	/**************************************************/
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
	if (liret!=0)
		return -35;

	printf("%s -------------------------> 10\r\n", __FUNCTION__);

	liret = SelectFile(aiddev,0x3f01, smartCardInFunc);
	if (liret!=0)
		return -6;

	printf("%s -------------------------> 11\r\n", __FUNCTION__);

	//Read File info
	liret = ReadCard(aiddev,1,12,46,1,StrRetData, smartCardInFunc);
	if (liret!=0)
		return -7;

	printf("%s -------------------------> 12\r\n", __FUNCTION__);

	asc2hex(StrRetData,46,StrTemp,46*2);
	//03-22 
	memcpy(LUserCi.CM_UserNo,StrRetData,17); //UserNo
	LUserCi.CM_UserNo[17]='\0';
	if (LUserCi.CM_UserNo[0]==0xFF)//Not initialize user card
    {
		return -22;
	}    

	memcpy(outData+47,LUserCi.CM_UserNo,18);
   

	memset(LUserCi.CM_Purchase_DateTime,' ',15);
	memcpy(LUserCi.CM_Purchase_DateTime,StrTemp+48,14);// the datetime of last purchase
  

	memcpy(outData+32,LUserCi.CM_Purchase_DateTime,15);

	memcpy(StrTemp,StrRetData+17,1);
	StrTemp[1] = '\0';
	LUserCi.CM_Card_Status =StrTemp[0];  //card status
	if ((StrTemp[0]&0x20)!=0)
		return -12;  //wrong status
	asc2hex(StrTemp,1,StrTemp2,2);
	memcpy(outData+65,StrTemp2,2);   

	Bcd2DoubleEXT(StrRetData+18,1,2,0,&(dtemp));//the times of purchase
	LUserCi.CM_Purchase_Times=(unsigned short)dtemp;
	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-4d",LUserCi.CM_Purchase_Times);  
	memcpy(outData+67,StrTemp,4);

	Bcd2DoubleEXT(StrRetData+20,1,4,2,&(LUserCi.CM_Purchase_Power)); //last power purchased 

	if (LUserCi.CM_Purchase_Power==0)  //Zero Power
	{
		return -21;
	}  

	printf("%s -------------------------> 13\r\n", __FUNCTION__);

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",LUserCi.CM_Purchase_Power);
	memcpy(outData+71,StrTemp,9);

	Bcd2DoubleEXT(StrRetData+31,1,4,2,&(LUserCi.CM_Alarm_L1)); //Alarm_L1

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",LUserCi.CM_Alarm_L1);
	memcpy(outData+80,StrTemp,9);

	Bcd2DoubleEXT(StrRetData+35,1,4,2,&(LUserCi.CM_Alarm_L2));//Alarm_L2

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",LUserCi.CM_Alarm_L2);
	memcpy(outData+89,StrTemp,9);

	Bcd2DoubleEXT(StrRetData+39,1,4,2,&(LUserCi.CM_Alarm_L3));//Alarm_L3

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",LUserCi.CM_Alarm_L3);
	memcpy(outData+98,StrTemp,9);

	result = 0;

	Bcd2DoubleEXT(StrRetData+43,1,4,2,&result);
	LUserCi.CM_Load_Thrashold = result;

    result = 0;
	Bcd2DoubleEXT(StrRetData+45,1,4,2,&result);
	LUserCi.CM_User_Demand = result;

	result = 0;
	Bcd2DoubleEXT(StrRetData+49,1,4,2,&result);
	LUserCi.CM_Arrearage_Months = result;

	result = 0;
	Bcd2DoubleEXT(StrRetData+50,1,4,2,&result);
	LUserCi.CM_Arrearage_Amount = result;

	//fill the file blank
	for(i=0;i<107;i++)
	{
		if (*(outData+i)=='\0')
			*(outData+i)=' ';   
	}
	outData[107]='\0';

	liret = ReadCard(aiddev,2,0,85,1,StrRetData, smartCardInFunc);
	if (liret!=0)
		return -7; //03-22

	printf("%s -------------------------> 14\r\n", __FUNCTION__);

	//get Meter Status
	LUserCi.CF_MeterStatus =   StrRetData[28];

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-2X",LUserCi.CF_MeterStatus);
	memcpy(outData+107,StrTemp,2);
	//by jarod 2012.04.12
	if (StrRetData[28]==0xff && smartCardInFunc->isDebug)
	{
		return -9;

	}
   
   
	Bcd2DoubleEXT(StrRetData+29,1,2,0,&(dtemp));//Load times
	LUserCi.CF_Load_Times=(unsigned short)dtemp;

	Bcd2DoubleEXT(StrRetData+31,1,4,2,&(LUserCi.CF_Remain_Power));
	Bcd2DoubleEXT(StrRetData+35,1,4,2,&(LUserCi.CF_TotalUsed_Power));

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-4d",LUserCi.CF_Load_Times);  
	memcpy(outData+109,StrTemp,4);//Load times

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",LUserCi.CF_Remain_Power); 
	memcpy(outData+113,StrTemp,9);

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",LUserCi.CF_TotalUsed_Power); 
	memcpy(outData+122,StrTemp,9);

	smartCardInFunc->closeSlot(aiddev);
	//PrintRecord(&LUserCi);
	return 0;
}


/**
 * Function: readUserCard
 * Usage: int ret = readUserCard(&userCardInfo, outData, inData);
 * --------------------------------------------------------------
 * @param userCardInfo Struct containing customer card's info, see La_Card_info.
 * @param outData String representation of the customer card's info.
 * @param inData Command to read customer card's info from customer card.
 *
 * @author Ajani Opeyemi Sunday.
 * Remodified ReadUserCard in order to pass out La_Card_info struct from readUserCard.
 * @Data 9/4/2017
 */

int readUserCard(La_Card_info * userCardInfo, unsigned char *outData, const unsigned char *inData, const SmartCardInFunc * smartCardInFunc)
{
	int liret;
	//unsigned long llret;
	//unsigned char rlen;
	//unsigned char ret;
	int aiddev;
	double dtemp;
	char i;
	//unsigned char initbuf[30];
	unsigned char StrRetData[256]={0},StrTemp[128]={0},portname[5]={0},StrTemp2[12]={0},StrCardSerNo[256]={0},strCardReset[33]={1};
	double result;
	

	//LOG_PRINTF(("In Data => '%s'", inData));

	memset(userCardInfo, '\0', sizeof(La_Card_info));
	memset(outData,' ',226);
	memset(StrCardSerNo,0,sizeof(StrCardSerNo));
	memset(StrRetData,0,sizeof(StrRetData));
	//analysis inData

	memcpy(portname,inData,5);

	liret=smartCardInFunc->detectPsamCB();
	if (liret!=0)
	{
	   //LOG_PRINTF("============> NO PSAM CARD");
	   return -11; //No Psam card
	}


    liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
    if (liret!=0)
    	return -1;

    liret = smartCardInFunc->resetCard(aiddev,strCardReset);
    if (liret!=0)
    	return -2;
// #ifdef Debug
// 	ShowHexString(2,strCardReset,strCardReset[ 0 ] + 1);
// 	getkey();
// #endif

	//MOD => Removed 1
	asc2hex(strCardReset+5,8,StrCardSerNo,16); //Get Card seial no //StrRetDate[0] is length so is 5+1
 	StrCardSerNo[16]='\0';
	memcpy(userCardInfo->CM_Card_SerNo,StrCardSerNo,32);

	//LOG_PRINTF(("Serial Number => '%s'", userCardInfo->CM_Card_SerNo));
	//get_char();

	memcpy(outData,userCardInfo->CM_Card_SerNo,32);
//////////////////////////////////////////////////////////////////////////
	//03-22
	//To read out the psam card .No
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_BOTTOM);
	if (liret!=0)
	{
		return -1;
	}
	
	liret = smartCardInFunc->resetCard(aiddev,StrRetData);
	if (liret!=0)
	{
		return -2;
	}
	
	//Note removed +1 here for MoreFun
	//asc2hex(StrRetData+5+1,8,userCardInfo->CD_UserName,16);
	asc2hex(StrRetData+5,8,userCardInfo->CD_UserName,16);
	userCardInfo->CD_UserName[16] = '\0';
	memcpy(outData+131,userCardInfo->CD_UserName, 32);

	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
	if (liret!=0)
	{
		return -1;
	}
	
//////////////////////////////////////////////////////////////////////////

	liret = IsSysCard(aiddev, smartCardInFunc);

	if (liret!=0)
		return -3;

	liret = InternalAuth(aiddev, smartCardInFunc);

	if (liret!=0)
		return -4;

	/**************************************************/
	/*             Read User Card File                */
	/**************************************************/
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
	if (liret!=0)
		return -35;

	liret = SelectFile(aiddev,0x3f01, smartCardInFunc);
	if (liret!=0)
		return -6;

	//Read File info
	liret = ReadCard(aiddev,1,12,46,1,StrRetData, smartCardInFunc);
	if (liret!=0)
		return -7;

	asc2hex(StrRetData,46,StrTemp,46*2);
	//03-22 
	memcpy(userCardInfo->CM_UserNo,StrRetData,17); //UserNo
	userCardInfo->CM_UserNo[17]='\0';
	if (userCardInfo->CM_UserNo[0]==0xFF)//Not initialize user card
    {
		return -22;
	}    

	memcpy(outData+47,userCardInfo->CM_UserNo,18);
   

	memset(userCardInfo->CM_Purchase_DateTime,' ',15);
	memcpy(userCardInfo->CM_Purchase_DateTime,StrTemp+48,14);// the datetime of last purchase
  

	memcpy(outData+32,userCardInfo->CM_Purchase_DateTime,15);

	memcpy(StrTemp,StrRetData+17,1);
	StrTemp[1] = '\0';
	userCardInfo->CM_Card_Status =StrTemp[0];  //card status
	if ((StrTemp[0]&0x20)!=0)
		return -12;  //wrong status
	asc2hex(StrTemp,1,StrTemp2,2);
	memcpy(outData+65,StrTemp2,2);   

	Bcd2DoubleEXT(StrRetData+18,1,2,0,&(dtemp));//the times of purchase
	userCardInfo->CM_Purchase_Times=(unsigned short)dtemp;
	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-4d",userCardInfo->CM_Purchase_Times);  
	memcpy(outData+67,StrTemp,4);

	Bcd2DoubleEXT(StrRetData+20,1,4,2,&(userCardInfo->CM_Purchase_Power)); //last power purchased 

	if (userCardInfo->CM_Purchase_Power==0)  //Zero Power
	{
		return -21;
	}  

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",userCardInfo->CM_Purchase_Power);
	memcpy(outData+71,StrTemp,9);

	Bcd2DoubleEXT(StrRetData+31,1,4,2,&(userCardInfo->CM_Alarm_L1)); //Alarm_L1

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",userCardInfo->CM_Alarm_L1);
	memcpy(outData+80,StrTemp,9);

	Bcd2DoubleEXT(StrRetData+35,1,4,2,&(userCardInfo->CM_Alarm_L2));//Alarm_L2

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",userCardInfo->CM_Alarm_L2);
	memcpy(outData+89,StrTemp,9);

	Bcd2DoubleEXT(StrRetData+39,1,4,2,&(userCardInfo->CM_Alarm_L3));//Alarm_L3

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",userCardInfo->CM_Alarm_L3);
	memcpy(outData+98,StrTemp,9);

	result = 0;
	Bcd2DoubleEXT(StrRetData+43,1,4,2,&result);
	userCardInfo->CM_Load_Thrashold = result;


	result = 0;
	Bcd2DoubleEXT(StrRetData+45,1,4,2,&result);
	userCardInfo->CM_User_Demand = result;

	result = 0;
	Bcd2DoubleEXT(StrRetData+49,1,4,2,&result);
	userCardInfo->CM_Arrearage_Months = result;

	result = 0;
	Bcd2DoubleEXT(StrRetData+50,1,4,2,&result);
	userCardInfo->CM_Arrearage_Amount = result;

	//fill the file blank
	for(i=0;i<107;i++)
	{
		if (*(outData+i)=='\0')
			*(outData+i)=' ';   
	}
	outData[107]='\0';

	liret = ReadCard(aiddev,2,0,85,1,StrRetData, smartCardInFunc);
	if (liret!=0)
		return -7; //03-22

	//get Meter Status
	userCardInfo->CF_MeterStatus =   StrRetData[28];

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-2X",userCardInfo->CF_MeterStatus);
	memcpy(outData+107,StrTemp,2);


	//by jarod 2012.04.12
	if (StrRetData[28]==0xff && smartCardInFunc->isDebug == 0)
	{
		return -9;
	}
   
   
	Bcd2DoubleEXT(StrRetData+29,1,2,0,&(dtemp));//Load times
	userCardInfo->CF_Load_Times=(unsigned short)dtemp;

	Bcd2DoubleEXT(StrRetData+31,1,4,2,&(userCardInfo->CF_Remain_Power));
	Bcd2DoubleEXT(StrRetData+35,1,4,2,&(userCardInfo->CF_TotalUsed_Power));

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-4d",userCardInfo->CF_Load_Times);  
	memcpy(outData+109,StrTemp,4);//Load times

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",userCardInfo->CF_Remain_Power); 
	memcpy(outData+113,StrTemp,9);

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-9.2f",userCardInfo->CF_TotalUsed_Power); 
	memcpy(outData+122,StrTemp,9);

	smartCardInFunc->closeSlot(aiddev);
	//PrintRecord(userCardInfo); //Comment Printing we can do this later.
	
	return 0;
}


 void dLog(char * functionName, char * label, char * value, int expectedBytes)
  {
#ifdef DEV_DEBUG
      //LOG_PRINTF("%s:%s => %s, len: %d, expected: %d", functionName, label, value, strlen(value), expectedBytes);
#endif
  }


 void logStringBcd(char * label, char * value, int len)
{

	char asc_buf[512] = {'\0'};

	PubBcd2Asc0(value, len, asc_buf);

	//LOG_HEX_PRINTF ("HEX", value, len);

#ifdef DEV_DEBUG
	//LOG_PRINTF("%s => '%s', len: %d", label, asc_buf, strlen(asc_buf));
	//get_char();
#endif
}


int WriteUserCard(unsigned char *inData,unsigned char *outData, const SmartCardInFunc * smartCardInFunc)
{
	int liret,aiddev;
	//unsigned long llret;
	//unsigned char rlen;

	double lasql,ladoutemp;
	/*struct */La_Card_info LUserCi;
	unsigned char *p;

	//unsigned char initbuf[30];
	unsigned char StrRetData[256],StrTemp[256],portname[5],StrCardSerNo[40];


	//LOG_PRINTF(("========================> 1"))

	memcpy(portname,inData,5);
		
	liret=smartCardInFunc->detectPsamCB();
	if (liret!=0)
	{
	   return -11; //no psam card
	}

	//LOG_PRINTF(("=======================> 1.1"))
//////////////////////////////////////////////////////////////////////////
	
    memset(StrTemp,0,sizeof(StrTemp));
	memcpy(StrTemp,inData+5,32);
    p=strchr(StrTemp,' ');
	if(p)
		StrTemp[p-StrTemp]='\0';
	memcpy(LUserCi.CM_Card_SerNo,StrTemp,17);//get 17 byte��last is 0

	//LOG_PRINTF(("=======================> 1.2"))

//////////////////////////////////////////////////////////////////////////

	memset(StrTemp,0,sizeof(StrTemp));
	memcpy(StrTemp,inData+37,18);
    p=strchr(StrTemp,' ');
	if(p)
		StrTemp[p-StrTemp]='\0';
	memcpy(LUserCi.CM_UserNo,StrTemp,18);//get 18 byte��last is 0

	//LOG_PRINTF(("=======================> 1.3"))

//////////////////////////////////////////////////////////////////////////

	memset(StrTemp,0,sizeof(StrTemp));
	memcpy(StrTemp,inData+55,9);

    p=strchr(StrTemp,' ');
	if (p)
		StrTemp[p-StrTemp]='\0';
	LUserCi.CM_Purchase_Power = strtod(StrTemp,NULL);

	//LOG_PRINTF(("========================> 1.4"));

//////////////////////////////////////////////////////////////////////////
	
	memset(StrTemp,0,sizeof(StrTemp));
	memcpy(StrTemp,inData+64,4);
    p=strchr(StrTemp,' ');
	if (p)
		StrTemp[p-StrTemp]='\0';
	ladoutemp = strtod(StrTemp,NULL);//Purchase_Times
	LUserCi.CM_Purchase_Times = ladoutemp;


	//LOG_PRINTF(("========================> 1.5"));

//////////////////////////////////////////////////////////////////////////

	memset(StrTemp,0,sizeof(StrTemp));
	memcpy(StrTemp,inData+68,9);
    p=strchr(StrTemp,' ');
	if (p)
		StrTemp[p-StrTemp]='\0';
	ladoutemp = strtod(StrTemp,NULL);
	LUserCi.CM_Alarm_L1 = ladoutemp;

	//LOG_PRINTF(("========================> 1.6"));

//////////////////////////////////////////////////////////////////////////

	memset(StrTemp,0,sizeof(StrTemp));
	memcpy(StrTemp,inData+77,9);
    p=strchr(StrTemp,' ');
	if (p)
		StrTemp[p-StrTemp]='\0';
	ladoutemp = strtod(StrTemp,NULL);
	LUserCi.CM_Alarm_L2 = ladoutemp;

	//LOG_PRINTF(("========================> 1.7"));

//////////////////////////////////////////////////////////////////////////

	memset(StrTemp,0,sizeof(StrTemp));
	memcpy(StrTemp,inData+86,9);
    p=strchr(StrTemp,' ');
	if (p)
		StrTemp[p-StrTemp]='\0';
	ladoutemp =strtod(StrTemp,NULL);
	LUserCi.CM_Alarm_L3 = ladoutemp;

	//LOG_PRINTF(("========================> 1.8"));

//////////////////////////////////////////////////////////////////////////
if (smartCardInFunc->isDebug == 0)
{
	//Compare the seialNo ,avoid change card after readrecord
	memset(StrCardSerNo,0,sizeof(StrCardSerNo));
	memset(StrRetData,0,sizeof(StrRetData));
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
 	if (liret!=0)
 		return -1;//03-22
 	liret = smartCardInFunc->resetCard(aiddev,StrRetData);
 	if (liret!=0)
 		return -2;//03-22
 	asc2hex(StrRetData+5+1,8,StrCardSerNo,16); //Get Card seial no //StrRetDate[0] is length so is 5+1
 	StrCardSerNo[16]='\0';
 	
	if (memcmp(StrCardSerNo,LUserCi.CM_Card_SerNo,16)!=0) {//Compare the seialNo ,avoid change card 
		printf("\r\nActual -> '%s', Expected -> '%s'\r\n", StrCardSerNo,LUserCi.CM_Card_SerNo);
 		return -13;
	}
}
//////////////////////        END       /////////////////////////////////	
	//Read PSAM Auth Power
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_BOTTOM);
	if (liret!=0)
		return -1;//03-22

	//LOG_PRINTF(("========================> 1.9"));

	memset(StrRetData,0,sizeof(StrRetData));
	liret = smartCardInFunc->resetCard(aiddev,StrRetData);
	if (liret!=0)
		return -2;//03-22

	//LOG_PRINTF(("========================> 2"));

	liret = SelectFile(aiddev,0x3f01, smartCardInFunc);
	if (liret!=0)
		return -6;//03-22


	//LOG_PRINTF(("========================> 2.1"));


	liret = ReadRecord(aiddev,3,1,4,0,StrRetData, smartCardInFunc);
	if (liret!=0)
		return -6;//03-22

	//LOG_PRINTF(("========================> 2.2"));

	memset(StrTemp,0,sizeof(StrTemp));
	asc2hex(StrRetData, 4, StrTemp, 8);
			
	lasql =  strtol(StrTemp,NULL,16);
	lasql = lasql/100;

	//LOG_PRINTF(("========================> 2.3"));
//////////////////////////////////////////////////////////////////////////
//03-29
	LUserCi.CP_Remain_Auth = lasql;

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-10.2f",LUserCi.CP_Remain_Auth);
	memcpy(outData,StrTemp,11);
	outData[11] = '\0';


	//LOG_PRINTF(("========================> 2.4"));

//////////////////////////////////////////////////////////////////////////

	if (lasql<LUserCi.CM_Purchase_Power)
		return -17; //Low power //03-22
//03-29
//	LUserCi.CP_Remain_Auth = lasql;

	//LOG_PRINTF(("========================> 2.5"));
	

	//LOG_PRINTF(("FIRST AUTH: aiddev = %d, serial: %s", aiddev,  LUserCi.CM_Card_SerNo));

	liret= PurchaseAuth(aiddev, LUserCi.CM_Card_SerNo, smartCardInFunc);
	if (liret!=0)
		return -8;

	//LOG_PRINTF(("========================> 2.6"));

	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
	if (liret!=0)
		return -1;//03-22

	//LOG_PRINTF(("========================> 2.7"));

	//Prepare the Card data
	memcpy(StrRetData,LUserCi.CM_UserNo,17);

//03-22	
	if (LUserCi.CM_Purchase_Times==1)  //Change card status 
		StrRetData[17] = 0x0059;
	else
		StrRetData[17] = 0x0056; //Card status

	Double2BCD(LUserCi.CM_Purchase_Times,StrTemp,2,0,0);
	memcpy(StrRetData+18,StrTemp,2);   //Purchase Times

	 logStringBcd("CM_Purchase_Times", StrTemp, 2);

	Double2BCD(LUserCi.CM_Purchase_Power,StrTemp,4,2,0);
	memcpy(StrRetData+20,StrTemp,4);   //Purchase power

	logStringBcd("CM_Purchase_Power", StrTemp, 4);

	//LOG_PRINTF(("========================> 2.8"));


	//DateTime
	smartCardInFunc->getDateTimeBcd(StrTemp);//get 7 len bcd time data YYYYMMDDHHmmSS
 	memcpy(StrRetData+24,StrTemp,7);

	//LOG_PRINTF(("========================> 2.8A"));

	//logStringBcd("TIME", StrTemp, 7);

/*
#ifdef DEV_DEBUG

	if (1) {
		char asc_buf[512] = {'\0'};

		PubBcd2Asc0(StrTemp, 7, asc_buf);

		//LOG_PRINTF("Asc_buf => '%s'", asc_buf);
		
		PubBcd2Asc(StrTemp, 7, asc_buf);

		//LOG_PRINTF("Asc_buf => '%s'", asc_buf);
	}
	//LOG_PRINTF("Time BCD => %s", StrTemp);
#endif
*/

	Double2BCD(LUserCi.CM_Alarm_L1,StrTemp,4,2,0);
	memcpy(StrRetData+31,StrTemp,4);

	logStringBcd("AL1", StrTemp, 4);

	//LOG_PRINTF(("========================> 2.8B"));

	Double2BCD(LUserCi.CM_Alarm_L2,StrTemp,4,2,0);
	memcpy(StrRetData+35,StrTemp,4);
	logStringBcd("AL2", StrTemp, 4);

	Double2BCD(LUserCi.CM_Alarm_L3,StrTemp,4,2,0);
	memcpy(StrRetData+39,StrTemp,4);
	logStringBcd("AL3", StrTemp, 4);

	//LOG_PRINTF(("========================> 2.8C"));

	Double2BCD(LUserCi.CM_Load_Thrashold,StrTemp,2,2,0);
	memcpy(StrRetData+43,StrTemp,2);
	logStringBcd("CM_Load_Thrashold", StrTemp, 2);

	StrRetData[45] = AddCheckSum(45,StrRetData);
	logStringBcd("CARD DATA => ", StrRetData, 46);

	//LOG_PRINTF(("========================> 2.8D"));

/*
#ifdef DEV_DEBUG

	if (1) {
		char asc_buf[512] = {'\0'};
		PubBcd2Asc0(StrRetData, 7, asc_buf);

		//LOG_PRINTF("ASC_BUF => '%s', len: %d", asc_buf, strlen(asc_buf));
		get_char();
	}
	//LOG_PRINTF("StrRetData => '%s'", StrRetData);
	get_char();
#endif
*/

	asc2hex(StrRetData,46,StrTemp,92);

	//LOG_PRINTF(("========================> 2.9"));

	liret = WriteCard(aiddev,1,0x0c,46,1,StrTemp);
	if (liret!=0)
		return -18;

	//LOG_PRINTF("========================> 3");
//////////////////////////////////////////////////////////////////////////

	//LOG_PRINTF("SECOND AUTH: aiddev = %d, serial: %s", aiddev,  LUserCi.CM_Card_SerNo);
	

	liret= ReturnAuth(aiddev, LUserCi.CM_Card_SerNo, smartCardInFunc);
	if (liret!=0)
		return -23;//03-22

	//LOG_PRINTF("========================> 3.1");
    memset(StrTemp,0xFF,sizeof(StrTemp));  //All 0xFF  Clean the Meter return area
    memcpy(StrRetData,StrTemp,51); 
    StrRetData[51] = AddCheckSum(51,StrRetData);

	//LOG_PRINTF("========================> 3.2");
    
    asc2hex(StrRetData,52,StrTemp,104);
	//by jarod 2012.04.12
	liret = WriteCard(aiddev,2,0x1c,52,1,StrTemp);
	if (liret!=0)
		return -18;     

//////////////////////////////////////////////////////////////////////////


	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_BOTTOM);
	if (liret!=0)
		return -1;//03-22

	//LOG_PRINTF("========================> 3.3");

	liret = decrease(aiddev,LUserCi.CM_Purchase_Power, smartCardInFunc);
	if (liret!=0)
		return -19;

	//put_env("*PBAL", outData, sizeof(outData)); //psam balance

	return 0;
}





/**
 * Function: updateUserCard
 * Usage: int ret = updateUserCard(outData, &LUserCi);
 * ---------------------------------------------------
 * @param userCardInfo Struct containing customer card's info, see La_Card_info.
 * @param outData String representation of the customer card's info.
 * @author Ajani Opeyemi Sunday.
 * @Data 08/11/2017
 */

int updateUserCard(unsigned char *psamBalance, La_Card_info * LUserCi, const SmartCardInFunc * smartCardInFunc)
{
	int liret,aiddev;
	//unsigned long llret;
	//unsigned char rlen;

	double lasql/*,ladoutemp*/;
	//unsigned char *p;

	//unsigned char initbuf[30];
	unsigned char StrRetData[256],StrTemp[256],StrCardSerNo[40];

	printf("1-----------Puchase Times -> %d\n", LUserCi->CM_Purchase_Times);


	liret=smartCardInFunc->detectPsamCB();
	if (liret!=0)
	{
	   return -11; //no psam card
	}


//////////////////////////////////////////////////////////////////////////
if (smartCardInFunc->isDebug == 0)
{
	//Compare the seialNo ,avoid change card after readrecord
	memset(StrCardSerNo,0,sizeof(StrCardSerNo));
	memset(StrRetData,0,sizeof(StrRetData));
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
 	if (liret!=0)
 		return -1;//03-22
 	liret = smartCardInFunc->resetCard(aiddev,StrRetData);
 	if (liret!=0)
 		return -2;//03-22
 	asc2hex(StrRetData+5,8,StrCardSerNo,16); //Get Card seial no //StrRetDate[0] is length so is 5+1
 	StrCardSerNo[16]='\0';
 	
	if (memcmp(StrCardSerNo,LUserCi->CM_Card_SerNo,16)!=0) {//Compare the seialNo ,avoid change card
		printf("\r\n**Actual -> '%s', Expected -> '%s'\r\n", StrCardSerNo,LUserCi->CM_Card_SerNo);
 		return -13;
	}
}
//////////////////////        END       /////////////////////////////////	
	//Read PSAM Auth Power
	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_BOTTOM);
	if (liret!=0)
		return -1;//03-22

	memset(StrRetData,0,sizeof(StrRetData));
	liret = smartCardInFunc->resetCard(aiddev,StrRetData);
	if (liret!=0)
		return -2;//03-22


	liret = SelectFile(aiddev,0x3f01, smartCardInFunc);
	if (liret!=0)
		return -6;//03-22


	liret = ReadRecord(aiddev,3,1,4,0,StrRetData, smartCardInFunc);
	if (liret!=0)
		return -6;//03-22

	memset(StrTemp,0,sizeof(StrTemp));
	asc2hex(StrRetData, 4, StrTemp, 8);
			
	lasql =  strtol(StrTemp,NULL,16);
	lasql = lasql/100;
//////////////////////////////////////////////////////////////////////////
//03-29
	LUserCi->CP_Remain_Auth = lasql;

	memset(StrTemp,' ',sizeof(StrTemp));
	sprintf(StrTemp, "%-10.2f",LUserCi->CP_Remain_Auth);
	memcpy(psamBalance,StrTemp,11);
	psamBalance[11] = '\0';

//////////////////////////////////////////////////////////////////////////

	if (lasql<LUserCi->CM_Purchase_Power)
		return -17; //Low power //03-22
//03-29
//	LUserCi->CP_Remain_Auth = lasql;

	
	liret= PurchaseAuth(aiddev,LUserCi->CM_Card_SerNo, smartCardInFunc);
	if (liret!=0)
		return -8;

	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_TOP);
	if (liret!=0)
		return -1;//03-22

	//Prepare the Card data
	memcpy(StrRetData,LUserCi->CM_UserNo,17);


//03-22	
	if (LUserCi->CM_Purchase_Times==1)  //Change card status 
		StrRetData[17] = 0x0059;
	else
		StrRetData[17] = 0x0056; //Card status

	printf("2-----------Puchase Times -> %d\n", LUserCi->CM_Purchase_Times);


	Double2BCD(LUserCi->CM_Purchase_Times,StrTemp,2,0,0);
	memcpy(StrRetData+18,StrTemp,2);   //Purchase Times

	Double2BCD(LUserCi->CM_Purchase_Power,StrTemp,4,2,0);
	memcpy(StrRetData+20,StrTemp,4);   //Purchase power


	//DateTime
	smartCardInFunc->getDateTimeBcd(StrTemp);
 	memcpy(StrRetData+24,StrTemp,7);

	Double2BCD(LUserCi->CM_Alarm_L1,StrTemp,4,2,0);
	memcpy(StrRetData+31,StrTemp,4);

	Double2BCD(LUserCi->CM_Alarm_L2,StrTemp,4,2,0);
	memcpy(StrRetData+35,StrTemp,4);

	Double2BCD(LUserCi->CM_Alarm_L3,StrTemp,4,2,0);
	memcpy(StrRetData+39,StrTemp,4);

	Double2BCD(LUserCi->CM_Load_Thrashold,StrTemp,2,2,0);
	memcpy(StrRetData+43,StrTemp,2);

	StrRetData[45] = AddCheckSum(45,StrRetData);

	asc2hex(StrRetData,46,StrTemp,92);

	liret = WriteCard(aiddev,1,0x0c,46,1,StrTemp);
	if (liret!=0)
		return -18;
//////////////////////////////////////////////////////////////////////////

	//��д��֤
	liret= ReturnAuth(aiddev,LUserCi->CM_Card_SerNo, smartCardInFunc);
	if (liret!=0)
		return -23;//03-22
    memset(StrTemp,0xFF,sizeof(StrTemp));  //All 0xFF  Clean the Meter return area
    memcpy(StrRetData,StrTemp,51); 
    StrRetData[51] = AddCheckSum(51,StrRetData);
    
    asc2hex(StrRetData,52,StrTemp,104);
	//by jarod 2012.04.12
	liret = WriteCard(aiddev,2,0x1c,52,1,StrTemp);
	if (liret!=0)
		return -18;     

//////////////////////////////////////////////////////////////////////////


	liret = smartCardInFunc->cardPostion2Slot(&aiddev,CP_BOTTOM);
	if (liret!=0)
		return -1;//03-22

	liret = decrease(aiddev,LUserCi->CM_Purchase_Power, smartCardInFunc);
	if (liret!=0)
		return -19;

	//put_env("*PBAL", psamBalance, sizeof(psamBalance));

	return 0;
}

//Mod => Verifone implementation.


#if 0 //Variability 
int PrintRecord(La_Card_info * cardinfo)
{
	unsigned char ret;
	unsigned char strTemp[512];
	unsigned char chrTemp;
	double dTemp;

	ret = PrnInit();
	PrnFontSet(1, 2);
	PrnStep(30);
	PrnStr((char *)"\n--------------------------------\n");
	PrnStr("-----------Card  Info-----------\n");
	PrnStr((char *)"--------------------------------\n");
	
	memset(strTemp,0,sizeof(strTemp));
	memcpy(strTemp,cardinfo->CM_Card_SerNo,16);
	PrnStr("Card Ser No:");
	PrnStr("%s",strTemp);
	PrnStr("\n");

	memset(strTemp,0,sizeof(strTemp));
	//03-22
	memcpy(strTemp,cardinfo->CM_UserNo,17);
	PrnStr("User No:");
	PrnStr("%s",strTemp);
	PrnStr("\n");

	chrTemp = cardinfo->CM_Card_Status;
	PrnStr("Card Status:");
	switch(chrTemp)
	{
	case 1:
			PrnStr("Creat Account");
			break;
	case 2:
			PrnStr("Purchase");
			break;
	case 3:
			PrnStr("Load Power");
			break;
	default:
			PrnStr("%d",chrTemp);
			break;
	}
	PrnStr("\n");

	chrTemp = cardinfo->CM_Purchase_Times;
	PrnStr("Purchase Times:");
	PrnStr("%d",chrTemp);
	PrnStr("\n");


	dTemp = cardinfo->CM_Purchase_Power;
	PrnStr("Purchase Power:");
	PrnStr("%0.9f",dTemp);
	PrnStr("\n");

	dTemp = cardinfo->CP_Remain_Auth;
	PrnStr("Remain Power:");
	PrnStr("%0.9f",dTemp);
	PrnStr("\n");


	dTemp = cardinfo->CM_Alarm_L1;
	PrnStr("Alarm_L1:");
	PrnStr("%0.9f",dTemp);
	PrnStr("\n");

	dTemp = cardinfo->CM_Alarm_L2;
	PrnStr("Alarm_L2:");
	PrnStr("%0.9f",dTemp);
	PrnStr("\n");

	dTemp = cardinfo->CM_Alarm_L3;
	PrnStr("Alarm_L3:");
	PrnStr("%0.9f",dTemp);
	PrnStr("\n");

	memset(strTemp,0,sizeof(strTemp));
	memcpy(strTemp,cardinfo->CM_Purchase_DateTime,14);
	PrnStr("Purchase DateTime:");
	PrnStr("%s",strTemp);
	PrnStr("\n");

	PrnStr((char *)"\n--------------------------------\n");
	PrnStr("--------------END---------------\n");
	PrnStr((char *)"--------------------------------\n\n\n\n\n");
	PrnStep(30);
	PrnStart();
	return 0;
}
#endif


char * unistarErrorToString(short errorCode)
{
	switch (errorCode)
	{
		case -1: return "switch slot error";
		case -2: return "card reset error";
		case -3: return "not a legal card of the system";
		case -4: return "inner authorization error (customer card and PASM card are not belong to same system)";
		case -5: return "PSAM  reset error";
		case -7: return "Read Card error";
		case -8: return "vending authorization error";
		case -9: return "credit has not loaded from card to meter yet";
		case -11: return "open port error / No Psam";
		case -12: return "Invalid customer card";
		case -13: return "The serial number of current card is different from the serial number in parameter.";
		case -17: return "insufficient authorized credit in PASM card";
		case -18: return "writing error";
		case -19: return "PSAM Decrease Error!";
		case -21: return "customer card which has not been used by customer";
		case -22: return "customer card which has not Personalize to customer";
		case -23: return "authorization error";

		default: 
			//LOG_PRINTF(("Unknown error======> %d", errorCode));
			return "Unknow customer or PSAM error";

	}
}

#if 0 //Variability
void logCardInfo(La_Card_info * userCardInfo)
{
	LOG_PRINTF(("CM_Card_SerNo: '%s', len: %d", userCardInfo->CM_Card_SerNo, strlen(userCardInfo->CM_Card_SerNo)));
	LOG_PRINTF(("CD_UserName: '%s', len: %d", userCardInfo->CD_UserName, strlen(userCardInfo->CD_UserName)));
	LOG_PRINTF(("CM_UserNo: '%s', len: %d", userCardInfo->CM_UserNo, strlen(userCardInfo->CM_UserNo)));
	LOG_PRINTF(("CM_Purchase_DateTime: '%s', len: %d", userCardInfo->CM_Purchase_DateTime, strlen(userCardInfo->CM_Purchase_DateTime)));
	LOG_PRINTF(("CM_Card_Status: '%c'", userCardInfo->CM_Card_Status));
	LOG_PRINTF(("CM_Purchase_Times: %d", userCardInfo->CM_Purchase_Times));
	LOG_PRINTF(("CM_Purchase_Power: %g", userCardInfo->CM_Purchase_Power));
	LOG_PRINTF(("CM_Alarm_L1: %g", userCardInfo->CM_Alarm_L1));
	LOG_PRINTF(("CM_Alarm_L2: %g", userCardInfo->CM_Alarm_L2));
	LOG_PRINTF(("CM_Alarm_L3: %g", userCardInfo->CM_Alarm_L3));
	LOG_PRINTF(("CM_Load_Thrashold: %f", userCardInfo->CM_Load_Thrashold));
	LOG_PRINTF(("CM_User_Demand: %g", userCardInfo->CM_User_Demand));
	LOG_PRINTF(("CM_Arrearage_Months: %d", userCardInfo->CM_Arrearage_Months));
	LOG_PRINTF(("CM_Arrearage_Amount: %g", userCardInfo->CM_Arrearage_Amount));
	LOG_PRINTF(("CF_MeterStatus: '%c'", userCardInfo->CF_MeterStatus));
	LOG_PRINTF(("CF_Remain_Power: %g", userCardInfo->CF_Remain_Power));
	LOG_PRINTF(("CF_TotalUsed_Power: %g", userCardInfo->CF_TotalUsed_Power));
	LOG_PRINTF(("CF_Load_Times: %d", userCardInfo->CF_Load_Times));
}

#endif




#if 0
short getUpdateCardCmd(char cardUpdateCmd[195],
                       const La_Card_info *userCardInfo,
                       const char *energyValue) {
  char cardSlotNumber[3] = "  ";
  char slotNumber[3] = "  ";
  char space[2] = " ";
  char cardSerialNumber[33] = {'\0'};
  char userNumber[19] = {'\0'};
  char vendingCredits[10] = {'\0'};
  char vendingTimes[5] = {'\0'};
  char alarm1[10] = {'\0'};
  char alarm2[10] = {'\0'};
  char alarm3[10] = {'\0'};
  char backup[101] = {'\0'};

  short pos = 0;
  char buffer[200] = {'\0'};


  //LOG_PRINTF(("%s ====> 1", __FUNCTION__));
  // 0-1:Customer Card Slot number;eg.:�00�or�01�
  pos += sprintf(&cardUpdateCmd[pos], "%s", cardSlotNumber);
  // 2-3:PSAM Card Slot number;eg.:�02�or�03�or�04�
  pos += sprintf(&cardUpdateCmd[pos], "%s", slotNumber);
  // 4:Space;
  pos += sprintf(&cardUpdateCmd[pos], "%s", space);

  //LOG_PRINTF(("%s ====> Serial Number: %s", __FUNCTION__, userCardInfo->CM_Card_SerNo));

  // 5..36:Card serial number
  if (getNextCardField(cardSerialNumber, sizeof(cardSerialNumber) - 1,
                       userCardInfo->CM_Card_SerNo, "CARD SN"))
    return -1;
  pos += sprintf(&cardUpdateCmd[pos], "%s", cardSerialNumber);

  //LOG_PRINTF(("%s ====> 3", __FUNCTION__));

  // 37..54: User number
  if (getNextCardField(userNumber, sizeof(userNumber) - 1,
                       userCardInfo->CM_UserNo, "USER NO"))
    return -2;
  pos += sprintf(&cardUpdateCmd[pos], "%s", userNumber);


  //LOG_PRINTF(("%s ====> 4", __FUNCTION__));

  // 55..63: vending credit of this time
  if (getNextCardField(vendingCredits, sizeof(vendingCredits) - 1, energyValue,
                       "CREDIT"))
    return -3;
  pos += sprintf(&cardUpdateCmd[pos], "%s", vendingCredits);

  //LOG_PRINTF(("%s ====> 5", __FUNCTION__));

  // 64..67 :vending times
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%d", userCardInfo->CM_Purchase_Times + 1);
  if (getNextCardField(vendingTimes, sizeof(vendingTimes) - 1, buffer, "TIMES"))
    return -4;
  pos += sprintf(&cardUpdateCmd[pos], "%s", vendingTimes);

  // 68..76: 1 Low credit alarm 1
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%g", userCardInfo->CM_Alarm_L1);
  if (getNextCardField(alarm1, sizeof(alarm1) - 1, buffer, "ALARM L1"))
    return -5;
  pos += sprintf(&cardUpdateCmd[pos], "%s", alarm1);


  //LOG_PRINTF(("%s ====> 6", __FUNCTION__));

  // 77..85: Low credit alarm 2
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%g", userCardInfo->CM_Alarm_L2);
  if (getNextCardField(alarm2, sizeof(alarm2) - 1, buffer, "ALARM L2"))
    return -6;
  pos += sprintf(&cardUpdateCmd[pos], "%s", alarm2);

  // 86..94: Low credit alarm 3
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%g", userCardInfo->CM_Alarm_L3);
  if (getNextCardField(alarm3, sizeof(alarm3) - 1, buffer, "ALARM L3"))
    return -7;
  pos += sprintf(&cardUpdateCmd[pos], "%s", alarm3);

  //LOG_PRINTF(("%s ====> 7", __FUNCTION__));

  // 95..194:backup
  memset(buffer, '\0', sizeof(buffer));
  strcat(buffer, " ");

  if (getNextCardField(backup, sizeof(backup) - 1, buffer, "BACK UP"))
    return -8;
  pos += sprintf(&cardUpdateCmd[pos], "%s", backup);

  //LOG_PRINTF(("%s ====> 8", __FUNCTION__));

  if (pos != 195) {
	error("CMD ERROR", "\nExpected %d, got %d", 195, pos);
    return -9;
  }

  //LOG_PRINTF(("%s ====> 9", __FUNCTION__));

  return 0;
}

#endif
//EndofFile
