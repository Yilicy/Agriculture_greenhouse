#ifndef __LCD_H_
#define __LCD_H_

#include "stm32f407xx.h"

//画笔颜色
#define WHITE       0xFFFF
#define BLACK      	0x0000	  
#define BLUE       	0x001F  
#define BRED        0XF81F
#define GRED 		0XFFE0
#define GBLUE		0X07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define BROWN 		0XBC40 //棕色
#define BRRED 		0XFC07 //棕红色
#define GRAY  		0X8430 //灰色
#define DARKBLUE    0X01CF	//深蓝色
#define LIGHTBLUE   0X7D7C	//浅蓝色  
#define GRAYBLUE    0X5458 //灰蓝色
#define LIGHTGREEN  0X841F //浅绿色
#define LIGHTGRAY   0XEF5B //浅灰色(PANNEL)
#define LGRAY 	    0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE   0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE      0X2B12 //浅棕蓝色(选择条目的反色)

void LCD_Clear(uint16_t Color); //清屏
void LCD_SetCursor(uint16_t x,uint16_t y);  //设置光标
void LCD_SetWindows(uint16_t xStart, uint16_t yStart,uint16_t xEnd,uint16_t yEnd);  //设置窗口大小
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color); //画点
void LCD_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color); //填充指定区域
void LCD_DrawLine(uint16_t x,uint16_t y,uint16_t x_end,uint16_t y_end,uint16_t color);  //画线
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);    //画水平线

void lcd_show_circle(uint16_t x,uint16_t y,uint16_t r,uint16_t color);   /* 显示圆函数 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint16_t color, uint16_t back_color);   /* 显示单个字符函数 */
extern void lcd_show_string(uint16_t x, uint16_t y, uint8_t size, char *p, uint16_t color, uint16_t back_color);  /* 显示字符串函数 */
void LCD_ShowChinese(uint16_t x,uint16_t y,uint8_t num,uint16_t sizey,uint16_t color, uint16_t back_color);  /* 显示单个汉字 */
void lcd_showchinese(uint16_t x,uint16_t y,int8_t sizey,const char *s,uint16_t color, uint16_t back_color);  /* 显示汉字串 */
void lcd_shownum(uint16_t x,uint16_t y,uint8_t sizey,uint16_t num,uint16_t color, uint16_t back_color);  /* 显示数字 */
void lcd_showpicture(uint16_t x,uint16_t y,uint16_t width,uint16_t length,uint8_t pic[]); /* 显示图片 */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);  /* 填充实心圆 */
void update_time_display(uint16_t x, uint16_t y, uint8_t size, char *time_str);

#endif
