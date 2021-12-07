/*********************************************************************************************
* 文件：uart.c
* 作者：fuyou 2018.03.21
* 说明：etc卡驱动
* 修改：
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "eh.h"
#include "sys_init.h"
#include "uart.h"
#include "ic.h"


/*********************************************************************************************
* 名称：CRC16_CCITT_FALSE
* 功能：CRC校验
* 参数：无
* 返回：校验结果
* 修改：
* 注释：
*********************************************************************************************/
unsigned short CRC16_CCITT_FALSE(unsigned char *puchMsg, unsigned int s1,unsigned int s2)  
{  
	unsigned short wCRCin = 0xFFFF;  
	unsigned short wCPoly = 0x1021;  
	unsigned char  wChar = 0;  

	for(u8 x=s1;x<s2+1;x++)
	{  
		wChar = puchMsg[x];  
		wCRCin ^= (wChar << 8);  
		for(int i = 0;i < 8;i++)  
		{  
			if(wCRCin & 0x8000)  
				wCRCin = (wCRCin << 1) ^ wCPoly;  
			else  
				wCRCin = wCRCin << 1;  
		}  
	}  
  return (wCRCin) ;  
}


/*********************************************************************************************
* 名称：mcuRead_etcUII
* 功能：读UII
* 参数：EPC数组
* 返回：1：成功，0：失败
* 修改：
* 注释：
*********************************************************************************************/
unsigned char mcuRead_etcUII(unsigned char* UIIarray)
{	
	unsigned char i,check_temp[3];
	u16 crc_16;
	/*读ETC UII指令*/
	u8 readEtcUII[9] = {0xbb,0x00,0x22,0x00,0x00,
						0x7e,0x00,0x00};

	crc_16 = CRC16_CCITT_FALSE(readEtcUII,1,5);				//计算校验
	readEtcUII[6] = (unsigned char)(crc_16 >> 8);
	readEtcUII[7] = (unsigned char)(crc_16 & 0xff);
	
	Uart0_Send_LenString(readEtcUII,8);							//发送读卡号命令
	delay_ms(50);
		
	if((UART0_RX_STA&0x80)==0x80)
	{	
		if((U0RX_Buf[1]==0x01)&&(U0RX_Buf[2]==0x22))			//操作成功
		{	
			crc_16 = CRC16_CCITT_FALSE(U0RX_Buf,1,(UART0_RX_STA&0x3F)-3);//计算校验
			check_temp[0] = (unsigned char)(crc_16 >> 8);
			check_temp[1] = (unsigned char)(crc_16 & 0xff);
			
			if((check_temp[0]==U0RX_Buf[(UART0_RX_STA&0x3F)-2])&&(check_temp[1]==U0RX_Buf[(UART0_RX_STA&0x3F)-1]))//校验正确
			{	
				for(i=0;i<12;i++)
				{
					UIIarray[i] = U0RX_Buf[i+7];				//获取卡号
				}
				UART0_RX_STA = 0;
				return 1;
			}
		}
		UART0_RX_STA = 0;
	}
	return 0;
}


/*********************************************************************************************
* 名称：mcuRead_EtcMemory
* 功能：读指定扇区
* 参数：密码，卡号，存储区域(mb)，开始地址，读取长度，读取数据
* 返回：1：成功，0：失败
* 修改：
* 注释：
*********************************************************************************************/
unsigned char mcuRead_EtcMemory(	unsigned char* password,unsigned char* epc,unsigned char mb,
							 		unsigned short sAdd,unsigned short len,unsigned char* Data)
{
	u16 i;
	unsigned char check_temp[3];
	/*读扇区命令*/
	unsigned char readMem[32] = {	0xbb,0x00,0x29,0x00,0x17,
									password[0],password[1],password[2],password[3],
									0x00,0x0c,epc[0],epc[1],epc[2],epc[3],epc[4],epc[5],epc[6],epc[7],epc[8],epc[9],epc[10],epc[11],
									mb,(u8)(sAdd>>8),(u8)(sAdd&0xff),(u8)(len>>8),(u8)(len&0xff),0x7e,0x00,0x00};	
	
	u16 crc_16 = CRC16_CCITT_FALSE(readMem,1,28);				//计算校验
	readMem[29] = (unsigned char)(crc_16 >> 8);				//计算校验
	readMem[30] = (unsigned char)(crc_16 & 0xff);				//计算校验
	Uart0_Send_LenString(readMem,31);							//发送读卡号命令
	delay_ms(50);
	
	if((UART0_RX_STA&0x80)==0x80)
	{
		if((U0RX_Buf[1]==0x01)&&(U0RX_Buf[2]==0x29))									//操作成功
		{
			crc_16 = CRC16_CCITT_FALSE(U0RX_Buf,1,(UART0_RX_STA&0x3F)-3);//CRC校验
			check_temp[0] = (unsigned char)(crc_16 >> 8);
			check_temp[1] = (unsigned char)(crc_16 & 0xff);
			
			if((check_temp[0]==U0RX_Buf[(UART0_RX_STA&0x3F)-2])&&(check_temp[1]==U0RX_Buf[(UART0_RX_STA&0x3F)-1]))//校验正确
			{	
				for(i=0;i<len*2;i++)
				{
					Data[i] = U0RX_Buf[i+5];					//获取扇区数据
				}
				UART0_RX_STA = 0;
				return 1;
			}
		}
		UART0_RX_STA = 0;
	}
	return 0;
}


/*********************************************************************************************
* 名称：mcuWrite_Etc2Byte
* 功能：写指定扇区,2个字节
* 参数：密码，卡号，存储区域，开始地址，数据
* 返回：1：成功，0：失败
* 修改：
* 注释：
*********************************************************************************************/
unsigned char mcuWrite_Etc2Byte(	unsigned char* password,unsigned char* epc,unsigned char mb,
							 		unsigned short sAdd,unsigned char* Data)
{
	unsigned char check_temp[3];
	/*读扇区命令*/
	unsigned char writeMem[34] = {	0xbb,0x00,0x46,0x00,0x19,
									password[0],password[1],password[2],password[3],
									0x00,0x0c,epc[0],epc[1],epc[2],epc[3],epc[4],epc[5],epc[6],epc[7],epc[8],epc[9],epc[10],epc[11],
									mb,(u8)(sAdd>>8),(u8)(sAdd&0xff),0x00,0x01,Data[0],Data[1],
									0x7e,0x00,0x00};	
	
	u16 crc_16 = CRC16_CCITT_FALSE(writeMem,1,30);				//计算校验
	writeMem[31] = (unsigned char)(crc_16 >> 8);				//计算校验
	writeMem[32] = (unsigned char)(crc_16 & 0xff);				//计算校验
	
	Uart0_Send_LenString(writeMem,33);							//发送写命令
	delay_ms(50);
	
	if((UART0_RX_STA&0x80)==0x80)
	{
		if((U0RX_Buf[1]==0x01)&&(U0RX_Buf[2]==0x46)&&(U0RX_Buf[5]==0x00))//操作成功
		{
			crc_16 = CRC16_CCITT_FALSE(U0RX_Buf,1,(UART0_RX_STA&0x3F)-3);//CRC校验
			check_temp[0] = (unsigned char)(crc_16 >> 8);
			check_temp[1] = (unsigned char)(crc_16 & 0xff);
			
			if((check_temp[0]==U0RX_Buf[(UART0_RX_STA&0x3F)-2])&&(check_temp[1]==U0RX_Buf[(UART0_RX_STA&0x3F)-1]))//校验正确
			{	
				UART0_RX_STA = 0;
				return 1;
			}
		}
		UART0_RX_STA = 0;
	}
	return 0;
}



/*********************************************************************************************
* 名称：etc_userinfo
* 功能：读取用户信息
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void etc_userinfo(void)
{	
	unsigned char i;
	
	/*操作成功返回数组*/
	unsigned char sendCorrect[19] = {	0xbb,0x0E,0xE1,
										0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
										0x00,0x00,
										0x00};
	/*密码数组*/
	unsigned char etcPassword[5] = {0x00,0x00,0x00,0x00};
	unsigned char etcEPC[13] = {0};
	unsigned char etcData[3] = {0};
	
	/*读EPC*/
	if(mcuRead_etcUII(etcEPC))
	{		
		for(i=0;i<12;i++)
		{
			sendCorrect[i+3] = etcEPC[i];						//获取EPC
		}
		
		/*读余额*/
		if(mcuRead_EtcMemory(etcPassword,etcEPC,0x03,0x0000,0x0001,etcData))
		{
			for(i=0;i<2;i++)
			{
				sendCorrect[i+15] = etcData[i];						//获取余额
			}
			sendCorrect[17] = xor_calculate(sendCorrect,1,16);			//计算校验
			Uart1_Send_LenString(sendCorrect,18);					//操作成功
		}
	}	
	UART1_RX_STA = 0;	
}


/*********************************************************************************************
* 名称：etc_topUp
* 功能：etc充值
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void etc_topUp(u8* etcData)
{
	u8 i,CorrectFlag=0;
	/*操作失败返回数组*/
	unsigned char readError[5] = {	0xbb,0x00,0xff,
									0x00};
	/*操作成功返回数组*/
	unsigned char sendCorrect[5] = {	0xbb,0x00,0x00,
										0x00};
	/*密码*/
	unsigned char etcPassword[5] = {0x00,0x00,0x00,0x00};
	/*EPC*/
	unsigned char etcEPC[13] = {0};
		
	for(i=0;i<U1RX_Buf[1];i++)
	{
		etcData[1-i] = U1RX_Buf[((UART1_RX_STA&0x3F)-2)-i];		//更新充值数据	
	}
	
    //读5次，提高成功率
    for(i=0;i<5;i++)
    {
        /*读EPC，写余额*/
        if((mcuRead_etcUII(etcEPC)) && (mcuWrite_Etc2Byte(etcPassword,etcEPC,0x03,0x0000,etcData)))
        {				
            sendCorrect[3] = xor_calculate(sendCorrect,1,2);			//计算校验
            Uart1_Send_LenString(sendCorrect,4);					//返回成功信息
            CorrectFlag = 1;                                        //标记成功
            break;
        }
    }
    
	if(!CorrectFlag)
	{
		readError[3] = xor_calculate(readError,1,2);				//计算校验
		Uart1_Send_LenString(readError,4);						//返回失败信息
	}
}



/*********************************************************************************************
* 名称：sticks_ioInit
* 功能：栏杆io初始化，P00，P01
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sticks_ioInit()
{
	P0SEL &= ~(1<<0);											//同用io
	P0SEL &= ~(1<<1);											//同用io
	P0DIR |= (1<<0);											//设置为输出
	P0DIR |= (1<<1);
}



/*********************************************************************************************
* 名称：sticks_up
* 功能：抬杆
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sticks_up()
{
	P0_0 = 0;
	P0_1 = 1;
	delay_ms(400);
	P0_0 = 0;
	P0_1 = 0;	
}

/*********************************************************************************************
* 名称：sticks_ioInit
* 功能：闭杆
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void sticks_down()
{
	P0_0 = 1;
	P0_1 = 0;
	delay_ms(400);
	P0_0 = 0;
	P0_1 = 0;		
}


/*********************************************************************************************
* 名称：pc_etc
* 功能：pc控制etc卡
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void pc_etc()
{	
	u8 check_temp;
	static u16 x2=0;
	u8 etcData[3] = {0};
	
	if((UART1_RX_STA&0x80)==0x80)								//数据接收完成
	{
		if(U1RX_Buf[0]==0xfa)									//确认数据头
		{		
			check_temp = xor_calculate(U1RX_Buf,1,(UART1_RX_STA&0x3F)-2);//异或校验
			
			if(check_temp==U1RX_Buf[(UART1_RX_STA&0x3F)-1])		//校验正确
			{
				switch(U1RX_Buf[2])
				{
					case 0xe2:	
						/*读取充值数据*/
						etcData[0] = U1RX_Buf[(UART1_RX_STA&0x3F)-3];
						etcData[1] = U1RX_Buf[(UART1_RX_STA&0x3F)-2];
						/*充值*/
						etc_topUp(etcData);						
						break;
					case 0xe3:
						sticks_up();
						break;
					case 0xe4:	
						sticks_down();
						break;
				}
			}
		}
		UART1_RX_STA = 0;
	}
	else
	{
		if(x2>999)
		{
			etc_userinfo();										//读取用户信息
			
			LED2 = 0;
			delay_ms(1);											//闪灯
			LED2 = 1;
			x2 = 0;
		}	
		
		delay_ms(1);
		x2++;
	}
}