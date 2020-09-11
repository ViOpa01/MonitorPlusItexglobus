/*
 * basefun.h
 *
 *  Created on: 2011-12-1
 *      Author: xzj
 */

#ifndef BASEFUN_H_
#define BASEFUN_H_

#define PSAM_CARD 0x82

#ifdef __cplusplus
extern "C" {
#endif

/*
struct Card_info
{
   //������Ϣ
	unsigned char	CM_Card_SerNo[32];  //�����к�
	unsigned char CM_UserNo[18];//�û���
    unsigned char CM_Card_Status;//��״̬  01����,02����,03ȡ��
    unsigned short int  CM_Purchase_Times; //�������
    double CM_Purchase_Power;//�������
   // struct tm CM_Purchase_DateTime; //����ʱ�� //YYYYMMDDHHmmSS
	unsigned char CM_Purchase_DateTime[15];
    double  CM_Alarm_L1; //����1
    double  CM_Alarm_L2; //����2
    double  CM_Alarm_L3; //����3
    float     CM_Load_Thrashold; //��������
    double CM_User_Demand; //�û�����
    unsigned short int CM_Arrearage_Months;//��Ƿ������
    double  CM_Arrearage_Amount; //��Ƿ����
    unsigned char CM_Card_Ver; //���汾��

    //��д��Ϣ
    unsigned char CF_Meter_ID[15];//����
    double CF_Refound_Power; //ȡ����
    unsigned char CF_MeterStatus;//���״̬��;
    unsigned short int CF_Load_Times;//װ�����
    double CF_Remain_Power; //ʣ�����
    double CF_TotalUsed_Power;//���õ���
    double CF_TotalLoad_Power; //��װ����
    double CF_Unreg_TotalPower;//��������������.
    double CF_Over_Power;//�������
    double CF_PreLoad_Power;//�ϴ�װ����
    unsigned char  CF_PreLoad_DateTime[7];//�ϴ�װ��ʱ��
    double CF_CurLoad_Power;//����װ����
    unsigned char  CF_CurLoad_DateTime[7];//����װ��ʱ��
    unsigned char CF_LoadYN;//�Ƿ���װ��   1��װ�� 0 δװ��
   //������Ϣ
    unsigned char CD_SerCen_No[7];//�������Ĵ���
    unsigned char CD_UserName[32];//�û�����
    unsigned short int CD_TarrNo; //�������ͱ��
    double CD_Security_Deposit;//��ȫ��֤��
    double CD_ConnectFee;//���߷�
    double CD_ReConnectFee;//�ٽ��߷�
    double CD_ProFund;//���̻���
    unsigned char CD_UserOldAccNo[20];//�û����ʺ�
    double CD_Pre_Amount; //Ԥ�����
    double CD_Pre_Cash;//Ԥ�õ��
    double CD_Meter_Pur;//������÷�
    unsigned short int CD_Rec_Months;//���յ�����

   //PSAM����Ϣ
    double CP_Remain_Auth; //PSAMʣ����Ȩ
};
*/

enum TCardPosition
{
  CP_TOP,
  CP_BOTTOM,
  CP_BOTTOM2,
  CP_BOTTOM3,
  CP_RADIO_CARD,
  CP_GPRS
};

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

typedef int (*CardPostion2SlotCB)(int * slot, const enum TCardPosition CardPos);
typedef int (*ResetCardCB)(int slot, unsigned char *adatabuffer);
typedef int (*CloseSlotCB)(int slot);
typedef char (*IccIsoCommandCB)(int slot, APDU_SEND *apduSend, APDU_RESP *apduRecv);

typedef void (*GetDateTimeBcdCB)(unsigned char yymdhmsBcd[7]);
typedef int (*DetectPsamCB) (void);

typedef int (*PrintCustomerCardInfoCB)(void *cardinfo, const char *logoPath);

typedef void (*RemoveCustomerCardCb)(const char *title, const char *desc);

typedef int (*DetectSmartCardCb)(char *title, char *msg, int timeoverMs);

typedef struct
{
  ResetCardCB resetCard;
  CardPostion2SlotCB cardPostion2Slot;
  IccIsoCommandCB exchangeApdu;
  CloseSlotCB closeSlot;
  GetDateTimeBcdCB getDateTimeBcd;
  DetectPsamCB detectPsamCB;
  PrintCustomerCardInfoCB printCustomerCardInfoCb;
  RemoveCustomerCardCb removeCustomerCardCb;
  DetectSmartCardCb detectSmartCardCb;
  short isDebug;
} SmartCardInFunc;

//signed int Set_Nad(int *aicdev, enum TCardPosition CardPos);
//signed int smartCardInFunc->resetCard(int aicdev, unsigned char *adatabuffer);
void asc2hex(char *ascStr, unsigned int ascLen, unsigned char *hexStr, unsigned int hexLen);
signed int ItexSendCmd(int aicdev, unsigned int lilen, unsigned char *strCmd, unsigned char iSig, unsigned int *arlen, unsigned char *strRet, const SmartCardInFunc * smartCardInFunc);
signed int SelectFile(int aicdev, unsigned int iFileID, const SmartCardInFunc * smartCardInFunc);
signed int ReadCard(int aicdev, unsigned int iFileID, unsigned int iOffset, unsigned int iLen, unsigned char iSig, unsigned char *strData, const SmartCardInFunc * smartCardInFunc);
signed int IsSysCard(int aiddev, const SmartCardInFunc * smartCardInFunc);
signed int InternalAuth(int aiddev, const SmartCardInFunc * smartCardInFunc);
signed int PurchaseAuth(int aiddev, unsigned char *lCM_Card_SerNo, const SmartCardInFunc * smartCardInFunc);
signed int ReturnAuth(int aiddev, unsigned char *lCM_Card_SerNo, const SmartCardInFunc * smartCardInFunc);
signed int ReadRecord(int aiddev, unsigned int iFileID, unsigned int iOffset, unsigned int iLen, unsigned char iSig, unsigned char *strData, const SmartCardInFunc * smartCardInFunc);
void BCDToASC(unsigned char * asc_buf, unsigned char * bcd_buf, int n);
#ifdef __cplusplus
}
#endif

#endif /* BASEFUN_H_ */
