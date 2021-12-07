/*********************************************************************************************
* �ļ���el.h
* ���ߣ�fuyou
* ˵����elͷ�ļ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
#ifndef _el_h_
#define _el_h_

#include "sys.h"

extern u8 sysMode;

/*********************************************************************************************
* ���ƣ�xor_calculate
* ���ܣ����У�����
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
unsigned char xor_calculate(unsigned char* data,unsigned char begin,unsigned char end);


/*********************************************************************************************
* ���ƣ�buzzer_ioInit
* ���ܣ�io��ʼ����P04
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void buzzer_ioInit();

/*********************************************************************************************
* ���ƣ�reported_icCardNumber
* ���ܣ��ϱ�IC����
* ��������
* ���أ�0-->��ȡʧ�ܣ�1-->��ȡ�ɹ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
u8 reported_icCardNumber(void);

/*********************************************************************************************
* ���ƣ�reported_icRemaining
* ���ܣ��ϱ�ic�����
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void reported_icRemaining(void);

/*********************************************************************************************
* ���ƣ�reported_icInfo
* ���ܣ��ϱ��û���Ϣ�����ţ����
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void reported_icInfo(void);

/*********************************************************************************************
* ���ƣ�reported_idInfo
* ���ܣ��ϱ�ID����Ϣ
* ������
* ���أ�
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
u8 reported_idInfo();

/*********************************************************************************************
* ���ƣ�pc_el
* ���ܣ�pc����el
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void pc_el();


#endif