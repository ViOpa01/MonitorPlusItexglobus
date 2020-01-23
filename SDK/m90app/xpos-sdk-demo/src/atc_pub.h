/*! \file atc_pub.h
	\brief AT�����װ
*	\author lx
*	\date 2014/02/08
*/
#pragma once

#include "pub/pub.h"

#ifdef __cplusplus
extern "C"{
#endif


//#define SOCK_HEX_CODE

#define AT_ERROR    -1
#define AT_NO_ECHO  -2

/**
* @brief ����apn
* @param apn apn����
* @return 0�ɹ�
*/
LIB_EXPORT int ap_set_apn(char *apn);
/**
* @brief ��ȡimei
* @param buff ������
* @param len ������λ��
* @return 0�ɹ�
*/
LIB_EXPORT int ap_get_imei(char *buff , int len);
/**
* @brief ��ȡģ��汾
* @param buff ������
* @param len ����������
* @return 0�ɹ�
*/
LIB_EXPORT int ap_get_model_ver(char *buff , int len);
/**
* @brief ����at����ȴ�����
* @param cmd ����
* @param timeover ��ʱʱ��
* @param retstr ����ָ��
* @param len ָ���
* @param count �ط�����
* @return 0�ɹ�
*/
LIB_EXPORT int ag_send_wait_return(char *cmd , int timeover , char * retstr , int len , int count);

LIB_EXPORT int ap_send_tts(char *buff);

/**
* @brief ���û���
* @param val ����ֵ
* @return 0�ɹ�
*/
LIB_EXPORT int ap_set_e_val(int val);
LIB_EXPORT int ap_get_echo();
/**
* @brief ��ȡcsq
* @param csq_val ������
* @return 0�ɹ�
*/
LIB_EXPORT int ap_get_csq(int *csq_val);
/**
* @brief ��ȡcreg
* @param creg_val ������
* @return 0�ɹ�
*/
LIB_EXPORT int ap_get_creg(int *creg_val);
LIB_EXPORT int ap_get_cgatt();

// ����apn
LIB_EXPORT int ap_set_apn_ex(int idx, char *apn_name, char *username, char *passwd, int auth, int ipv);
/**
* @brief ��ȡpin
* @return 0�ɹ�
*/
LIB_EXPORT int ap_get_cpin();
/**
* @brief ��ȡimei
* @param buff ������
* @param len ����������
* @return 0�ɹ�
*/
LIB_EXPORT int ap_get_imsi(char *buff , int len);


/**
* @brief ATC��ʼ��
* @return 
*/
LIB_EXPORT int atc_init();
LIB_EXPORT void atc_win32_init();
/**
* @brief ��ȡ�Ƿ���ע������
* @return 0δע��  1��ע��
*/
LIB_EXPORT int atc_isreg();
/**
* @brief ��ȡcsq ֵ
* @return csqֵ
*/
LIB_EXPORT int atc_csq();
/**
* @brief ��ȡimei
* @return imeiֵ
*/
LIB_EXPORT const char * atc_imei();
/**
* @brief ��ȡimsi
* @return imsiֵ
*/
LIB_EXPORT const char * atc_imsi();

/**
* @brief ��ȡ�źŸ���
* @return 0 δע��������  1-4�źŸ���
*/
LIB_EXPORT int atc_signal();

/**
* @brief ��ȡCPIN״̬
* @return 0ʧ�� 1���� 2���� 3puk
*/
LIB_EXPORT int atc_cpin();


/**
* @brief ��ȡģ��汾��
* @return ģ��汾��
*/
LIB_EXPORT const char * atc_model_ver();
LIB_EXPORT int atc_scan_authorize();

LIB_EXPORT int atc_cell();
LIB_EXPORT int atc_lac();

typedef enum
{
	MODULE_SIMCOM = 0,		//Simcomģ��
	MODULE_SPREADTRUM = 1,	//չѶģ��(AT188MA)
	MODULE_M62 = 2,			//M62ģ�� ,M35ģ��
	MODULE_AT139D = 3,		//AT139D
	MODULE_MC509 = 4,		//��ΪMC509
	MODULE_A8500 = 5,		//A8500

}MODULE_TYPE_E;

LIB_EXPORT MODULE_TYPE_E atc_getModuleType();

#define MODULE_IS_M35	atc_getModuleType() == MODULE_M62
#define MODULE_IS_A8500	atc_getModuleType() == MODULE_A8500



LIB_EXPORT const char * atc_iccid();


//�趨�ⲿ������ AT+CCED=0,2  ���ڷ�������
LIB_EXPORT void atc_setccedbuff(char *pbuff, int nbufflen);

typedef struct _cceditem{
	unsigned short	MCC;					//�ƶ����Һ���
	unsigned char	MNC;					//�ƶ��������
	unsigned short	LAC;					//λ�������
	unsigned short	CELL;					//С����
	unsigned char	BSIC;					//��վ��ʾ��
	unsigned char	BCCH_Freq_absolute;		//BCCH �ŵ���
	unsigned char	RxLev;					//�����ź�ǿ��
}ccedItem;

LIB_EXPORT ccedItem * atc_getcceditems( int *outcount);

LIB_EXPORT int atc_modelreset_set();

LIB_EXPORT int atc_play_file(char * file, int timeover);

#ifdef __cplusplus
}
#endif	