/*
 * basefun.c
 *
 *  Created on: 2011-12-1
 *      Author: xzj
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "basefun.h"

#include "basefun.h"

#define MAX_R_W_BINFILE_LEN 0xb2

APDU_SEND apduSend;
APDU_RESP apduResp;

//MOD => see old one below

void asc2hex(char *ascStr, unsigned int ascLen, unsigned char *hexStr, unsigned int hexLen);


/*
//Mod => Changed to static, because it is also in emvui
static void hex2asc(unsigned char *hexStr, unsigned int hexLen, char *ascStr, unsigned int ascLen)
{
	int i;
	unsigned char uc;
	for (i = 0; i < hexLen; i++)
	{
		if (i > ascLen * 2)
			break;
		uc = hexStr[i];
		if (uc >= 0x30 && uc <= 0x39)
			uc -= 0x30;
		else if (uc >= 0x41 && uc <= 0x46)
			uc -= 0x37;
		else if (uc >= 0x61 && uc <= 0x66)
			uc -= 0x57;
		else
			break;
		if (i % 2 == 1)
			ascStr[(i - 1) / 2] = ascStr[(i - 1) / 2] | uc;
		else
			ascStr[i / 2] = uc << 4;
	}
	return;
}
*/


void ShowHexString(int iRow, unsigned char *pBuf, int iLen)
{
	int i = 0;

	for (i = 0; i < iLen; i++)
	{
		//MOD => Not used
		//ScrPrint( ( i % 8 ) * 15, iRow + i / 8, 0, "%02X ", *( pBuf + i ) );
	}

	return;
}

//IccCommand return code switch
void Ret_l_to_hex(long lnum, long lhex)
{
}
void l_to_asc(long lnum, unsigned char *asc_buf)
{
	long l;
	unsigned char i, j, c, buf[20];

	l = lnum;
	i = 0;
	do
	{
		c = '0' + (unsigned char)(l % 10);
		buf[i] = c;
		i++;
		l = l / 10;
	} while (l != 0);
	buf[i] = 0;

	for (j = 0; j < i; j++)
	{
		asc_buf[j] = buf[i - 1 - j];
	}
	asc_buf[i] = 0;
}

unsigned char abcd_to_asc(unsigned char abyte)
{
	if (abyte <= 9)
		abyte = abyte + '0';
	else
		abyte = abyte + 'A' - 10;
	return (abyte);
}

void BCDToASC(unsigned char *asc_buf, unsigned char *bcd_buf, int n)
{
	int i, j;

	j = 0;
	for (i = 0; i < n / 2; i++)
	{
		asc_buf[j] = (bcd_buf[i] & 0xf0) >> 4;
		asc_buf[j] = abcd_to_asc(asc_buf[j]);
		j++;
		asc_buf[j] = bcd_buf[i] & 0x0f;
		asc_buf[j] = abcd_to_asc(asc_buf[j]);
		j++;
	}
	if (n % 2)
	{
		asc_buf[j] = (bcd_buf[i] & 0xf0) >> 4;
		asc_buf[j] = abcd_to_asc(asc_buf[j]);
	}
}

/*=======================================================================
 asc_to_bcd() - Translate ASCII string into BCD string
 <n> : number of ascii character
=======================================================================*/
unsigned char aasc_to_bcd(unsigned char asc)
{
	unsigned char bcd;

	if ((asc >= '0') && (asc <= '9'))
		bcd = asc - '0';
	else if ((asc >= 'A') && (asc <= 'F'))
		bcd = asc - 'A' + 10;
	else if ((asc >= 'a') && (asc <= 'f'))
		bcd = asc - 'a' + 10;
	else if ((asc > 0x39) && (asc <= 0x3f))
		bcd = asc - '0';
	else
	{
		bcd = 0x0f;
	}
	return bcd;
}

void ASCToBCD(unsigned char *bcd_buf, unsigned char *asc_buf, int n)
{
	int i, j;

	j = 0;

	for (i = 0; i < (n + 1) / 2; i++)
	{
		bcd_buf[i] = aasc_to_bcd(asc_buf[j++]);
		bcd_buf[i] = ((j >= n) ? 0x00 : aasc_to_bcd(asc_buf[j++])) + (bcd_buf[i] << 4);
	}
}

/************************************************************************/
/*                       Function:�ַ���תdouble                        */
/************************************************************************/

double str2d(const char *str)
{
	int temp = 0;
	double xs = 0, xsw = 0;
	const char *ptr = str; //ptr save str's start
	if (*str == '-' || *str == '+')
	{
		str++;
	}

	while (*str != 0)
	{
		if ((*str == '.') && (xsw == 0))
		{
			xsw = 1;
		}
		else
		{
			if ((*str == '.') && (xsw != 0))
			{
				break;
			}
		}

		if (((*str < '0') || (*str > '9')) && (*str != '.'))
		{
			break;
		}
		if ((xsw != 0) && (*str != '.'))
		{
			xsw = xsw * 0.1;
			xs = xs + (*str - '0') * xsw;
		}
		else
		{
			if (*str != '.')
				temp = temp * 10 + (*str - '0');
		}
		str++; //move to next
	}
	if (*ptr == '-') //if the str start as "-"��then change to opposite char
	{
		temp = -temp;
	}
	return (temp + xs);
}

/*
//���ܣ�doubleתBCD��
//
//���룺int Dec                      ��ת����Double����
//         int length                   BCD�������ֽڳ���
//         int iDig               ��ת�������ݵ�С��λ��
//         int iSig         0:��λ�ֽ���ǰ,1:��λ�ֽ���ǰ
//
//�����unsigned char *Bcd           ת�����BCD�� char
//
//���أ�0  success
*/
/************************************************************************/
/*	Function:Double To BCD                                              */
/*  Input Para:  int Dec         the Double data                        */
/*               int length      Bcd data length                        */
/*               int iDig        the position of double's decimal       */
/*               int iSig        0:Low byte first,1:High byte first     */
/*  Output Para: unsigned char *Bcd                                     */
/*  Return: 0  succes                                                   */
/************************************************************************/

int Double2BCD(double Dec, unsigned char *Bcd, int length, int iDig, int iSig)
{
	int i;
	long int temp, temp1;
	double dec1;
	unsigned char BcdTemp[128] = {0};
	dec1 = Dec;
	temp1 = Dec;

	for (i = 0; i < iDig; i++)
		dec1 = dec1 * 10;
	temp1 = dec1;

	for (i = length - 1; i >= 0; i--) //High at first
	{
		temp = temp1 % 100;

		Bcd[i] = ((temp / 10) << 4) + ((temp % 10) & 0x0F);
		temp1 /= 100;
	}

	if (iSig == 0) //Low at last
	{
		memcpy(BcdTemp, Bcd, length);
		for (i = 0; i < length; i++)
			Bcd[i] = BcdTemp[length - 1 - i];
	}

	return 0;
}

//���ܣ�ʮ����תʮ������
//  //���룺int dec     ��ת����ʮ��������
//      int length    ת�����ʮ���������ݳ���  //
//�����unsigned char *hex          ת�����ʮ����������  //
//���أ�0    success  //
//˼·��ԭ��ͬʮ������תʮ����  //////////////////////////////////////////////////////////
/************************************************************************/
/*	Function:decimal to hexadecimal                                     */
/*  Input Para:  int Dec         the Double data                        */
/*               int length      Bcd data length                        */
/*  Output Para: unsigned char *hex                                     */
/*  Return: 0  succes                                                   */
/************************************************************************/

unsigned int DectoHex(unsigned long dec, unsigned char *hex, int length)
{
	int i;
	for (i = length - 1; i >= 0; i--)
	{
		hex[i] = (dec % 256) & 0xFF;
		dec /= 256;
	}
	return 0;
}

/*
 * ����:BCDת��Ϊdouble BCD���ֽ���ǰ
 *buf BCD ��:buf[0]=20,buf[1]=11
 *lidiglen:С����λ��.
 *lbreverse:�����BCD�Ƿ��ǵ���.0����,1��.
 */
/************************************************************************/
/*	Function:BCD to Double,BCD's lower byte is first                    */
/*  Input Para:  buf BCD     e.g. :buf[0]=20,buf[1]=11                  */
/*               lidiglen      the position of the decimal point        */
/*               lbreverse      0:inverted order 1:not                  */
/************************************************************************/

double Bcd2Double(unsigned char *buf, unsigned char lbreverse, unsigned char length, unsigned char lidiglen)
{
	//unsigned long xdata;
	double tem;
	unsigned char strtemp[20] = {0};
	register unsigned char i;

	asc2hex(buf, length, strtemp, length * 2);
	printf("Bcd2Hex��������:%s\n", strtemp);
	tem = 0;
	if (lbreverse != 0)
		for (i = 0; i < length; i++)
		{
			strtemp[i] = buf[length - i - 1];
			printf("strtemp: %x\n", strtemp[i]);
		}
	else
	{
		memcpy(strtemp, buf, length);
	}

	for (i = 0; i < length; i++)
	{
		tem = tem * 100 + (strtemp[i] / 16) * 10 + strtemp[i] % 16;
		printf("tem:%f\n", tem);
	}
	if (lidiglen == 0)
		return tem;
	else
	{
		for (i = 0; i < lidiglen; i++)
			tem = tem / 10;
		return tem;
	}
}

signed int Bcd2DoubleEXT(unsigned char *buf, unsigned char lbreverse, unsigned char length, unsigned char lidiglen, double *out)
{
	//unsigned long xdata;
	double tem;
	unsigned char strtemp[20] = {0};
	register unsigned char i;

	asc2hex(buf, length, strtemp, length * 2);

	tem = 0;
	if (lbreverse != 0)
		for (i = 0; i < length; i++)
		{
			strtemp[i] = buf[length - i - 1];
		}
	else
	{
		memcpy(strtemp, buf, length);
	}

	for (i = 0; i < length; i++)
	{
		tem = tem * 100 + (strtemp[i] / 16) * 10 + strtemp[i] % 16;
	}
	if (lidiglen == 0)
	{
		*out = tem;
	}
	else
	{
		for (i = 0; i < lidiglen; i++)
			tem = tem / 10;
		*out = tem;
	}
	return 0;
}


#if 0
unsigned char *Hex2Bcd(double lasource, unsigned char lidig, unsigned char liwigth)
{
	register unsigned char i;
	unsigned char dest[8];
	unsigned long hexg;
	double lisource;
	for (i = 0; i < lidig; i++)
		lisource = lasource * 10;
	hexg = lisource;

	for (i = 0; i < liwigth; i++)
	{
		dest[i] = (hexg % 10) * 16; /* High harf byte */
		hexg = hexg / 10;
		dest[i + 1] = hexg % 10; /* Low harf byte */

		hexg = hexg / 10;
	}
	return dest;
}
#endif 






/*----------------------------------------------------------
*Function	:	asc2hex
e.g:
ascStr[0]=0x35,ascStr[1]=0x56;ת�����Ϊ hexStr[0]=0x33,hexStr[1]=0x35,hexStr[2]=0x35,hexStr[3]=0x36,
*-----------------------------------------------------------
*/

void asc2hex(char *ascStr, unsigned int ascLen, unsigned char *hexStr, unsigned int hexLen)
{
	int i;
	unsigned char uc;
	for (i = 0; i < ascLen; i++)
	{
		if (i * 2 >= hexLen)
			break;
		uc = (ascStr[i] & 0xF0) >> 4;
		if (uc < 0x0A)
			uc += 0x30;
		else
			uc += 0x37;
		hexStr[i * 2] = uc;
		uc = ascStr[i] & 0x0F;
		if (uc < 0x0A)
			uc += 0x30;
		else
			uc += 0x37;
		if (i * 2 + 1 >= hexLen)
			break;
		hexStr[i * 2 + 1] = uc;
	}
	return;
}


// Check the status word of ApduResp
unsigned char CheckSw(unsigned char sw1, unsigned char sw2)
{
	if ((apduResp.SWA == sw1) && (apduResp.SWB == sw2))
	{
		return 0;
	}
	else
	{
		return 1; // Execute failure
	}
}




signed int decrease(int aiddev, double adcharge, const SmartCardInFunc * smartCardInFunc)
{
	unsigned int liret;
	unsigned long ldcharge;
	unsigned int rlen;
	unsigned char StrTemp[12] = {0}, StrTemp1[12] = {0}, strCommand[128] = {0}, StrRetData[256] = {0};

	ldcharge = adcharge * 100;

	DectoHex(ldcharge, StrTemp, 4);

	asc2hex(StrTemp, 4, StrTemp1, 8);

	memset(strCommand, 0, sizeof(strCommand));
	memcpy(strCommand, "8030001C04", 10);

	strncat(strCommand, StrTemp1, 8);

	memset(StrRetData, 0, 256);
	liret = ItexSendCmd(aiddev, 18, strCommand, 1, &rlen, StrRetData, smartCardInFunc);

	if (liret != 0)
		return -1;

	smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);

	return 0;
}

/*-------------------------------------------------------------
*Function	:	SendCmd
*Output	    :   arlen	return data length
*Input	    :   
				aicdev :slot; 
				lilen  Command length;
				strCmd Command Data;
*Input	    :   iSig : no use
*/

signed int ItexSendCmd(int aicdev, unsigned int lilen, unsigned char *strCmd, unsigned char iSig, unsigned int *arlen, unsigned char *strRet, const SmartCardInFunc * smartCardInFunc)
{
	//int i, iTemp;
	//long lTemp;
	unsigned char st;
	unsigned char retlen;
	unsigned char sdata[300], strRetTemp[300];
	unsigned char databuf[300];
	//unsigned char strTemp[10];
	printf("Command -> %s\r\n", strCmd);

	memset(&apduSend, 0, sizeof(APDU_SEND));
	memset(&apduResp, 0, sizeof(APDU_RESP));

	memset(databuf, 0, sizeof(databuf));

	memset(sdata, 0, sizeof(sdata));
	memset(strRetTemp, 0, sizeof(strRetTemp));
	//memset(strRet, 0, sizeof(strRet));

	retlen = lilen / 2;

	ASCToBCD(databuf, strCmd, lilen);
	memcpy(apduSend.Command, databuf, 4);

	if (retlen > 5)
	{
		apduSend.Lc = databuf[4];
		memcpy(apduSend.DataIn, databuf + 5, apduSend.Lc);
		if (retlen > (apduSend.Lc + 5))
		{
			apduSend.Le = databuf[retlen - 1];
		}
		else
		{
			apduSend.Le = 256;
		}
	}
	else
	{
		apduSend.Le = databuf[4];
	}

	memset(&apduResp, 0, sizeof(APDU_RESP));
	st = smartCardInFunc->exchangeApdu(aicdev, &apduSend, &apduResp);

	if (!st && apduResp.SWA == 0x61) //if necessary to get sth back
	{

		sdata[0] = 0x00;
		sdata[1] = 0xC0;
		sdata[2] = 0x00;
		sdata[3] = 0x00;
		sdata[4] = st & 0x00ff;

		//LOG_PRINTF(("ST ==> '%c : %d'", st, st));

		//LOG_PRINTF(("sdata b4 asc2hex => '%s'", sdata));
		//asc2hex(sdata,5,sdata,10);
		//LOG_PRINTF("sdata after asc2hex => '%s'", sdata);

		memset(&apduSend, 0, sizeof(APDU_SEND));
		memset(&apduResp, 0, sizeof(APDU_RESP));

		memcpy(apduSend.Command, sdata, 4);

		//LOG_PRINTF(("apduSend.Command => '%s'", apduSend.Command));
		//MOD => Commented 2 lines
		//apduSend.Lc = 10 - 5;
		//memcpy(apduSend.DataIn, sdata+5, apduSend.Lc);
		//LOG_PRINTF(("apduSend.DataIn => '%s'", apduSend.DataIn));
		apduSend.Le = 256;

		st = smartCardInFunc->exchangeApdu(aicdev, &apduSend, &apduResp);

		//LOG_PRINTF(("%s:::::AFTER ICCISOCMD", __FUNCTION__));
		//t_char();

		if (st || CheckSw(0x90, 0x00))
		{
			return (apduResp.SWA * 256 + apduResp.SWB);
		}
	}

	if (st || CheckSw(0x90, 0x00))
	{
		return (apduResp.SWA * 256 + apduResp.SWB);
	}

	//////////////////////////////////////////////////////////////////////////

	/*
	if (apduResp.SWA==0x61) //if necessary to get sth back
	{

		sdata[0] = 0x00;
		sdata[1] = 0xC0;
		sdata[2] = 0x00;
		sdata[3] = 0x00;
		sdata[4] = st&0x00ff;

		//LOG_PRINTF(("ST ==> '%c : %d'", st, st));

		//LOG_PRINTF(("sdata b4 asc2hex => '%s'", sdata));
		asc2hex(sdata,5,sdata,10);
		//LOG_PRINTF("sdata after asc2hex => '%s'", sdata);

		memset(&apduSend, 0, sizeof(APDU_SEND));
		memset(&apduResp, 0, sizeof(APDU_RESP));

		memcpy(apduSend.Command, sdata, 4);


		//LOG_PRINTF(("apduSend.Command => '%s'", apduSend.Command));
		apduSend.Lc = 10 - 5;
		memcpy(apduSend.DataIn, sdata+5, apduSend.Lc);
		//LOG_PRINTF(("apduSend.DataIn => '%s'", apduSend.DataIn));
		apduSend.Le = 256;

		st = smartCardInFunc->exchangeApdu(aicdev,&apduSend,&apduResp);


		if (st || CheckSw(0x90, 0x00))
		{
			return (apduResp.SWA*256+apduResp.SWB);
		}
	}
	*/

	retlen = apduResp.LenOut;

	memcpy(strRet, apduResp.DataOut, retlen);
	*(strRet + retlen) = '\0';
	*arlen = retlen;

	return 0;
}

signed int SelectFile(int aicdev, unsigned int iFileID, const SmartCardInFunc * smartCardInFunc)
{
	int i;
	unsigned int rlen;
	int liret;
	unsigned char strCommand[300];
	unsigned char strRet[256];
	unsigned char strCommTemp[2], strRetTemp[4];

	// strCommTemp[0] = 0xff;
	strCommTemp[0] = (iFileID >> 8) & 0x00FF;
	strCommTemp[1] = iFileID & 0x00FF;
	memset(strRetTemp, 0, 4);
	asc2hex(strCommTemp, 2, strRetTemp, 4);

	memset(strCommand, 0, sizeof(strCommand));
	memcpy(strCommand, "00A4000002", 10);
	for (i = 0; i < 4; i++)
	{
		strCommand[i + 10] = strRetTemp[i];
	}
	memset(strRet, 0, sizeof(strRet));

#ifdef DEV_DEBUG
	//LOG_PRINTF(("%s === CMD => '%s'", __FUNCTION__, strCommand));
	//get_char();
#endif

	liret = ItexSendCmd(aicdev, 14, strCommand, 0, &rlen, strRet, smartCardInFunc);

	printf("Select file ret -> %d\n", liret);

	if (liret || CheckSw(0x90, 0x00))
	{
		return liret;
	}
	return 0;
}
/********************************************************************************
Function��ReadCard base function
Input��
    iFileID
    iOffset
    iLen    :   length of data read out
    iSig    :   no use
Output��
    strData :   Date read out
Return��
      success�� 0
      fail��    not 0
********************************************************************************/
signed int ReadCard_Base(int aiddev, unsigned int iFileID, unsigned int iOffset, unsigned int iLen, unsigned char iSig, unsigned char *strData, const SmartCardInFunc * smartCardInFunc)
{
	int liret;
	unsigned int rlen;
	int i;

	unsigned char strCommand[128] = {0};
	unsigned char strCommTemp[4] = {0};
	unsigned char strRetTemp[4] = {0};

	if ((iOffset > 0x00ff) || (iFileID > 31))
	{
		liret = SelectFile(aiddev, iFileID, smartCardInFunc);
		if (liret != 0)
			return -1;

		strCommTemp[0] = (iOffset >> 8) & 0x00FF;
		strCommTemp[1] = iOffset & 0x00FF;
		memset(strRetTemp, 0, sizeof(strRetTemp));
		asc2hex(strCommTemp, 2, strRetTemp, 4);

		memset(strCommand, 0, sizeof(strCommand));
		memcpy(strCommand, "00B0", 4);
		for (i = 0; i < 4; i++)
		{
			strCommand[i + 4] = strRetTemp[i];
		}

		strCommTemp[0] = iLen & 0x00ff;
		memset(strRetTemp, 0, 4);
		asc2hex(strCommTemp, 1, strRetTemp, 2);
		strCommand[8] = strRetTemp[0];
		strCommand[9] = strRetTemp[1];
	}
	else
	{
		strCommTemp[0] = (iFileID + 128);
		strCommTemp[1] = iOffset;
		strCommTemp[2] = iLen;
		memset(strRetTemp, 0, 4);
		asc2hex(strCommTemp, 3, strRetTemp, 6);
		memset(strCommand, 0, sizeof(strCommand));
		memcpy(strCommand, "00B0", 4);
		for (i = 0; i < 6; i++)
		{
			strCommand[i + 4] = strRetTemp[i];
		}
	}
	memset(strData, 0, 256);
	liret = ItexSendCmd(aiddev, strlen(strCommand), strCommand, iSig, &rlen, strData, smartCardInFunc);
	if (liret != 0)
		return liret;

	return 0;
}

/********************************************************************************
 Function��ReadCard
 Input��
     iFileID
     iOffset
     iLen
     iSig : 0: return Hex  strData. 
	        1: return char strData.
 Output��
     strData :   Date read out
 Return��
       0 or not
       
 ********************************************************************************/
signed int ReadCard(int aiddev, unsigned int iFileID, unsigned int iOffset, unsigned int iLen, unsigned char iSig, unsigned char *strData, const SmartCardInFunc * smartCardInFunc)
{
	signed int liret;
	unsigned int iReadLen;
	//unsigned char strReadData[256] = {0};

	liret = 0;
	memset(strData, 0, 256);
	while ((iLen > 0) && (liret == 0))
	{
		if (iLen > MAX_R_W_BINFILE_LEN)
			iReadLen = MAX_R_W_BINFILE_LEN;
		else
			iReadLen = iLen;
		liret = ReadCard_Base(aiddev, iFileID, iOffset, iReadLen, iSig, strData, smartCardInFunc);
		if (liret != 0)
			return -1;
		iLen = iLen - iReadLen;
		iOffset = iOffset + iReadLen;
	}
	return 0;
}

/********************************************************************************
Function��WriteCard_Base
Input��
    iFileID   
    iOffset   
    iLen    :   Length of data(hex)
    strData :   Data write in
Return��
      TRUE
      FALSE
********************************************************************************/

signed int WriteCard_Base(int aiddev, unsigned int iFileID, unsigned int iOffset, unsigned int iLen, unsigned char iSig, unsigned char *strData, const SmartCardInFunc * smartCardInFunc)
{
	unsigned char strCommand[512] = {0};
	unsigned char strCommTemp[3] = {0}, strRetTemp[4] = {0};
	signed int liret;
	unsigned int rlen;
	int i;

	if ((iOffset > 0x00ff) || (iFileID > 31))
	{
		//LOG_PRINTF(("%s:::::::::Case 1", __FUNCTION__));
		//get_char();

		liret = SelectFile(aiddev, iFileID, smartCardInFunc);
		if (liret != 0)
			return -1;

		strCommTemp[0] = (iOffset >> 8) & 0x00FF;
		strCommTemp[1] = iOffset & 0x00FF;
		memset(strRetTemp, 0, sizeof(strRetTemp));
		asc2hex(strCommTemp, 2, strRetTemp, 4);

		memset(strCommand, 0, sizeof(strCommand));
		memcpy(strCommand, "00D6", 4);
		for (i = 0; i < 4; i++)
		{
			strCommand[i + 4] = strRetTemp[i];
		}

		strCommTemp[0] = iLen & 0x00ff;
		memset(strRetTemp, 0, 4);
		asc2hex(strCommTemp, 1, strRetTemp, 2);
		strCommand[8] = strRetTemp[0];
		strCommand[9] = strRetTemp[1];
	}
	else
	{
		strCommTemp[0] = (iFileID + 128);
		strCommTemp[1] = iOffset;
		strCommTemp[2] = iLen;
		memset(strRetTemp, 0, 4);
		asc2hex(strCommTemp, 3, strRetTemp, 6);

		//LOG_PRINTF(("StrRet ==========> '%s'", strRetTemp));
		//get_char();

		memset(strCommand, 0, sizeof(strCommand));
		memcpy(strCommand, "00D6", 4);
		for (i = 0; i < 6; i++)
		{
			strCommand[i + 4] = strRetTemp[i];
		}
		strncat(strCommand, strData, iLen * 2);
	}
	memset(strData, 0, 256);

#ifdef DEV_DEBUG
	//LOG_PRINTF(("%s:::StrCMD => %s", __FUNCTION__, strCommand));
	//get_char();
#endif

	liret = ItexSendCmd(aiddev, strlen(strCommand), strCommand, iSig, &rlen, strData, smartCardInFunc);
	if (liret != 0)
		return liret;

	return 0;
}

signed int WriteCard(int aiddev, unsigned int iFileID, unsigned int iOffset, unsigned int iLen, unsigned char iSig, unsigned char *strData, const SmartCardInFunc * smartCardInFunc)
{
	//unsigned char strCommand[512] = {0};
	//unsigned char strCommTemp[2] = {0}, strRetTemp[4] = {0};
	signed int liret, iWriteLen;
	//unsigned int rlen;
	//int i;

	liret = 0;
	while ((iLen > 0) && (liret == 0))
	{
		if (iLen > MAX_R_W_BINFILE_LEN)
			iWriteLen = MAX_R_W_BINFILE_LEN;
		else
			iWriteLen = iLen;
		liret = WriteCard_Base(aiddev, iFileID, iOffset, iWriteLen, iSig, strData, smartCardInFunc);
		if (liret != 0)
			return -1;
		iLen = iLen - iWriteLen;
		iOffset = iOffset + iWriteLen;
	}
	return 0;
}

/*****************************************************
 Function��AddCheckSum
 Input
		inLen
		inData
 Return
******************************************************/
unsigned char AddCheckSum(unsigned int inLen, unsigned char *inData)
{
	signed int i;
	unsigned char LSum;
	LSum = 0;
	for (i = 0; i < inLen; i++)
	{
		LSum += inData[i];
	}
	return LSum;
}

signed int IsSysCard(int aiddev, const SmartCardInFunc * smartCardInFunc)
{
	signed int liret;
	unsigned char LSum;
	unsigned char strRetData[256] = {0}, strTemp[20] = {0};
	unsigned char strTemp1[50] = {0};

	puts("**********IsSysCard  1");

	if (SelectFile(aiddev, 0x3f01, smartCardInFunc) != 0)
		return -1;

	puts("**********IsSysCard  2");

	liret = ReadCard(aiddev, 1, 0, 0x0c, 1, strRetData, smartCardInFunc);
	if (liret != 0) {
		printf("Error reading card\r\n");
		return -1;
	}

	puts("**********IsSysCard  3");
	//MOD => Commented out, why

	printf("\r\nAddCheckSum Data -> '%s'\r\n", strRetData);
	asc2hex(strRetData, 14, strTemp1, 28);

	 

	LSum = AddCheckSum(0x0c - 1, strRetData);
	if (LSum != strRetData[0x0c - 1]) //AddCheckSum fail
	{
		printf("Error Adding checksum\r\n");
		return -2;
	}

	puts("**********IsSysCard  4");

	memcpy(strTemp, strRetData, 6);
	strTemp[6] = '\0';

	if (strcmp(strTemp, "HNSTAR") != 0)
	{
		printf("Error: Not HNSTAR card, data -> '%s'\r\n", strTemp);
		return -1;
	}

	puts("**********IsSysCard  5");

	memcpy(strTemp, strRetData + 6, 4);
	strTemp[4] = '\0';
	if (strcmp(strTemp, "NEPA") != 0)
	{
		printf("Not Nepa Card\r\n");
		return -1;
	}

	puts("**********IsSysCard  6");

	if (strRetData[0x0a] != 2)
	{
		printf("Final check failed.\r\n");
		return -1;
	}

	puts("**********IsSysCard  7");
	
	return 0;
}

signed int InternalAuth(int aiddev, const SmartCardInFunc * smartCardInFunc)
{
	signed int liret;
	unsigned int rlen;
	//unsigned char strRetData[256] = {0}, strComm[128] = {0}, strTemp[128] = {0}, strTemp2[128] = {0};
	unsigned char strRetData[256] = {0}, strComm[128] = {0}, strTemp2[128] = {0};
	unsigned char strRandom[20] = {0};
	//unsigned char *strzz;

	//LOG_PRINTF("=========> 1");

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);
	if (liret != 0)
		return -1;
	memcpy(strComm, "0084000008", 10);
	liret = ItexSendCmd(aiddev, 10, strComm, 0, &rlen, strRetData, smartCardInFunc); //get 8 byte Random number from USER_CARD
	if (liret != 0)
		return -2;

	memcpy(strRandom, strRetData, 16);
	strRandom[16] = '\0';

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "0088000708", 10);

	strncat(strComm, strRandom, 16);

	//LOG_PRINTF("=========> 2");

	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc); //USER_CARD encrypt calc
	if (liret != 0)
		return -3;

	memcpy(strTemp2, strRetData, rlen); //USER_CARD ciphertext
	strTemp2[rlen] = '\0';

	//LOG_PRINTF("=========> 3");

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_BOTTOM);
	if (liret != 0)
		return -1;

	liret = smartCardInFunc->resetCard(aiddev, strRetData);
	if (liret != 0)
		return -5;

	//LOG_PRINTF("=========> 4");

	liret = SelectFile(aiddev, 0x3f01, smartCardInFunc);
	if (liret != 0)
		return -6;

	//LOG_PRINTF("=========> 5");

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "0088000708", 10);
	strncat(strComm, strRandom, 16);

	//LOG_PRINTF("=========> 6");

	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc); //USER_CARD make a inside encrypt calc
	if (liret != 0)
		return -7;

	//LOG_PRINTF("=========> 7");

	liret = strncmp(strTemp2, strRetData, rlen);
	if (liret != 0)
		return -8;

	//LOG_PRINTF("=========> 8");

	smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);

	//LOG_PRINTF("=========> 8");

	return 0;
}

/*
 * Function:PurchaseAuth
 * Input:lCM_Card_SerNo is  HEX
 * Output:
 * Return
 *
 */
signed int PurchaseAuth(int aiddev, unsigned char *lCM_Card_SerNo, const SmartCardInFunc * smartCardInFunc)
{
	signed int liret;
	unsigned int rlen;
	//unsigned char *strzz;
	unsigned char strRetData[256] = {0}, strComm[128] = {0}, strTemp[128] = {0};
	unsigned char strRandom[20] = {0};

	printf("\r\n%s ========================> 1", __FUNCTION__);

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);
	if (liret != 0)
		return -1;

	printf("\r\n%s ========================> 1.1\n", __FUNCTION__);
/*
	liret = smartCardInFunc->resetCard(aiddev, strRetData);
	if (liret != 0)
		return -5;
*/

	printf("\r\n%s ========================> 1.2", __FUNCTION__);

	liret = SelectFile(aiddev, 0x3f01, smartCardInFunc);
	if (liret != 0)
		return -6;

	printf("\r\n%s ========================> 1.3", __FUNCTION__);

	memcpy(strComm, "0084000008", 10);
	liret = ItexSendCmd(aiddev, 10, strComm, 0, &rlen, strRetData, smartCardInFunc); //get 8 byte Random number from USER_CARD
	if (liret != 0)
		return -2;

	//LOG_PRINTF(("%s ========================> 1.4", __FUNCTION__));

	memcpy(strRandom, strRetData, 16);
	strRandom[16] = '\0';

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_BOTTOM);
	if (liret != 0)
		return -1;

	printf("\r\n%s ========================> 1.5", __FUNCTION__);

	liret = smartCardInFunc->resetCard(aiddev, strRetData);
	if (liret != 0)
		return -5;

	printf("\r\n%s ========================> 1.6", __FUNCTION__);

	liret = SelectFile(aiddev, 0x3f01, smartCardInFunc);
	if (liret != 0)
		return -6;

	printf("\r\n%s ========================> 1.7", __FUNCTION__);

	/***********************************************************/
	/*               PSAM Card Encrypt User CardNo             */
	/***********************************************************/

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "80FA000408", 10);
	strncat(strComm, lCM_Card_SerNo, 16);
	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc);
	if (liret != 0)
		return -7;

	printf("\r\n%s ========================> 1.8", __FUNCTION__);

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "80FA000008", 10);
	asc2hex(strRandom, 8, strTemp, 16);
	strncat(strComm, strTemp, 16);

	printf("\r\n%s ========================> 1.9", __FUNCTION__);

	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc);
	if (liret != 0)
		return -7;

	printf("\r\n%s ========================> 2", __FUNCTION__);

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);
	if (liret != 0)
		return -1;

	printf("\r\n%s ========================> 2.1", __FUNCTION__);

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "0082000408", 10);
	asc2hex(strRetData, 8, strTemp, 16);
	strncat(strComm, strTemp, 16);
	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc);
	if (liret != 0)
		return -1;

	printf("\r\n%s ========================> 2.2", __FUNCTION__);

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);
	if (liret != 0)
		return -1;

	printf("\r\n%s ========================> 2.3", __FUNCTION__);

	return 0;
}

/*
 * Function:ReturnAuth
 * Input:lCM_Card_SerNo is  HEX
 * Output:
 * Return
 *
 */
signed int ReturnAuth(int aiddev, unsigned char *lCM_Card_SerNo, const SmartCardInFunc * smartCardInFunc)
{
	signed int liret;
	unsigned int rlen;
	unsigned char strRetData[256] = {0}, strComm[128] = {0}, strTemp[128] = {0}, strRandom[20] = {0}, strCardReset[28] = {0};

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);
	if (liret != 0)
		return -1;
	liret = SelectFile(aiddev, 0x3f01, smartCardInFunc);
	if (liret != 0)
		return -6;
	memcpy(strComm, "0084000008", 10);
	liret = ItexSendCmd(aiddev, 10, strComm, 0, &rlen, strRetData, smartCardInFunc); //Send user card 8 byte random number
	if (liret != 0)
		return -2;
	memcpy(strRandom, strRetData, 16);
	strRandom[16] = '\0';

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_BOTTOM);
	if (liret != 0)
		return -1;

	liret = smartCardInFunc->resetCard(aiddev, strCardReset);
	if (liret != 0)
		return -5;

	liret = SelectFile(aiddev, 0x3f01, smartCardInFunc);
	if (liret != 0)
		return -6;

	/***********************************************************/
	/*               PSAM Card Encrypt User CardNo             */
	/***********************************************************/

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "80FA000608", 10);
	strncat(strComm, lCM_Card_SerNo, 16);
	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc);
	if (liret != 0)
		return -7;

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "80FA000008", 10);
	asc2hex(strRandom, 8, strTemp, 16);
	strncat(strComm, strTemp, 16);

	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc);
	if (liret != 0)
		return -7;

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);
	if (liret != 0)
		return -1;

	memset(strComm, 0, sizeof(strComm));
	memcpy(strComm, "0082000608", 10);
	asc2hex(strRetData, 8, strTemp, 16);
	strncat(strComm, strTemp, 16);

	//LOG_PRINTF(("%s::::::Before SendCmd", __FUNCTION__));
	//get_char();
	liret = ItexSendCmd(aiddev, 26, strComm, 1, &rlen, strRetData, smartCardInFunc);
	if (liret != 0)
		return -1;

	liret = smartCardInFunc->cardPostion2Slot(&aiddev, CP_TOP);
	if (liret != 0)
		return -1;

	return 0;
}

/*
 * Function:ReadRecord
 * input: 
          FileID 
		  iOffset: file's quantity 
		  iLen: file's length   
 */
signed int ReadRecord(int aiddev, unsigned int iFileID, unsigned int iOffset, unsigned int iLen, unsigned char iSig, unsigned char *strData, const SmartCardInFunc * smartCardInFunc)
{
	signed int liret;
	unsigned char strTemp[128] = {0}, strCommand[128] = {0};
	unsigned int rlen;

	strTemp[0] = 0x00;
	strTemp[1] = 0xB2;
	strTemp[2] = iOffset;
	strTemp[3] = (iFileID * 8 + 4);
	strTemp[4] = iLen;
	asc2hex(strTemp, 5, strCommand, 10);

	memset(strData, 0, 256);
	liret = ItexSendCmd(aiddev, strlen(strCommand), strCommand, iSig, &rlen, strData, smartCardInFunc);
	if (liret != 0)
		return liret;

	return 0;
}
