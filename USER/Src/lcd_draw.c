#include "lcd_draw.h"
#include "lcd.h"
#include "stdio.h"
#include "string.h"
#include "lcdfont.h"

/* 清屏 */
void LCD_Clear(uint16_t Color)
{
    uint32_t total_pixels=(uint32_t)lcddev.width*lcddev.height;

    LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1); //设置区域，清屏=全屏
    
	for(uint32_t i=0;i<total_pixels;i++)
    {
        lcd_wr_data(Color);
    }
}

/* 设置光标 */
void LCD_SetCursor(uint16_t x,uint16_t y)
{
    //检查坐标范围
    if(x>lcddev.width) x=lcddev.width;
    if(y>lcddev.height) y=lcddev.height;

	lcd_wr_regno(lcddev.setxcmd);
	lcd_wr_data(x>>8);
	lcd_wr_data(x&0xFF);
	lcd_wr_regno(lcddev.setycmd);
	lcd_wr_data(y>>8);
	lcd_wr_data(y&0xFF);
}

/* 画点 */
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{
    LCD_SetCursor(x,y);  //设置光标位置
	LCD_WriteRAM_Prepare();
    lcd_wr_data(color);
}

/* 设置显示区域 */
void LCD_SetWindows(uint16_t xStart, uint16_t yStart,uint16_t xEnd,uint16_t yEnd)
{
    // 设置X坐标范围（0-319）
    lcd_wr_regno(lcddev.setxcmd);  // 0x2A
    lcd_wr_data(xStart>>8);           // X起始坐标 高8位
	lcd_wr_data(xStart&0xFF);		  // X起始坐标 低8位
	lcd_wr_data(xEnd>>8);             // X结束坐标 高8位
    lcd_wr_data(xEnd&0XFF);           // X结束坐标 低8位
    
    // 设置Y坐标范围（0-479）
    lcd_wr_regno(lcddev.setycmd);  // 0x2B
    lcd_wr_data(yStart>>8);           // Y起始坐标 高8位
	lcd_wr_data(yStart&0XFF);         // Y起始坐标 高8位
    lcd_wr_data(yEnd>>8);             // Y结束坐标 高8位
    lcd_wr_data(yEnd&0XFF);           // Y结束坐标 高8位
    
    // 准备写入GRAM
    LCD_WriteRAM_Prepare();        // 0x2C
}

/* 填充指定区域 */
void LCD_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color)
{
	uint16_t width=xend-xsta+1;
	uint16_t heighth=yend-ysta+1;
	uint32_t total_pixels=(uint32_t)width*heighth;

	LCD_SetWindows(xsta,ysta,xend,yend);

	for(uint32_t i=0;i<total_pixels;i++)
	{
		lcd_wr_data(color);
	}
}

/* 画线 */
void LCD_DrawLine(uint16_t x,uint16_t y,uint16_t x_end,uint16_t y_end,uint16_t color)
{
    uint16_t t;
    int xerr=0,yerr=0,delta_x,delta_y,distance=0;
	int incx,incy,uRow,uCol;
    //计算坐标增量
	delta_x=x_end-x;  
	delta_y=y_end-y;
    //画线起点坐标
	uRow=x;  
	uCol=y;

	if(delta_x>0)  incx=1;   //设置单步方向 
	else if (delta_x==0)  incx=0;   //垂直线 
	else { incx=-1; delta_x=-delta_x;}

	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线 
	else {incy=-1;delta_y=-delta_y;}

	if(delta_x>delta_y) distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		LCD_DrawPoint(uRow,uCol,color);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

// 画矩形
void lcd_draw_rectangle(uint16_t x,uint16_t y,uint16_t wide,uint16_t height,uint16_t color)
{
	LCD_DrawLine(x,y,x+wide,y,color);
	LCD_DrawLine(x+wide,y,x+wide,y+height,color);
	LCD_DrawLine(x,y,x,y+height,color);
	LCD_DrawLine(x,y+height,x+wide,y+height,color);
}

//显示单个字符
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size,uint16_t color,uint16_t back_color)
{
    uint8_t i, j,temp;
	uint16_t y0=y,x0=x;
    uint8_t *pfont = 0;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); // 统计一个字符所占的字节数
    if(x > lcddev.width || y > lcddev.height)  return;  // 检查坐标范围

	if(size==64)
	{
        uint8_t index=chr-'0';
        if(chr == ':')
            index = 10;
        
        pfont = (uint8_t *)asc2_6432[index];
        
        LCD_WriteRAM_Prepare();  // 开始写入GRAM
        
        // 64x32 字体：64列，每列4字节
        for(uint16_t col = 0; col < 64; col++)  // 64列
        {
            // 每列4个字节
            for(uint8_t byte_idx = 0; byte_idx < 4; byte_idx++)
            {
                temp = pfont[col * 4 + byte_idx];
                
                // 处理这个字节的8个点
                for(j = 0; j < 8; j++)
                {
                    if(temp & (0x80 >> j))
                        LCD_DrawPoint(x0 + col, y0 + byte_idx*8 + j, color);
                    else
                        LCD_DrawPoint(x0 + col, y0 + byte_idx*8 + j, back_color);
                }
            }
        }
	}
	else
	{
		if(chr < ' ' || chr > '~') return;  // 只支持可打印ASCII字符

		chr=chr-' ';
		switch (size)
		{
			case 12:
				pfont = (uint8_t *)asc2_1206[chr];  /* 调用1206字体 */
				break;
			case 16:
				pfont = (uint8_t *)asc2_1608[chr];  /* 调用1608字体 */
				break;
			case 24:
				pfont = (uint8_t *)asc2_2412[chr];  /* 调用2412字体 */
				break;
			case 32:
				pfont = (uint8_t *)asc2_3216[chr];  /* 调用3216字体 */
				break;
			default:
				return;
		}
		LCD_WriteRAM_Prepare();  // 开始写入GRAM

		// 逐行显示字符点
		for(i = 0; i < csize; i++) //字符字节数
		{
			temp=pfont[i]; //获取字符的点阵数据
			for(j = 0; j < 8; j++) //一字节
			{ 
				if(temp & (0x80>>j)){
					LCD_DrawPoint(x,y,color);  // 该点需要显示
				}
				else
					LCD_DrawPoint(x,y,back_color);
				y++;
				if(y>=lcddev.height) return; //超区域
					
				if((y-y0)==size) //显示完一列
				{
					y=y0; //y坐标复位
					x++;  //x坐标递增
					if(x>=lcddev.width) return;
						break;
				}
			}
		}
	}
}

//显示字符串
void lcd_show_string(uint16_t x, uint16_t y,uint8_t size, char *p, uint16_t color,uint16_t back_color)
{
    uint16_t x0 = x;

    // 检查参数
    if(p == NULL) return;
    
    // 逐个字符显示
	while((*p<='~')&&(*p>=' '))  //判断是否为非法字符
	{
		if(x>=lcddev.width)
		{
			x=x0;
			y+=size;
		}
		if(y>=lcddev.height) break;
		
		lcd_show_char(x,y,*p,size,color,back_color);
		x+=size/2;
		p++;
	}
}

//显示圆
void lcd_show_circle(uint16_t x,uint16_t y,uint16_t r,uint16_t color)
{
    int a,b;
    a=0;b=r;
    while (a<=b)
    {
        LCD_DrawPoint(x-b,y-a,color);
        LCD_DrawPoint(x+b,y-a,color);
        LCD_DrawPoint(x-a,y+b,color);
        LCD_DrawPoint(x-a,y-b,color);
        LCD_DrawPoint(x+b,y+a,color);
        LCD_DrawPoint(x+a,y-b,color);
        LCD_DrawPoint(x+a,y+b,color);
        LCD_DrawPoint(x-b,y+a,color);
        a++;
        if((a*a+b*b)>(r*r))
            b--;
    }
}

/* 显示单个汉字 */
void LCD_ShowChinese(uint16_t x,uint16_t y,uint8_t num,uint16_t sizey,uint16_t color,uint16_t back_color)
{
	uint8_t i,j;
	uint16_t TypefaceNum; //字符所占字节数
	uint16_t x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey; //一个字符所占字节数

	LCD_WriteRAM_Prepare(); //开始写入RAM

	for(i=0;i<TypefaceNum;i++)
	{
		for(j=0;j<8;j++)
		{
			// if(sizey==12){
			// 	if(tfont12[num].Msk[i]&(0x01<<j))	
			// 		LCD_DrawPoint(x,y,color);
			// 	// else
			// 	// 	LCD_DrawPoint(x,y,back_color);
			// 	x++;
			// 	if((x-x0)==sizey)
			// 	{
			// 		x=x0;
			// 		y++;
			// 		break;
			// 	}
			// }
			if(sizey==16){
				if(tfont16[num].Msk[i]&(0x01<<j))	
					LCD_DrawPoint(x,y,color);
				else
					LCD_DrawPoint(x,y,back_color);
				x++;
				if((x-x0)==sizey)
				{
					x=x0;
					y++;
					break;
				}
			}
			else if(sizey==24){
				if(tfont24[num].Msk[i]&(0x01<<j))	
					LCD_DrawPoint(x,y,color);
				else
					LCD_DrawPoint(x,y,back_color);
				x++;
				if((x-x0)==sizey)
				{
					x=x0;
					y++;
					break;
				}
			}
			else{
				if(tfont32[num].Msk[i]&(0x01<<j))	
					LCD_DrawPoint(x,y,color);
				else
					LCD_DrawPoint(x,y,back_color);
				x++;
				if((x-x0)==sizey)
				{
					x=x0;
					y++;
					break;
				}
			}
		}
	}
}

//显示中文字符串
void lcd_showchinese(uint16_t x,uint16_t y,int8_t sizey,const char *s,uint16_t color,uint16_t back_color)
{
	uint16_t num;
	while(*s!=0)
	{
		// if(sizey==12){
		// 	for(int i=0;i<sizeof(tfont12)/sizeof(typFNT_GB12);i++){
		// 		if(memcmp(font_labels[i], s, 3) == 0){
		// 			num=i;
		// 			break;
		// 		}
		// 	}
		// 	LCD_ShowChinese(x,y,num,sizey,color);
		// }
		if(sizey==16) {
			for(int i=0;i<sizeof(tfont16)/sizeof(typFNT_GB16);i++){
				if(memcmp(font_label[i], s, 3) == 0){
					num=i;
					break;
				}
			}
			LCD_ShowChinese(x,y,num,sizey,color,back_color);
		}
		else if(sizey==24)
		{
			for(int i=0;i<sizeof(tfont24)/sizeof(typFNT_GB24);i++){
				if(memcmp(font_labels[i], s, 3) == 0){
					num=i;
					break;
				}
			}
			LCD_ShowChinese(x,y,num,sizey,color,back_color);
		} 
		else if(sizey==32)
		{
			for(int i=0;i<sizeof(tfont32)/sizeof(typFNT_GB32);i++){
				if(memcmp(font_labels[i], s, 3) == 0){
					num=i;
					break;
				}
			}
			LCD_ShowChinese(x,y,num,sizey,color,back_color);
		} 
		else return;
		s+=3;
		x+=sizey;
	}
}

/* 计算整数位数，存储每一位数 */
static uint16_t caculate(uint16_t data,uint8_t *digits)
{
	//递归
	// if(data==0){
	// 	return 0; //终止条件
	// }
	// return 1+caculate(data/10);
	if(data==0){
		digits[0]=0;
		return 1;
	}
	uint16_t length=0;
	while(data!=0)
	{
		digits[length]=data%10;
		length++;
		data/=10;
	}
	return length;
}

/* 显示数字 */
void lcd_shownum(uint16_t x,uint16_t y,uint8_t sizey,uint16_t num,uint16_t color)
{
	uint8_t digit[10];
	uint16_t len=caculate(num,digit);
	for(int i=len-1;i>=0;i--)
	{
		char ch=digit[i]+'0';
		lcd_show_char(x,y,ch,sizey,color,WHITE);
		x+=(sizey/2);
	}
}

/* 显示图片 */
void lcd_showpicture(uint16_t x,uint16_t y,uint16_t width,uint16_t length, const uint8_t pic[])
{
	uint16_t i,j;
	uint32_t k=0;

	/* 设置显示区域并准备写入GRAM */
	LCD_SetWindows(x, y, x + width - 1, y + length - 1);

	for(i=0;i<length;i++)
	{
		for(j=0;j<width;j++)
		{
			uint16_t color = (uint16_t)((pic[k*2] << 8) | pic[k*2 + 1]);
			lcd_wr_data(color);
			k++;
		}
	}
}

/**
 * @brief       画水平线
 * @param       x,y   : 起点坐标
 * @param       len   : 线长度
 * @param       color : 矩形的颜色
 * @retval      无
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > lcddev.width) || (y > lcddev.height))
    {
        return;
    }

    LCD_DrawLine(x, y, x + len - 1, y, color);
}

/**
 * @brief       填充实心圆
 * @param       x,y  : 圆中心坐标
 * @param       r    : 半径
 * @param       color: 圆的颜色
 * @retval      无
 */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    uint32_t i;
    uint32_t imax = ((uint32_t)r * 707) / 1000 + 1;
    uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
    uint32_t xr = r;

    lcd_draw_hline(x - r, y, 2 * r, color);

    for (i = 1; i <= imax; i++)
    {
        if ((i * i + xr * xr) > sqmax)
        {
            /* draw lines from outside */
            if (xr > imax)
            {
                lcd_draw_hline (x - i + 1, y + xr, 2 * (i - 1), color);
                lcd_draw_hline (x - i + 1, y - xr, 2 * (i - 1), color);
            }
            xr--;
        }
        /* draw lines from inside (center) */
        lcd_draw_hline(x - xr, y + i, 2 * xr, color);
        lcd_draw_hline(x - xr, y - i, 2 * xr, color);
    }
}

//更新时间函数
void update_time_display(uint16_t x, uint16_t y, uint8_t size, char *time_str)
{
    static char last_time[16] = "";
    uint8_t char_width = size/2;
    
    // 检查小时是否变化
    if(time_str[0] != last_time[0] || time_str[1] != last_time[1]) {
        lcd_show_char(x, y, time_str[0], size, 0x0140,WHITE);
        lcd_show_char(x + char_width, y, time_str[1], size, 0x0140,WHITE);
		lcd_show_char(x + 2*char_width,y,':',size,0x0140,WHITE);
    }
    
    // 检查分钟是否变化
    if(time_str[3] != last_time[3] || time_str[4] != last_time[4]) {
        lcd_show_char(x + 3*char_width, y, time_str[3], size, 0x0140,WHITE);
        lcd_show_char(x + 4*char_width, y, time_str[4], size, 0x0140,WHITE);
		lcd_show_char(x + 5*char_width,y,':',size,0x0140,WHITE);
    }
    
    // 检查秒是否变化
    if(time_str[6] != last_time[6] || time_str[7] != last_time[7]) {
        lcd_show_char(x + 6*char_width, y, time_str[6], size, 0x0140,WHITE);
        lcd_show_char(x + 7*char_width, y, time_str[7], size, 0x0140,WHITE);
    }
    
    strcpy(last_time, time_str);
}

//更新时间函数
void update_time(uint16_t x, uint16_t y, uint8_t size, char *time_str)
{
    static char last_time[16] = "";
    uint8_t char_width = size/2;
    
    // 检查小时是否变化
    if(time_str[0] != last_time[0] || time_str[1] != last_time[1]) {
        lcd_show_char(x, y, time_str[0], size, 0x0140,WHITE);
        lcd_show_char(x + char_width, y, time_str[1], size, 0x0140,WHITE);
		lcd_show_char(x + 2*char_width,y,':',size,0x0140,WHITE);
    }
    
    // 检查分钟是否变化
    if(time_str[3] != last_time[3] || time_str[4] != last_time[4]) {
        lcd_show_char(x + 3*char_width, y, time_str[3], size, 0x0140,WHITE);
        lcd_show_char(x + 4*char_width, y, time_str[4], size, 0x0140,WHITE);
    }
    
    strcpy(last_time, time_str);
}
