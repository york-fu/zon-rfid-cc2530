/*********************************************************************************************
* 文件：el.c
* 作者：fuyou 2018.5.23
* 说明：el公交刷卡系统驱动，el考勤系统驱动  
* 修改：
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include "el.h"
#include "led.h"
#include "uart.h"
#include "oled.h"
#include "relay.h"
#include "time.h"


//公交刷卡系统票价
u16 sysPrice = 2;		

/*ic卡模式,0-->消费模式，1-->充值模式*/
u8 icMode = 0;
/*ic卡A组密码*/
u8 icPasswordA[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
/*ic卡B组密码*/
u8 icPasswordB[6] = {0xff,0xff,0xff,0xff,0xff,0xff};


/*********************************************************************************************
* 名称：buzzer_ioInit
* 功能：io初始化，P04
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void buzzer_ioInit()
{
	P0SEL &= ~(1<<4);											//通用io
	P0DIR |= (1<<4);											//设置为输出
}


u32 myPow(u16 x,u16 n)
{
    u32 num=1;
    while(n--)
    {
        num *= x;
    }
    return num;
}


/*********************************************************************************************
* 名称：oled_display
* 功能：oled显示
* 参数：公交系统票价,ic卡余额
* 返回：无
* 修改：
* 注释：余额输入0xffffffff，只改票价，不改余额
*********************************************************************************************/
void oled_display(u16 price,u32 surplus)
{
    char tempBuf[16]={0};
    u8 len=0;
    
    sprintf(tempBuf,"%u",price);
    oled_areaClear(2,2,48,95);
    OLED_ShowString(48,2,(u8*)tempBuf,8);						    //显示票价，10进制
    
    if(surplus!=0xffffffff)
    {
        for(len=0;len<(sizeof tempBuf);len++)
        {
            tempBuf[len] = 0;
        }
        sprintf(tempBuf,"%lu",surplus);
        oled_areaClear(3,3,48,95);
        OLED_ShowString(48,3,(u8*)tempBuf,8);					    //显示余额，10进制
    }
}


/*********************************************************************************************
* 名称：xor_count
* 功能：异或校验计算
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
unsigned char xor_count(unsigned char* array,unsigned char s1,unsigned char s2)
{
	unsigned char i,check_temp;
	
	check_temp = array[s1];
	for(i = s1+1;i<(s2+1);i++)
	{	
		check_temp ^= array[i];									//异或校验
	}
	
	return check_temp;
}



/*********************************************************************************************
* 名称：mcuRead_UID
* 功能：读UID
* 参数：读取地址，卡号BUF
* 返回：1：成功，0：失败
* 修改：
* 注释：
*********************************************************************************************/
unsigned char mcuRead_UID(unsigned char icAdd,unsigned char* UIDarray)
{	
	unsigned char i=0,check_temp=0;
	/*读IC卡命令*/
	unsigned char readUID[7] = {	0xAB,0xBA,
									icAdd,0x10,0x00,
									0x00};	
	readUID[5] = xor_count(readUID,2,4);						//计算校验
	Uart0_Send_LenString(readUID,6);							//发送读卡号命令
    
    while((UART0_RX_STA&0x80)!=0x80)
    {
        delay_ms(1);
        i++;
        if(i>49) break;
    }
		
	if((UART0_RX_STA&0x80)==0x80)
	{	
		if(U0RX_Buf[3]==0x81)									//操作成功
		{	
			check_temp = xor_count(U0RX_Buf,2,(UART0_RX_STA&0x7f)-1);//异或校验
			
			if(check_temp==U0RX_Buf[UART0_RX_STA&0x7f])		//校验正确
			{	
				for(i=0;i<4;i++)
				{
					UIDarray[i] = U0RX_Buf[i+5];				//获取卡号
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
* 名称：mcuRead_memory
* 功能：读指定扇区
* 参数：IC卡地址，扇区，块，A/B组，密码，数据
* 返回：1：成功，0：失败
* 修改：
* 注释：
*********************************************************************************************/
unsigned char mcuRead_memory(	unsigned char icAdd,unsigned char M1,unsigned char M2,
							 	unsigned char group,unsigned char* password,unsigned char* Data)
{
	unsigned char i=0,check_temp=0;
	/*读扇区命令*/
	unsigned char readMem[16] = {	0xAB,0xBA,
									icAdd,0x12,0x09,
									M1,M2,group,
									password[0],password[1],password[2],password[3],password[4],password[5],
									0x00};	
	
	readMem[14] = xor_count(readMem,2,13);						//计算校验
	Uart0_Send_LenString(readMem,15);							//发送读卡号命令
    
    while((UART0_RX_STA&0x80)!=0x80)
    {
        delay_ms(1);
        i++;
        if(i>49) break;
    }
	
	if((UART0_RX_STA&0x80)==0x80)
	{
		if(U0RX_Buf[3]==0x81)									//操作成功
		{
			check_temp = xor_count(U0RX_Buf,2,(UART0_RX_STA&0x7f)-1);//异或校验
			
			if(check_temp==U0RX_Buf[UART0_RX_STA&0x7f])		//校验正确
			{		
				for(i=0;i<16;i++)
				{
					Data[i] = U0RX_Buf[i+5+2];					//获取扇区数据
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
* 名称：mcuWrite_memory
* 功能：写定扇区
* 参数：IC卡地址，扇区，块，A/B组，密码，数据
* 返回：1：成功，0：失败
* 修改：
* 注释：
*********************************************************************************************/
unsigned char mcuWrite_memory(	unsigned char icAdd,unsigned char M1,unsigned char M2,
							 	unsigned char group,unsigned char* password,unsigned char* Data)
{
	unsigned char i=0,check_temp=0;
	/*读扇区命令*/
	unsigned char readMem[32] = {	0xAB,0xBA,
									icAdd,0x13,0x19,
									M1,M2,group,
									password[0],password[1],password[2],password[3],password[4],password[5],
									Data[0],Data[1],Data[2],Data[3],Data[4],Data[5],Data[6],Data[7],
									Data[8],Data[9],Data[10],Data[11],Data[12],Data[13],Data[14],Data[15],
									0x00};	
	
	readMem[30] = xor_count(readMem,2,29);						//计算校验
	Uart0_Send_LenString(readMem,31);							//发送读卡号命令
    
    while((UART0_RX_STA&0x80)!=0x80)
    {
        delay_ms(1);
        i++;
        if(i>49) break;
    }
	
	if((UART0_RX_STA&0x80)==0x80)
	{
		if(U0RX_Buf[3]==0x81)									//操作成功
		{
			check_temp = xor_count(U0RX_Buf,2,(UART0_RX_STA&0x7f)-1);//异或校验
			
			if(check_temp==U0RX_Buf[UART0_RX_STA&0x7f])		//校验正确
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
* 名称：reported_icCardNumber
* 功能：上报IC卡号
* 参数：无
* 返回：0-->读取失败，1-->读取成功
* 修改：
* 注释：
*********************************************************************************************/
u8 reported_icCardNumber(void)
{
	unsigned char i;
	
	/*操作成功返回数组*/
	unsigned char sendCorrect[9] = {	0xBF,0x04,0x0A,
										0x00,0x00,0x00,0x00,
										0x00};
	
	unsigned char cardUID[5] = {0};
	
	if(mcuRead_UID(0x00,cardUID))								//读取成功
	{	
		for(i=0;i<4;i++)
		{
			sendCorrect[i+3] = cardUID[i];						//获取卡号
		}
		sendCorrect[7] = xor_count(sendCorrect,1,6);			//计算校验
		Uart1_Send_LenString(sendCorrect,8);					//操作成功
        return 1;
	}
    return 0;
}


/*********************************************************************************************
* 名称：reported_icRemaining
* 功能：上报ic卡余额
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void reported_icRemaining(void)
{
	unsigned char i;
	
	/*操作成功返回数组*/
	unsigned char sendCorrect[21] = {	0xBF,0x10,0x0B,
										0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
										0x00};
	unsigned char icData[17] = {0};
	
	if(mcuRead_memory(0x00,0x01,0x01,0x0A,icPasswordA,icData))
	{
		for(i=0;i<16;i++)
		{
			sendCorrect[i+3] = icData[i];						//获取扇区数据
		}
		sendCorrect[19] = xor_count(sendCorrect,1,18);			//计算校验
		Uart1_Send_LenString(sendCorrect,20);					//操作成功
	}
}



/*********************************************************************************************
* 名称：reported_icInfo
* 功能：上报用户信息，卡号，余额
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void reported_icInfo()
{
	unsigned char i;
	
	/*操作成功返回数组*/
	unsigned char sendCorrect[13] = {	0xBF,0x08,0x0B,
										0x00,0x00,0x00,0x00,	//卡号
										0x00,0x00,0x00,0x00,	//余额
										0x00};
	/*存放卡号*/
	unsigned char cardUID[5] = {0};
    /*存放数据*/
	unsigned char icData[17] = {0};
    u32 icSurplus=0;
	
	if(mcuRead_UID(0x00,cardUID))								//读取卡号
	{	
		for(i=0;i<4;i++)
		{
			sendCorrect[i+3] = cardUID[i];						//获取卡号
		}
		
		if(mcuRead_memory(0x00,0x01,0x01,0x0a,icPasswordA,icData))//读余额
		{
			for(i=0;i<4;i++)
			{
				sendCorrect[10-i] = icData[15-i];				//获取余额
			}
			sendCorrect[11] = xor_count(sendCorrect,1,10);		//计算校验
			Uart1_Send_LenString(sendCorrect,12);				//返回信息
            ledFlickerSet(2);
            
            //16进制转10进制
            for(u8 i=0;i<4;i++)
            {
                icSurplus += icData[15-i]*myPow(256,i);
            }
            
            oled_display(sysPrice,icSurplus);
            
            /*蜂鸣器提示*/
            if(icSurplus>sysPrice)
            {
                P0_4 = 0;
                delay_ms(20);
                P0_4 = 1;          
            }
            else
            {
                P0_4 = 0;
                delay_ms(20);
                P0_4 = 1;   
                delay_ms(200);
                P0_4 = 0;
                delay_ms(20);
                P0_4 = 1;   
                delay_ms(200);
                P0_4 = 0;
                delay_ms(20);
                P0_4 = 1;   
            }
		}
	}
}



/*********************************************************************************************
* 名称：update_icData
* 功能：修改ic卡数据
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void update_icData()
{
	u8 i,group;
    u32 icSurplus=0;
    u8* password;
	/*操作失败返回数组*/
	unsigned char readError[6] = {	0xBF,0x01,0x0b,
									0xff,
									0x00};
	/*操作成功返回数组*/
	unsigned char sendCorrect[6] = {	0xBF,0x01,0x0B,
										0x00,
										0x00};
	/*要写的数据*/
	unsigned char WriteData[17] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
									0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		
    /*根据模式选择A/B组密码*/
    if(icMode==1)
    {
        group = 0x0b;
        password = icPasswordB;
    }
    else
    {     
        group = 0x0a;
        password = icPasswordA;
    }
    
    //更新数据
	for(i=0;i<U1RX_Buf[1];i++)
	{
		WriteData[15-i] = U1RX_Buf[((UART1_RX_STA&0x7f)-1)-i];		
	}
    
	//写数据
	if(mcuWrite_memory(0x00,0x01,0x01,group,password,WriteData))
	{
		sendCorrect[4] = xor_count(sendCorrect,1,3);			//计算校验
		Uart1_Send_LenString(sendCorrect,5);					//操作成功
        ledFlickerSet(2);
		UART1_RX_STA = 0;
        
        //16进制转10进制
        for(u8 i=0;i<4;i++)
        {
            icSurplus += WriteData[15-i]*myPow(256,i);
        }
        oled_display(sysPrice,icSurplus);
	}
	else
	{
		readError[4] = xor_count(readError,1,3);				//计算校验
		Uart1_Send_LenString(readError,5);						//操作失败
        ledFlickerSet(2);
		UART1_RX_STA = 0;
	}
}



/*********************************************************************************************
* 名称：update_price
* 功能：更新票价
* 参数：price：票价
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void update_price(unsigned short* price)
{	
	/*操作成功返回数组*/
	unsigned char sendCorrect[6] = {	0xBF,0x00,0x0C,
										0x00};
	
	*price = U1RX_Buf[3]*256 + U1RX_Buf[4];
	
	sendCorrect[3] = xor_count(sendCorrect,1,2);//计算校验
	Uart1_Send_LenString(sendCorrect,4);		//操作成功
    ledFlickerSet(2);	
}



/*********************************************************************************************
* 名称：update_icMode
* 功能：更新ic卡模式
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void update_icMode(u8 data)
{			
	/*操作成功返回数组*/
	unsigned char sendCorrect[6] = {	0xBF,0x00,0x0D,
										0x00};
    
    if(data==0x0b)
        icMode = 1;                         //充值模式
    else
        icMode = 0;                         //消费模式 
    
	sendCorrect[3] = xor_count(sendCorrect,1,2);//计算校验
	Uart1_Send_LenString(sendCorrect,4);		//操作成功
    ledFlickerSet(2);	
}



/*********************************************************************************************
* 名称：update_icPassword
* 功能：更新ic卡密码
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void update_icPassword()
{
    u8 i=0;
	/*操作成功返回数组*/
	unsigned char sendCorrect[6] = {	0xBF,0x00,0x0E,
										0x00};
    
    if(icMode==1)
    {
        for(i=0;i<6;i++)
        {
           icPasswordB[i] = U1RX_Buf[3+i];//更新B组密码
        }
    }
    else
    {                           
        for(i=0;i<6;i++)
        {
           icPasswordA[i] = U1RX_Buf[3+i];//更新A组密码
        }
    }
    
	sendCorrect[3] = xor_count(sendCorrect,1,2);//计算校验
	Uart1_Send_LenString(sendCorrect,4);		//操作成功
    ledFlickerSet(2);	
}





/*********************************************************************************************
* 名称：pc_el
* 功能：pc控制el卡
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void pc_el()
{
    //公交刷卡系统模式
    if((UART1_RX_STA&0x80)==0x80)							//数据接收完成
    {
        u8 check_temp = xor_count(U1RX_Buf,1,(UART1_RX_STA&0x7f)-1);//异或校验
        if(check_temp==U1RX_Buf[UART1_RX_STA&0x7f])		    //校验正确
        {
            switch(U1RX_Buf[2])
            {
                //ic充值，ic扣款
                case 0x0b:
                    update_icData();				        //更新ic卡数据
                    break;
                    
                //更新系统票价
                case 0x0c:						
                    update_price(&sysPrice);		
                    oled_display(sysPrice,0xffffffff);
                    break;
                
                //修改ic卡使用模式
                case 0x0d:
                    update_icMode(U1RX_Buf[3]);
                    break;    
                
                //修改ic卡密码
                case 0x0e:
                    update_icPassword();
                    break;
            }
        }
        UART1_RX_STA = 0;
    } 
}