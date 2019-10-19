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


/*系统模式,0-->公交刷卡系统，1-->考勤系统*/
u8 sysMode = 0;

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
	unsigned char ICData[17] = {0};
	
	if(mcuRead_memory(0x00,0x01,0x01,0x0A,icPasswordA,ICData))
	{
		for(i=0;i<16;i++)
		{
			sendCorrect[i+3] = ICData[i];						//获取扇区数据
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
	unsigned char ICData[17] = {0};
	
	if(mcuRead_UID(0x00,cardUID))								//读取卡号
	{	
		for(i=0;i<4;i++)
		{
			sendCorrect[i+3] = cardUID[i];						//获取卡号
		}
		
		if(mcuRead_memory(0x00,0x01,0x01,0x0a,icPasswordA,ICData))//读余额
		{
			for(i=0;i<4;i++)
			{
				sendCorrect[10-i] = ICData[15-i];				//获取余额
			}
			sendCorrect[11] = xor_count(sendCorrect,1,10);		//计算校验
			Uart1_Send_LenString(sendCorrect,12);				//返回信息
            
            /*蜂鸣器鸣叫*/
            P0_4 = 0;
            delay_ms(20);
            P0_4 = 1;          
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
		UART1_RX_STA = 0;
	}
	else
	{
		readError[4] = xor_count(readError,1,3);				//计算校验
		Uart1_Send_LenString(readError,5);						//操作失败
		UART1_RX_STA = 0;
	}
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
}



/*********************************************************************************************
* 名称：mcuRead_idCard
* 功能：读取ID卡卡号
* 参数：读取地址，卡号BUF
* 返回：1：成功，0：失败
* 修改：
* 注释：
*********************************************************************************************/
unsigned char mcuRead_idCard(unsigned char icAdd,unsigned char* idBuf)
{	
	unsigned char i=0,check_temp=0;
	/*读IC卡命令*/
	unsigned char readIdCommand[7] = {	0xAB,0xBA,
                                        icAdd,0x15,0x00,
                                        0x00};	
	readIdCommand[5] = xor_count(readIdCommand,2,4);						//计算校验
	Uart0_Send_LenString(readIdCommand,6);							//发送读卡号命令
    
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
			if(check_temp==U0RX_Buf[UART0_RX_STA&0x7f])		    //校验正确
			{	
				for(i=0;i<4;i++)
				{
					idBuf[i] = U0RX_Buf[i+5];				//获取卡号
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
* 名称：oled_display
* 功能：oled显示
* 参数：公交系统票价
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void oled_display(u16 price,u8* idCardNumber)
{
    if(sysMode==1)
    {
        u32 CardNumber=0;
        u8 CardNumber_temp[11]={0};
        
        //16进制转10进制
        for(u8 i=0;i<4;i++)
        {
            CardNumber += idCardNumber[i]*pow(256,i);
        }
        //数字转字符
        CardNumber_temp[10] = '\0';
        for(u8 i=0;i<10;i++)
        {
            CardNumber_temp[9-i] = (CardNumber/(u32)pow(10,i))%10 + '0';
        }
        //显示信息
        OLED_ShowString(1,1,"CardNumber:",11);							
        OLED_ShowString(1,32,CardNumber_temp,10);						//显示卡号，10进制
    }
    else
    {
        u8 priceTemp[5];
        
        priceTemp[4] = '\0';
        for(u8 i=0;i<4;i++)
        {
            priceTemp[4-i] = (price/(u32)pow(10,i))%10 + '0';
        }
        //显示信息    
        OLED_ShowString(20,1,"Fare:",6);							
        OLED_ShowString(50,1,priceTemp,6);							//显示票价，10进制
    }
}


/*********************************************************************************************
* 名称：reported_idInfo
* 功能：上报ID卡信息
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
u8 reported_idInfo()
{
    u8 idReportedBuf[] = {  0xBF,0x04,0x0F,
                            0x00,0x00,0x00,0x00,
                            0x00};
    u8 idCardNumber[4] = {0};
    
    if(mcuRead_idCard(0x00,idCardNumber))                   //读取成功
    {
        for(u8 i=0;i<4;i++)
        {
            idReportedBuf[3+i] = idCardNumber[i];
        }
		idReportedBuf[7] = xor_count(idReportedBuf,1,6);        //计算校验
		Uart1_Send_LenString(idReportedBuf,8);					//操作成功
        
        //显示卡号
        oled_display(0,idCardNumber);
        
        return 1;
	}
    return 0;
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
	static u16 price=2;									    //系统票价
	
    //考勤系统模式
    if(sysMode==1)
    {
        if((UART1_RX_STA&0x80)==0x80)								//数据接收完成
        {
            u8 check_temp = xor_count(U1RX_Buf,1,(UART1_RX_STA&0x7f)-1);//异或校验
            if(check_temp==U1RX_Buf[UART1_RX_STA&0x7f])		        //校验正确
            {
                switch(U1RX_Buf[2])
                {
                    //考勤系统，开门操作
                    case 0x0f:
                        relay1_control(1);
                        relay_tiem = 20*3;
                        break;
                }
            }
            UART1_RX_STA = 0;
        } 
    }
    //公交刷卡系统模式
    else
    {
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
                        update_price(&price);				
                        break;
                    
                    //修改ic卡使用模式
                    case 0x0d:						
                        if(U1RX_Buf[3]==0x0b)
                            icMode = 1;                         //充值模式
                        else
                            icMode = 0;                         //消费模式
                        break;    
                    
                    //修改ic卡密码
                    case 0x0e:
                        update_icPassword();
                        break;
                }
                //显示信息
                oled_display(price,NULL);
            }
            UART1_RX_STA = 0;
        } 
    }
}