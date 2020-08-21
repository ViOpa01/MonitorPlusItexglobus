#ifndef CPUCARD_FUN
#define CPUCARD_FUN


#define PSAM_CARD	0x82

#include "basefun.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
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
}  La_Card_info;
//;

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

int readUserCard(La_Card_info * userCardInfo, unsigned char *outData, const unsigned char *inData, const SmartCardInFunc * smartCardInFunc);

/**
 * Function: logCardInfo
 * Usage: logCardInfo(&userCardInfo);
 * -----------------------------------
 * Logs user card info struct, see La_Card_info
 *
 * @author Ajani Opeyemi Sunday.
 * @Data 09/04/2017
 */
 

/**
 * Function: updateUserCard
 * Usage: int ret = updateUserCard(outData, &LUserCi);
 * ---------------------------------------------------
 * @param userCardInfo Struct containing customer card's info, see La_Card_info.
 * @param psamBalance balance on Psam after transaction.
 * @author Ajani Opeyemi Sunday.
 * @Data 08/11/2017
 */

int updateUserCard(unsigned char *psamBalance, La_Card_info * LUserCi, const SmartCardInFunc * smartCardInFunc);

char * unistarErrorToString(short errorCode);

#ifdef __cplusplus
}
#endif

#endif

