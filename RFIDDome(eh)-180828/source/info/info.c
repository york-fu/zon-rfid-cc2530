/*********************************************************************************************
* 文件：info.c
* 作者：liutong 2016.7.20
* 说明：通过串口控制LCD显示的相关程序
* 修改：
* 注释：
*********************************************************************************************/
#include <ioCC2530.h>             //引入CC2530所对应的头文件（包含各SFR的定义）
#include "sys_init.h"
#include "uart.h"
#include <stdio.h>
#include "info.h"
#include "string.h"
#define HAL_INFOP_IEEE_OSET        0xC                          //mac地址偏移量
/*********************************************************************************************
* 名称：sensor_init()
* 功能：传感器硬件初始化
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void lcd_dis(void){
  for(unsigned char i = 0;i<2;i++){                              //发送TYPE,发2遍
    Uart0_Send_String("{TYPE=00005}");                            //Uart实验
    halWait(250);
    halWait(250);
  }
  
  halWait(250);
  halWait(250);
  char CC2530_MAC[30] = {0};                                     //存放MAC
  char devmacaddr[8];
  unsigned char *macaddrptr = (unsigned char *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET);
  for(int i=0;i<8;i++) {
    devmacaddr[i] = macaddrptr[i];                              //获取mac地址
  }
  strcat(CC2530_MAC,"{MAC=");
  sprintf(&CC2530_MAC[5],"%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                          devmacaddr[7],devmacaddr[6],devmacaddr[5],
                          devmacaddr[4],devmacaddr[3],devmacaddr[2],
                          devmacaddr[1],devmacaddr[0]);
  CC2530_MAC[28]='}';
   for(unsigned char i = 0;i<2;i++){                            //发送MAC，发2遍
   Uart0_Send_String(CC2530_MAC);  
   halWait(250);
   halWait(250);
  }
}