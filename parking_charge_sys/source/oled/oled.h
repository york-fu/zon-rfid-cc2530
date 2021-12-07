#ifndef OLED_H
#define OLED_H
#include <ioCC2530.h>

void OLED_Init(void);
void OLED_Write_command(unsigned char IIC_Command);
void OLED_IIC_write(unsigned char IIC_Data);
void OLED_fillpicture(unsigned char fill_Data);
void OLED_Clear(void) ;
void OLED_Refresh_Gram(void);
void OLED_Display_Off(void);
void OLED_Display_On(void);
void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t);
void OLED_DisFill(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char dot);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char Char_Size);
void OLED_ShowCHinese(unsigned char x,unsigned char y,unsigned char no);
void OLED_ShowString(unsigned char x,unsigned char y,unsigned char *chr,unsigned char Char_Size);
void OLED_DisClear(int hstart,int hend,int lstart,int lend);
void OLED_Fill(void);
void oled_turn_on(void);
void oled_turn_off(void);
#endif
